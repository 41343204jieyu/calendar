#include "mainwindow.h"
#include "dotcalendar.h"
#include "addentrydialog.h"

#include<QStack>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QToolButton>
#include <QFrame>
#include <QFile>


#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QProgressBar>
#include <QStackedWidget>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

static const QColor BG("#0B0B0B");
static const QColor PANEL("#141414");
static const QColor TEXT("#EDEDED");
static const QColor DIM("#9A9A9A");
static const QColor ACCENT("#F5A623");

static QString monthTitleZh(int y, int m){
    return QString("%1年%2月").arg(y).arg(m);
}

// ✅ Todo 檔名：data/yyyy-MM-dd.todo.json
static QString todoFilePath(const QDate& d) {
    QDir dir("data");
    if (!dir.exists()) dir.mkpath(".");
    return QString("data/%1.todo.json").arg(d.toString("yyyy-MM-dd"));
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

    // ✅ 初始化日期 + 載入今天記帳 + Todo
    currentDate = QDate::currentDate();
    account.loadFromFile(currentDate);
    loadTodosFromFile(currentDate);

    monthTitle->setText(monthTitleZh(cal->yearShown(), cal->monthShown()));

    connect(cal, &QCalendarWidget::clicked, this, [=](const QDate &d){
        currentDate = d;

        account.loadFromFile(d);
        loadTodosFromFile(d);

        refreshDayList(d);
        refreshTodoList(d);
        refreshMonthSummary(d);
    });

    connect(cal, &QCalendarWidget::currentPageChanged, this, [=](int y, int m){
        monthTitle->setText(monthTitleZh(y, m));
        refreshCalendarMarks();
        refreshMonthSummary(QDate(y, m, 1));
    });

    refreshDayList(currentDate);
    refreshTodoList(currentDate);
    refreshCalendarMarks();
    refreshMonthSummary(currentDate);

    // 預設進記帳頁
    if (stack) stack->setCurrentIndex(0);
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

    // ✅ 月總覽（固定最上方）
    auto *summary = new QFrame(panel);
    summary->setObjectName("monthSummary");
    auto *sv = new QVBoxLayout(summary);
    sv->setContentsMargins(10,10,10,10);
    sv->setSpacing(6);

    monthIncomeLabel  = new QLabel("本月收入: 0", summary);
    monthExpenseLabel = new QLabel("本月支出: 0", summary);
    budgetLabel       = new QLabel("預算: 未設定", summary);

    budgetBar = new QProgressBar(summary);
    budgetBar->setRange(0, 100);
    budgetBar->setValue(0);
    budgetBar->setTextVisible(false);

    sv->addWidget(monthIncomeLabel);
    sv->addWidget(monthExpenseLabel);
    sv->addWidget(budgetLabel);
    sv->addWidget(budgetBar);

    v->addWidget(summary);

    // ✅ Stack：記帳頁 / 待辦頁
    stack = new QStackedWidget(panel);
    v->addWidget(stack);

    // =========================
    // Page 0：記帳
    // =========================
    auto *accPage = new QWidget(panel);
    auto *av = new QVBoxLayout(accPage);
    av->setContentsMargins(0,0,0,0);
    av->setSpacing(8);

    sumLabel = new QLabel("支出:0", accPage);
    sumLabel->setObjectName("sumLabel");
    av->addWidget(sumLabel);

    list = new QListWidget(accPage);
    list->setObjectName("expenseList");
    av->addWidget(list);

    // ✅ 右鍵刪除單筆記帳
    list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(list, &QListWidget::customContextMenuRequested, this, [=](const QPoint &pos){
        auto *it = list->itemAt(pos);
        if (!it) return;

        int idx = it->data(Qt::UserRole).toInt();

        QMenu menu;
        QAction *del = menu.addAction("刪除這筆記帳");
        QAction *chosen = menu.exec(list->viewport()->mapToGlobal(pos));
        if (chosen != del) return;

        if (QMessageBox::question(this, "刪除", "確定刪除這筆記帳？") != QMessageBox::Yes)
            return;

        if (!account.removeAt(idx)) return;

        if (!account.saveToFile(currentDate)) {
            QMessageBox::warning(this, "存檔失敗", "刪除後無法寫入檔案 data/...");
            return;
        }

        refreshDayList(currentDate);
        refreshCalendarMarks();
        refreshMonthSummary(currentDate);
        checkBudgetWarning(currentDate);
    });

    stack->addWidget(accPage);

    // =========================
    // Page 1：待辦
    // =========================
    auto *todoPage = new QWidget(panel);
    auto *tv = new QVBoxLayout(todoPage);
    tv->setContentsMargins(0,0,0,0);
    tv->setSpacing(8);

    todoList = new QListWidget(todoPage);
    todoList->setObjectName("todoList");
    tv->addWidget(todoList);

    // ✅ 勾選完成（點一下打勾/取消），並立刻存檔
    connect(todoList, &QListWidget::itemChanged, this, [=](QListWidgetItem *it){
        if (!it) return;
        int idx = it->data(Qt::UserRole).toInt();
        if (idx < 0 || idx >= todoDone.size()) return;

        todoDone[idx] = (it->checkState() == Qt::Checked);
        saveTodosToFile(currentDate);
    });

    // ✅ 右鍵刪除 Todo
    todoList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(todoList, &QListWidget::customContextMenuRequested, this, [=](const QPoint &pos){
        auto *it = todoList->itemAt(pos);
        if (!it) return;

        int idx = it->data(Qt::UserRole).toInt();

        QMenu menu;
        QAction *del = menu.addAction("刪除這個待辦");
        QAction *chosen = menu.exec(todoList->viewport()->mapToGlobal(pos));
        if (chosen != del) return;

        if (QMessageBox::question(this, "刪除", "確定刪除這個待辦？") != QMessageBox::Yes)
            return;

        if (idx < 0 || idx >= todos.size()) return;
        todos.removeAt(idx);
        if (idx < todoDone.size()) todoDone.removeAt(idx);

        if (!saveTodosToFile(currentDate)) {
            QMessageBox::warning(this, "存檔失敗", "待辦刪除後無法寫入檔案 data/...");
            return;
        }

        refreshTodoList(currentDate);
        refreshCalendarMarks();
    });

    stack->addWidget(todoPage);

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

    // ✅ 設定預算（寫進當天 json）
    connect(btnBook, &QToolButton::clicked, this, [=]{
        // 1) 先切回「記帳頁」
        if (stack) stack->setCurrentIndex(0);
        refreshDayList(currentDate);

        // 2) 選單：設定 / 重設
        QMenu menu;
        QAction *actSet   = menu.addAction("設定本月預算");
        QAction *actReset = menu.addAction("重設本月預算（清空）");

        QAction *act = menu.exec(btnBook->mapToGlobal(QPoint(btnBook->width()/2, btnBook->height())));
        if (!act) return;

        if (act == actSet) {
            bool ok = false;
            double b = QInputDialog::getDouble(
                this,
                "設定預算",
                "本月預算：",
                account.getMonthlyBudget(),
                0, 1e9, 0,
                &ok
                );
            if (!ok) return;

            account.setMonthlyBudget(b);

            if (!account.saveToFile(currentDate)) {
                QMessageBox::warning(this, "存檔失敗", "預算無法寫入檔案 data/...");
                return;
            }

            refreshMonthSummary(currentDate);
            checkBudgetWarning(currentDate);
            return;
        }

        if (act == actReset) {
            if (QMessageBox::question(this, "重設預算", "確定要清空本月預算？") != QMessageBox::Yes)
                return;

            account.setMonthlyBudget(0);

            if (!account.saveToFile(currentDate)) {
                QMessageBox::warning(this, "存檔失敗", "預算無法寫入檔案 data/...");
                return;
            }

            refreshMonthSummary(currentDate);
            // 清空後不需要 warning
            return;
        }
    });

    // ✅ 新增（記帳/待辦）
    connect(btnPlus, &QToolButton::clicked, this, [=]{
        QDate d = cal->chosenDate();

        currentDate = d;
        account.loadFromFile(d);
        loadTodosFromFile(d);

        AddEntryDialog dlg(d, this);

        connect(&dlg, &AddEntryDialog::savedExpenseIncome, this, [=](const AccountItem& item){
            account.addItem(item);

            if (!account.saveToFile(d)) {
                QMessageBox::warning(this, "存檔失敗", "無法寫入 data/ 資料夾（權限或路徑問題）。");
                return;
            }

            refreshDayList(d);
            refreshCalendarMarks();
            refreshMonthSummary(d);
            checkBudgetWarning(d);

            if (stack) stack->setCurrentIndex(0); // 回到記帳頁
        });

        connect(&dlg, &AddEntryDialog::savedTodo, this, [=](const Todo& td){
            todos.push_back(td);
            todoDone.push_back(false);

            if (!saveTodosToFile(d)) {
                QMessageBox::warning(this, "存檔失敗", "待辦無法寫入 data/...");
                return;
            }

            refreshTodoList(d);
            refreshCalendarMarks();

            if (stack) stack->setCurrentIndex(1); // 切去待辦頁看到新增結果
        });

        dlg.exec();
    });

    // ✅ 待辦事項：切換到待辦頁
    connect(btnTodo, &QToolButton::clicked, this, [=]{
        if (!stack) return;
        stack->setCurrentIndex(1);
        refreshTodoList(currentDate);
    });

    return w;
}

