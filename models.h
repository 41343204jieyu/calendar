#pragma once
#include <QString>
#include <QDate>
#include <QDateTime>
#include <QStack>
struct Txn {
    QDate date;
    QString category;
    int amount = 0;
    bool isIncome = false;
};

struct Todo {
    QString id;
    QString title;
    bool allDay = true;
    QDateTime start;
    QDateTime end;
};
