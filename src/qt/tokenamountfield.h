#ifndef TOKENAMOUNTFIELD_H
#define TOKENAMOUNTFIELD_H

#include <amount.h>
#include <QWidget>

#include <boost/multiprecision/cpp_int.hpp>
using namespace boost::multiprecision;

class TokenAmountSpinBox;

/** Widget for entering token amounts.
  */
class TokenAmountField : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int256_t value READ value WRITE setValue NOTIFY valueChanged USER true)

public:
    explicit TokenAmountField(QWidget *parent = 0);
    int256_t value(bool *value=0) const;
    void setValue(const int256_t& value);

    /** Set single step **/
    void setSingleStep(const int256_t& step);

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

    int256_t minimum() const;
    void setMinimum(const int256_t& min);

    void setTotalSupply(const int256_t &value);
    void setDecimalUnits(int value);

    QString text() const;

Q_SIGNALS:
    void valueChanged();

protected:
    /** Intercept focus-in event and ',' key presses */
    bool eventFilter(QObject *object, QEvent *event);

public Q_SLOTS:

private:
    TokenAmountSpinBox *amount;
};

#endif // TOKENAMOUNTFIELD_H
