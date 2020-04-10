#include <qt/qvalidatedtextedit.h>
#include <qt/styleSheet.h>

#include <QValidator>

QValidatedTextEdit::QValidatedTextEdit(QWidget *parent) :
    QTextEdit(parent),
    valid(true),
    checkValidator(0),
    emptyIsValid(true),
    isValidManually(false),
    lineByLine(false),
    removeDuplicates(false)
{
    connect(this, &QValidatedTextEdit::textChanged, this, &QValidatedTextEdit::markValid);
    setStyleSheet("");
}

void QValidatedTextEdit::clear()
{
    setValid(true);
    QTextEdit::clear();
}

void QValidatedTextEdit::setCheckValidator(const QValidator *v, bool _lineByLine, bool _removeDuplicates)
{
    checkValidator = v;
    lineByLine = _lineByLine;
    removeDuplicates = _removeDuplicates;
}

bool QValidatedTextEdit::isValid()
{
    // use checkValidator in case the QValidatedTextEdit is disabled
    if (checkValidator)
    {
        if(lineByLine)
        {
            QStringList lines = getLines();

            for (QString line : lines) {
                int pos = 0;
                if (checkValidator->validate(line, pos) == QValidator::Invalid)
                    return false;
            }
        }
        else
        {
            QString line = toPlainText();
            int pos = 0;
            if (checkValidator->validate(line, pos) == QValidator::Acceptable)
                return true;
        }
    }

    return valid;
}

void QValidatedTextEdit::setValid(bool _valid)
{
    if(_valid == this->valid)
    {
        return;
    }

    if(_valid)
    {
        setStyleSheet("");
    }
    else
    {
        SetObjectStyleSheet(this, StyleSheetNames::Invalid);
    }
    this->valid = _valid;
}

void QValidatedTextEdit::setEnabled(bool enabled)
{
    if (!enabled)
    {
        // A disabled QValidatedLineEdit should be marked valid
        setValid(true);
    }
    else
    {
        // Recheck validity when QValidatedLineEdit gets enabled
        checkValidity();
    }

    QTextEdit::setEnabled(enabled);
}

void QValidatedTextEdit::checkValidity()
{
    if (emptyIsValid && toPlainText().isEmpty())
    {
        setValid(true);
    }
    else if(isValidManually)
    {
        setValid(true);
    }
    else if (checkValidator)
    {
        if(lineByLine)
        {
            QStringList lines = getLines();

            for (QString line : lines) {
                int pos = 0;
                if (checkValidator->validate(line, pos) == QValidator::Acceptable)
                    setValid(true);
                else
                    setValid(false);
            }
        }
        else
        {
            QString line = toPlainText();
            int pos = 0;
            if (checkValidator->validate(line, pos) == QValidator::Acceptable)
                setValid(true);
            else
                setValid(false);
        }
    }
    else
        setValid(false);
}

void QValidatedTextEdit::markValid()
{
    // As long as a user is typing ensure we display state as valid
    setValid(true);
}

void QValidatedTextEdit::focusInEvent(QFocusEvent *event)
{
    setValid(true);
    QTextEdit::focusInEvent(event);
}

void QValidatedTextEdit::focusOutEvent(QFocusEvent *event)
{
    if(lineByLine && removeDuplicates)
    {
        QStringList lines = getLines();
        lines.removeDuplicates();
        setLines(lines);
    }
    checkValidity();
    QTextEdit::focusOutEvent(event);
}

bool QValidatedTextEdit::getIsValidManually() const
{
    return isValidManually;
}

void QValidatedTextEdit::setIsValidManually(bool value)
{
    isValidManually = value;
}

bool QValidatedTextEdit::getEmptyIsValid() const
{
    return emptyIsValid;
}

void QValidatedTextEdit::setEmptyIsValid(bool value)
{
    emptyIsValid = value;
}

QStringList QValidatedTextEdit::getLines() const
{
    return toPlainText().split('\n', QString::SkipEmptyParts);
}

void QValidatedTextEdit::setLines(const QStringList &lines)
{
    setPlainText(lines.join('\n'));
}
