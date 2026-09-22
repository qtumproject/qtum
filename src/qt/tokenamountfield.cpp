#include <qt/tokenamountfield.h>

#include <qt/bitcoinunits.h>
#include <qt/styleSheet.h>
#include <qt/qvaluecombobox.h>

#include <QApplication>
#include <QAbstractSpinBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QHBoxLayout>

/** QSpinBox that uses fixed-point numbers internally and uses our own
 * formatting/parsing functions.
 */
class TokenAmountSpinBox: public QAbstractSpinBox
{
    Q_OBJECT

public:
    explicit TokenAmountSpinBox(QWidget *parent):
        QAbstractSpinBox(parent),
        decimalUnits(0),
        totalSupply(0),
        singleStep(0),
        minAmount(0)
    {
        setAlignment(Qt::AlignRight);

        connect(lineEdit(), &QLineEdit::textEdited, this, &TokenAmountSpinBox::valueChanged);
    }

    QValidator::State validate(QString &text, int &pos) const override
    {
        if(text.isEmpty())
            return QValidator::Intermediate;
        bool valid = false;
        parse(text, &valid);
        /* Make sure we return Intermediate so that fixup() is called on defocus */
        return valid ? QValidator::Intermediate : QValidator::Invalid;
    }

    void fixup(QString &input) const override
    {
        bool valid = false;
        dev::s256 val = parse(input, &valid);
        val = getMax(val, minAmount);
        if(valid)
        {
            input = BitcoinUnits::formatToken(decimalUnits, val, false, BitcoinUnits::SeparatorStyle::ALWAYS);
            lineEdit()->setText(input);
        }
    }

    dev::s256 value(bool *valid_out=0) const
    {
        return parse(text(), valid_out);
    }

    void setValue(const dev::s256& value)
    {
        dev::s256 val = getMax(value, minAmount);
        lineEdit()->setText(BitcoinUnits::formatToken(decimalUnits, val, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        Q_EMIT valueChanged();
    }

    void stepBy(int steps) override
    {
        bool valid = false;
        dev::s256 val = value(&valid);
        val = val + steps * singleStep;
        val = getMin(getMax(val, minAmount), totalSupply);
        setValue(val);
    }

    dev::s256 minimum() const
    {
        return minAmount;
    }

    void setMinimum(const dev::s256& min)
    {
        minAmount = min;
        Q_EMIT valueChanged();
    }

    void setTotalSupply(const dev::s256 &value)
    {
        totalSupply = value;
    }

    void setDecimalUnits(int value)
    {
        decimalUnits = value;
        setSingleStep();
    }

private:
    int decimalUnits; // Token decimal units
    dev::s256 totalSupply; // Token total supply
    dev::s256 singleStep;
    dev::s256 minAmount;

    /**
     * Parse a string into a number of base monetary units and
     * return validity.
     * @note Must return 0 if !valid.
     */
    dev::s256 parse(const QString &text, bool *valid_out=0) const
    {
        dev::s256 val = 0;
        bool valid = BitcoinUnits::parseToken(decimalUnits, text, &val);
        if(valid)
        {
            if(val < 0 || val > totalSupply)
                valid = false;
        }
        if(valid_out)
            *valid_out = valid;
        return valid ? val : 0;
    }

    void setSingleStep()
    {
        dev::s256 step = 1;
        for(int i = 1; i < decimalUnits; i++)
        {
            step *= 10;
        }
        singleStep = step;
    }

    dev::s256 getMax(dev::s256 a, dev::s256 b) const{
        return a > b ? a : b;
    }

    dev::s256 getMin(dev::s256 a, dev::s256 b) const{
        return a > b ? b : a;
    }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Comma)
            {
                // Translate a comma into a period
                QKeyEvent periodKeyEvent(event->type(), Qt::Key_Period, keyEvent->modifiers(), ".", keyEvent->isAutoRepeat(), keyEvent->count());
                return QAbstractSpinBox::event(&periodKeyEvent);
            }
        }
        return QAbstractSpinBox::event(event);
    }

    StepEnabled stepEnabled() const override
    {
        if (isReadOnly()) // Disable steps when TokenAmountSpinBox is read-only
            return StepNone;
        if (text().isEmpty()) // Allow step-up with empty field
            return StepUpEnabled;

        StepEnabled rv = QAbstractSpinBox::StepNone;
        bool valid = false;
        dev::s256 val = value(&valid);
        if(valid)
        {
            if(val > minAmount)
                rv |= StepDownEnabled;
            if(val < totalSupply)
                rv |= StepUpEnabled;
        }
        return rv;
    }

Q_SIGNALS:
    void valueChanged();
};

#include <qt/tokenamountfield.moc>

TokenAmountField::TokenAmountField(QWidget *parent) :
    QWidget(parent),
    amount(0)
{
    amount = new TokenAmountSpinBox(this);
    amount->setLocale(QLocale::c());
    amount->installEventFilter(this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(amount);

    layout->setContentsMargins(0,0,0,0);

    setLayout(layout);
    connect(amount, &TokenAmountSpinBox::valueChanged, this, &TokenAmountField::valueChanged);
}

void TokenAmountField::clear()
{
    amount->clear();
}

void TokenAmountField::setEnabled(bool fEnabled)
{
    amount->setEnabled(fEnabled);
}

bool TokenAmountField::validate()
{
    bool valid = false;
    value(&valid);
    setValid(valid);
    return valid;
}

void TokenAmountField::setValid(bool valid)
{
    if (valid)
        amount->setStyleSheet("");
    else
        SetObjectStyleSheet(amount, StyleSheetNames::Invalid);
}

bool TokenAmountField::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn)
    {
        // Clear invalid flag on focus
        setValid(true);
    }
    return QWidget::eventFilter(object, event);
}

dev::s256 TokenAmountField::value(bool *valid_out) const
{
    return amount->value(valid_out);
}

void TokenAmountField::setValue(const dev::s256& value)
{
    amount->setValue(value);
}

void TokenAmountField::setReadOnly(bool fReadOnly)
{
    amount->setReadOnly(fReadOnly);
}

dev::s256 TokenAmountField::minimum() const
{
    return amount->minimum();
}

void TokenAmountField::setMinimum(const dev::s256& min)
{
    amount->setMinimum(min);
}

void TokenAmountField::setTotalSupply(const dev::s256 &value)
{
    amount->setTotalSupply(value);
}

void TokenAmountField::setDecimalUnits(int value)
{
    amount->setDecimalUnits(value);
}

QString TokenAmountField::text() const
{
    return QString::fromStdString(value().str());
}