void MainWindow::refreshDayList(const QDate&) {
    if (!list || !sumLabel) return;

    list->clear();

    double sumExpense = account.dailyExpense();

    int idx = 0;
    for (const auto &item : account.getItems()) {
        QString sign = (item.type == "income") ? "收入" : "支出";

        auto *it = new QListWidgetItem(QString("%1\n%2  %3")
                                           .arg(item.category)
                                           .arg(sign)
                                           .arg(item.amount));
        it->setData(Qt::UserRole, idx);
        list->addItem(it);
        idx++;
    }

    sumLabel->setText(QString("支出:%1").arg(sumExpense));
}

// ✅ Todo：更新清單顯示（含勾選完成）
void MainWindow::refreshTodoList(const QDate&) {
    if (!todoList) return;

    todoList->blockSignals(true);  // 避免重建時 itemChanged 亂觸發
    todoList->clear();

    int idx = 0;
    for (const auto& td : todos) {
        QString timeInfo = td.allDay
                               ? "全天"
                               : QString("%1-%2")
                                     .arg(td.start.time().toString("hh:mm"))
                                     .arg(td.end.time().toString("hh:mm"));

        auto *it = new QListWidgetItem(QString("%1\n%2").arg(td.title).arg(timeInfo));
        it->setData(Qt::UserRole, idx);

        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        bool done = (idx < todoDone.size()) ? todoDone[idx] : false;
        it->setCheckState(done ? Qt::Checked : Qt::Unchecked);

        todoList->addItem(it);
        idx++;
    }

    todoList->blockSignals(false);
}

