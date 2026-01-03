#pragma once
#include <QDialog>
#include <QDate>
#include "models.h"
#include "account.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QComboBox;
class QCheckBox;
class QDateTimeEdit;

class AddEntryDialog : public QDialog {
    Q_OBJECT
public:
    enum Page { Expense=0, Income=1, TodoPage=2 };
    explicit AddEntryDialog(const QDate& selectedDate, QWidget *parent=nullptr);

signals:
    void savedExpenseIncome(const AccountItem& item);
    void savedTodo(const Todo& td);

private slots:
    void onSave();

private:
    void buildUi();
    void applyStyle();

    QWidget* buildTopBar();
    QWidget* buildSegment();
    QWidget* buildExpenseIncomePage(bool isIncome);
    QWidget* buildTodoPage();
    QWidget* buildKeypad();

    void switchPage(Page p);
    void updateDateLabel();

    QLineEdit* currentAmountEdit() const;
    QComboBox* currentCategoryBox() const;

private:
    QDate date;

    QLabel *dateLabel = nullptr;
    QPushButton *btnClose = nullptr;
    QPushButton *btnSave  = nullptr;

    QPushButton *segExpense = nullptr;
    QPushButton *segIncome  = nullptr;
    QPushButton *segTodo    = nullptr;

    QStackedWidget *pages = nullptr;

    QLineEdit *amountExpense = nullptr;
    QComboBox *categoryExpense = nullptr;

    QLineEdit *amountIncome = nullptr;
    QComboBox *categoryIncome = nullptr;

    bool currentIsIncome = false;

    // Todo
    QLineEdit *todoTitle = nullptr;
    QCheckBox *allDay = nullptr;
    QDateTimeEdit *startDT = nullptr;
    QDateTimeEdit *endDT = nullptr;
};
