#ifndef TOKENAMOUNTFIELD_H
#define TOKENAMOUNTFIELD_H

#include <QWidget>

#include <libdevcore/Common.h>

class TokenAmountSpinBox;

/** Widget for entering token amounts.
  */
class TokenAmountField : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(dev::s256 value READ value WRITE setValue NOTIFY valueChanged USER true)

public:
    explicit TokenAmountField(QWidget *parent = 0);
    dev::s256 value(bool *value=0) const;
    void setValue(const dev::s256& value);

    /** Set single step **/
    void setSingleStep(const dev::s256& step);

    /** Make read-only **/
    void setReadOnly(bool fReadOnly);

    /** Mark current value as invalid in UI. */
    void setValid(bool valid);
    /** Perform input validation, mark field as invalid if entered value is not valid. */
    bool validate();

    /** Make field empty and ready for new input. */
    void clear();

    /** Enable/Disable. */
    void setEnabled(bool fEnabled);

    dev::s256 minimum() const;
    void setMinimum(const dev::s256& min);

    void setTotalSupply(const dev::s256 &value);
    void setDecimalUnits(int value);

    QString text() const;

Q_SIGNALS:
    void valueChanged();

protected:
    /** Intercept focus-in event and ',' key presses */
    bool eventFilter(QObject *object, QEvent *event) override;

public Q_SLOTS:

private:
    TokenAmountSpinBox *amount;
};

#endif // TOKENAMOUNTFIELD_H