// ✅ 行事曆白點：記帳檔 or Todo 檔，有任一個就標記
void MainWindow::refreshCalendarMarks() {
    QSet<QDate> marks;

    QDate first(cal->yearShown(), cal->monthShown(), 1);
    int days = first.daysInMonth();

    for (int d = 1; d <= days; ++d) {
        QDate date(first.year(), first.month(), d);

        QString accPath = QString("data/%1.json").arg(date.toString("yyyy-MM-dd"));
        QString tdPath  = todoFilePath(date);

        if (QFile::exists(accPath) || QFile::exists(tdPath))
            marks.insert(date);
    }

    cal->setMarkedDates(marks);
}

void MainWindow::refreshMonthSummary(const QDate& d)
{
    if (!monthIncomeLabel || !monthExpenseLabel || !budgetLabel || !budgetBar) return;

    Account temp;
    double mIncome  = temp.monthlyIncome(d.year(), d.month());
    double mExpense = temp.monthlyExpense(d.year(), d.month());

    monthIncomeLabel->setText(QString("本月收入: %1").arg(mIncome));
    monthExpenseLabel->setText(QString("本月支出: %1").arg(mExpense));

    double budget = account.getMonthlyBudget();
    if (budget <= 0) {
        budgetLabel->setText("預算: 未設定");
        budgetBar->setEnabled(false);
        budgetBar->setValue(0);
        return;
    }

    budgetBar->setEnabled(true);
    budgetLabel->setText(QString("預算: %1（已用 %2）").arg(budget).arg(mExpense));

    int pct = (budget > 0) ? int((mExpense / budget) * 100.0) : 0;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    budgetBar->setValue(pct);
}

