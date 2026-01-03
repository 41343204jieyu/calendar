#include "mainwindow.h"
#include "dotcalendar.h"
#include "addentrydialog.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QToolButton>
#include <QFrame>

static const QColor BG("#0B0B0B");
static const QColor PANEL("#141414");
static const QColor TEXT("#EDEDED");
static const QColor DIM("#9A9A9A");
static const QColor ACCENT("#F5A623");

static QString monthTitleZh(int y, int m){
    return QString("%1年%2月").arg(y).arg(m);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Calendar Mock");
    setMinimumSize(390, 780);

    auto *root = new QWidget(this);
    auto *v = new QVBoxLayout(root);
    v->setContentsMargins(14,14,14,14);
    v->setSpacing(10);

    v->addWidget(buildTopTitle());
    v->addWidget(buildMonthBar());

    cal = new DotCalendar(this);
    v->addWidget(cal, 1);

    v->addWidget(buildListPanel(), 0);
    v->addWidget(buildBottomBar(), 0);

    setCentralWidget(root);

    applyStyle();

    monthTitle->setText(monthTitleZh(cal->yearShown(), cal->monthShown()));

    connect(cal, &QCalendarWidget::clicked, this, [=](const QDate &d){
        refreshDayList(d);
    });

    connect(cal, &QCalendarWidget::currentPageChanged, this, [=](int y, int m){
        monthTitle->setText(monthTitleZh(y, m));
    });

    refreshDayList(QDate::currentDate());
    refreshCalendarMarks();
}

QWidget* MainWindow::buildTopTitle() {
    auto *w = new QWidget(this);
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(0,0,0,0);

    auto *title = new QLabel("行事曆", w);
    title->setObjectName("topTitle");
    title->setAlignment(Qt::AlignCenter);

    h->addStretch(1);
    h->addWidget(title);
    h->addStretch(1);

    return w;
}

QWidget* MainWindow::buildMonthBar() {
    auto *w = new QWidget(this);
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(0,0,0,0);

    auto mkBtn = [&](const QString& t){
        auto *b = new QToolButton(w);
        b->setText(t);
        b->setFixedSize(34, 28);
        return b;
    };

    auto *prev = mkBtn("‹");
    auto *next = mkBtn("›");
    monthTitle = new QLabel(w);
    monthTitle->setAlignment(Qt::AlignCenter);
    monthTitle->setObjectName("monthTitle");

    h->addWidget(prev);
    h->addStretch(1);
    h->addWidget(monthTitle);
    h->addStretch(1);
    h->addWidget(next);

    connect(prev, &QToolButton::clicked, this, [=]{ cal->showPreviousMonth(); });
    connect(next, &QToolButton::clicked, this, [=]{ cal->showNextMonth(); });

    return w;
}

QWidget* MainWindow::buildListPanel() {
    auto *panel = new QFrame(this);
    panel->setObjectName("listPanel");
    auto *v = new QVBoxLayout(panel);
    v->setContentsMargins(12,10,12,10);
    v->setSpacing(8);

    sumLabel = new QLabel("支出:0", panel);
    sumLabel->setObjectName("sumLabel");
    v->addWidget(sumLabel);

    list = new QListWidget(panel);
    list->setObjectName("expenseList");
    v->addWidget(list);

    return panel;
}

QWidget* MainWindow::buildBottomBar() {
    auto *w = new QWidget(this);
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(0,8,0,0);
    h->setSpacing(10);

    auto mk = [&](const QString& t){
        auto *b = new QToolButton(w);
        b->setText(t);
        b->setToolButtonStyle(Qt::ToolButtonTextOnly);
        b->setFixedHeight(44);
        return b;
    };

    btnBook = mk("帳本");
    btnPlus = mk("+");
    btnTodo = mk("待辦事項");
    btnPlus->setObjectName("plusBtn");

    h->addWidget(btnBook, 1);
    h->addWidget(btnPlus, 1);
    h->addWidget(btnTodo, 1);

    connect(btnPlus, &QToolButton::clicked, this, [=]{
        QDate d = cal->chosenDate();

        AddEntryDialog dlg(d, this);

        connect(&dlg, &AddEntryDialog::savedExpenseIncome, this, [=](const Txn& t){
            txns.push_back(t);
            refreshDayList(d);
            refreshCalendarMarks();
        });

        connect(&dlg, &AddEntryDialog::savedTodo, this, [=](const Todo& td){
            todos.push_back(td);
            refreshCalendarMarks();
        });

        dlg.exec();
    });

    return w;
}

void MainWindow::refreshDayList(const QDate &d) {
    if (!list || !sumLabel) return;

    list->clear();
    int sumExpense = 0;

    for (const auto &t : txns) {
        if (t.date != d) continue;

        QString sign = t.isIncome ? "收入" : "支出";
        if (!t.isIncome) sumExpense += t.amount;

        list->addItem(QString("%1\n%2  %3")
                          .arg(t.category)
                          .arg(sign)
                          .arg(t.amount));
    }

    sumLabel->setText(QString("支出:%1").arg(sumExpense));
    // 2.顯示待辦事項
    for (const auto &td : todos) {
        if (td.start.date() == d) {
            QString timeStr = td.allDay ? "全天" : td.start.toString("hh:mm");
            list->addItem(QString(" [待辦] %1 (%2)").arg(td.title).arg(timeStr));
        }
    }
}

void MainWindow::refreshCalendarMarks() {
    QSet<QDate> marks;

    // 記帳
    for (const auto &t : txns) marks.insert(t.date);

    // 待辦：全天用 start 的日期；非全天也用 start date 標
    for (const auto &td : todos) {
        if (td.allDay) marks.insert(td.start.date());
        else marks.insert(td.start.date());
    }

    cal->setMarkedDates(marks);
}

void MainWindow::applyStyle() {
    qApp->setStyleSheet(QString(R"(
        QWidget { background: %1; color: %2; }

        QLabel#topTitle { font-size: 18px; font-weight: 700; color: %2; padding: 6px 0px; }
        QLabel#monthTitle { font-size: 20px; font-weight: 700; color: %2; }

        QToolButton { border: 1px solid #2A2A2A; border-radius: 12px; background: #121212; color: %2; padding: 6px; }
        QToolButton:hover { background: #1A1A1A; }

        QToolButton#plusBtn { font-size: 18px; font-weight: 700; background: #EDEDED; color: #111; border: none; }

        QFrame#listPanel { background: %3; border: 1px solid #1E1E1E; border-radius: 14px; }
        QLabel#sumLabel { color: %2; font-size: 14px; padding: 6px 2px; }

        QListWidget { background: transparent; border: none; }
        QListWidget::item { padding: 10px; border-bottom: 1px solid #1E1E1E; color: %2; }
    )").arg(BG.name(), TEXT.name(), PANEL.name()));
}
