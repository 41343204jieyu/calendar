#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QString>
#include <QVector>
#include <QDate>

struct AccountItem {
    QDate date;
    QString category;
    int amount = 0;
    QString type;   // "income" / "expense"
    QString note;
};

class Account
{
public:
    Account();

    void addItem(const AccountItem &item);
    bool removeAt(int index);
    void clearDailyItems();

    double dailyIncome() const;
    double dailyExpense() const;
    double dailyNet() const;

    double monthlyIncome(int year, int month);
    double monthlyExpense(int year, int month);

    void setMonthlyBudget(double budget);
    double getMonthlyBudget() const;
    bool isBudgetWarning(int year, int month) const;

    bool loadFromFile(const QDate &date);
    bool saveToFile(const QDate &date) const;

    const QVector<AccountItem>& getItems() const { return m_items; }

    bool updateAt(int idx, const AccountItem& item);


    bool loadMonthlyBudget(int year, int month);
    bool saveMonthlyBudget(int year, int month) const;



private:
    QVector<AccountItem> m_items;
    double m_monthlyBudget = 0.0;

    QString filePath(const QDate &date) const;
};

#endif // ACCOUNT_H
