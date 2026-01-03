#pragma once
#include <QMainWindow>
#include <QDate>
#include <QVector>

#include "models.h"
#include "account.h"

class QLabel;
class QListWidget;
class QToolButton;
class DotCalendar;
class QProgressBar;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent=nullptr);

private:
    void applyStyle();
    QWidget* buildTopTitle();
    QWidget* buildMonthBar();
    QWidget* buildListPanel();
    QWidget* buildBottomBar();

    void refreshDayList(const QDate& d);
    void refreshCalendarMarks();

    void checkBudgetWarning(const QDate& d);
    void refreshMonthSummary(const QDate& d);

    // ===== Todo =====
    bool loadTodosFromFile(const QDate& d);
    bool saveTodosToFile(const QDate& d) const;
    void refreshTodoList(const QDate& d);

private:
    DotCalendar *cal = nullptr;
    QLabel *monthTitle = nullptr;

    // ✅ 月總覽
    QLabel *monthIncomeLabel = nullptr;
    QLabel *monthExpenseLabel = nullptr;
    QLabel *budgetLabel = nullptr;
    QProgressBar *budgetBar = nullptr;

    // ✅ 中間區：切換 記帳/待辦
    QStackedWidget *stack = nullptr;

    // Page 0：記帳
    QLabel *sumLabel = nullptr;
    QListWidget *list = nullptr;

    // Page 1：待辦
    QListWidget *todoList = nullptr;

    QToolButton *btnBook = nullptr;
    QToolButton *btnPlus = nullptr;
    QToolButton *btnTodo = nullptr;

    // ✅ 記帳
    Account account;
    QDate currentDate;

    // ✅ Todo
    QVector<Todo> todos;
    QVector<bool> todoDone;
};
