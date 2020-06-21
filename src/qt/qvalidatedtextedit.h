#ifndef QVALIDATEDTEXTEDIT_H
#define QVALIDATEDTEXTEDIT_H

#include <QTextEdit>

class QValidator;
class QValidatedTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit QValidatedTextEdit(QWidget *parent);
    void clear();
    void setCheckValidator(const QValidator *v, bool lineByLine = false, bool removeDuplicates = false);
    bool isValid();

    bool getEmptyIsValid() const;
    void setEmptyIsValid(bool value);

    bool getIsValidManually() const;
    void setIsValidManually(bool value);

    QStringList getLines() const;
    void setLines(const QStringList& lines);

protected:
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);

private:
    bool valid;
    const QValidator *checkValidator;
    bool emptyIsValid;
    bool isValidManually;
    bool lineByLine;
    bool removeDuplicates;

public Q_SLOTS:
    void setValid(bool valid);
    void setEnabled(bool enabled);
    void checkValidity();

Q_SIGNALS:

private Q_SLOTS:
    void markValid();
};

#endif // QVALIDATEDTEXTEDIT_H