void MainWindow::checkBudgetWarning(const QDate& d) {
    double budget = account.getMonthlyBudget();
    if (budget <= 0) return;

    Account temp;
    double monthExpense = temp.monthlyExpense(d.year(), d.month());

    if (monthExpense >= budget * 0.8) {
        QMessageBox::warning(this, "預算提醒",
                             QString("本月支出已達 %1 / %2（80%%）")
                                 .arg(monthExpense)
                                 .arg(budget));
    }
}

// ====== ✅ Todo 存檔/讀檔 ======
bool MainWindow::loadTodosFromFile(const QDate& d) {
    todos.clear();
    todoDone.clear();

    QFile f(todoFilePath(d));
    if (!f.open(QIODevice::ReadOnly)) {
        return false; // 沒檔案也算正常
    }

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    QJsonObject root = doc.object();
    QJsonArray arr = root["todos"].toArray();

    for (const auto& v : arr) {
        QJsonObject o = v.toObject();

        Todo td;
        td.title = o["title"].toString();
        td.allDay = o["allDay"].toBool(true);
        td.start = QDateTime::fromString(o["start"].toString(), Qt::ISODate);
        td.end   = QDateTime::fromString(o["end"].toString(), Qt::ISODate);

        if (!td.start.isValid()) td.start = QDateTime(d, QTime(9,0));
        if (!td.end.isValid())   td.end   = QDateTime(d, QTime(10,0));

        todos.push_back(td);
        todoDone.push_back(o["done"].toBool(false));
    }

    return true;
}

bool MainWindow::saveTodosToFile(const QDate& d) const {
    QJsonArray arr;

    for (int i = 0; i < todos.size(); ++i) {
        const auto& td = todos[i];

        QJsonObject o;
        o["title"] = td.title;
        o["allDay"] = td.allDay;
        o["start"] = td.start.toString(Qt::ISODate);
        o["end"]   = td.end.toString(Qt::ISODate);
        o["done"]  = (i < todoDone.size()) ? todoDone[i] : false;

        arr.append(o);
    }

    QJsonObject root;
    root["todos"] = arr;

    QFile f(todoFilePath(d));
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson());
    f.close();
    return true;
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
        QFrame#monthSummary { background: %3; border: 1px solid #1E1E1E; border-radius: 14px; }

        QProgressBar { border: 1px solid #2A2A2A; border-radius: 8px; padding: 2px; background: #121212; min-height: 12px; }
        QProgressBar::chunk { background: #EDEDED; border-radius: 8px; }

        QLabel#sumLabel { color: %2; font-size: 14px; padding: 6px 2px; }

        QListWidget { background: transparent; border: none; }
        QListWidget::item { padding: 10px; border-bottom: 1px solid #1E1E1E; color: %2; }
    )").arg(BG.name(), TEXT.name(), PANEL.name()));
}
