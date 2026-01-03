#include "addentrydialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTimeEdit>

static const QColor BG("#0B0B0B");
static const QColor PANEL("#141414");
static const QColor TEXT("#EDEDED");

AddEntryDialog::AddEntryDialog(const QDate &selectedDate, QWidget *parent)
    : QDialog(parent), date(selectedDate)
{
    setWindowTitle("Add Entry");
    setModal(true);
    setMinimumSize(380, 720);

    buildUi();
    applyStyle();
    updateDateLabel();
    switchPage(Expense);
}

void AddEntryDialog::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14,14,14,14);
    root->setSpacing(12);

    root->addWidget(buildTopBar());
    root->addWidget(buildSegment());

    pages = new QStackedWidget(this);
    pages->addWidget(buildExpenseIncomePage(false)); // Expense
    pages->addWidget(buildExpenseIncomePage(true));  // Income
    pages->addWidget(buildTodoPage());               // Todo
    root->addWidget(pages, 1);

    root->addWidget(buildKeypad());
}

QWidget* AddEntryDialog::buildTopBar() {
    auto *w = new QWidget(this);
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(0,0,0,0);

    btnClose = new QPushButton("✕", w);
    btnSave  = new QPushButton("儲存", w);

    btnClose->setFixedSize(44, 36);
    btnSave->setFixedSize(72, 36);

    dateLabel = new QLabel(w);
    dateLabel->setAlignment(Qt::AlignCenter);

    h->addWidget(btnClose);
    h->addWidget(dateLabel, 1);
    h->addWidget(btnSave);

    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnSave,  &QPushButton::clicked, this, &AddEntryDialog::onSave);

    return w;
}

QWidget* AddEntryDialog::buildSegment() {
    auto *w = new QWidget(this);
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(8);

    segExpense = new QPushButton("支出", w);
    segIncome  = new QPushButton("收入", w);
    segTodo    = new QPushButton("待辦事項", w);

    segExpense->setCheckable(true);
    segIncome->setCheckable(true);
    segTodo->setCheckable(true);

    segExpense->setAutoExclusive(true);
    segIncome->setAutoExclusive(true);
    segTodo->setAutoExclusive(true);

    segExpense->setChecked(true);

    h->addWidget(segExpense);
    h->addWidget(segIncome);
    h->addWidget(segTodo);

    connect(segExpense, &QPushButton::clicked, this, [=]{ switchPage(Expense); });
    connect(segIncome,  &QPushButton::clicked, this, [=]{ switchPage(Income); });
    connect(segTodo,    &QPushButton::clicked, this, [=]{ switchPage(TodoPage); });

    return w;
}

QWidget* AddEntryDialog::buildExpenseIncomePage(bool isIncome) {
    auto *panel = new QWidget(this);
    auto *v = new QVBoxLayout(panel);
    v->setContentsMargins(18,18,18,18);
    v->setSpacing(14);

    auto *title = new QLabel(isIncome ? "收入" : "支出", panel);
    title->setObjectName("pageTitle");
    v->addWidget(title);

    auto row1 = new QHBoxLayout();
    row1->addWidget(new QLabel("金額", panel));
    row1->addStretch(1);

    QLineEdit *amount = new QLineEdit(panel);
    amount->setPlaceholderText("金額輸入");
    amount->setAlignment(Qt::AlignRight);
    amount->setReadOnly(true);
    row1->addWidget(amount);
    v->addLayout(row1);

    auto row2 = new QHBoxLayout();
    row2->addWidget(new QLabel("類別", panel));
    row2->addStretch(1);

    QComboBox *cat = new QComboBox(panel);
    cat->addItems({"飲食","交通","購物","娛樂","日用必需品","醫療","投資","薪水","獎金","零用金","其他"});
    row2->addWidget(cat);
    v->addLayout(row2);

    if (!isIncome) {
        amountExpense = amount;
        categoryExpense = cat;
    } else {
        amountIncome = amount;
        categoryIncome = cat;
    }

    v->addStretch(1);
    return panel;
}

QWidget* AddEntryDialog::buildTodoPage() {
    auto *panel = new QWidget(this);
    auto *v = new QVBoxLayout(panel);
    v->setContentsMargins(18,18,18,18);
    v->setSpacing(14);

    auto *title = new QLabel("待辦事項", panel);
    title->setObjectName("pageTitle");
    v->addWidget(title);

    todoTitle = new QLineEdit(panel);
    todoTitle->setPlaceholderText("標題");
    v->addWidget(todoTitle);

    allDay = new QCheckBox("全天", panel);
    allDay->setChecked(true);
    v->addWidget(allDay);

    startDT = new QDateTimeEdit(QDateTime(date, QTime(9,0)), panel);
    endDT   = new QDateTimeEdit(QDateTime(date, QTime(10,0)), panel);
    startDT->setCalendarPopup(true);
    endDT->setCalendarPopup(true);

    auto rowS = new QHBoxLayout();
    rowS->addWidget(new QLabel("開始", panel));
    rowS->addStretch(1);
    rowS->addWidget(startDT);
    v->addLayout(rowS);

    auto rowE = new QHBoxLayout();
    rowE->addWidget(new QLabel("結束", panel));
    rowE->addStretch(1);
    rowE->addWidget(endDT);
    v->addLayout(rowE);

    auto updateEnabled = [=]{
        bool enable = !allDay->isChecked();
        startDT->setEnabled(enable);
        endDT->setEnabled(enable);
    };
    connect(allDay, &QCheckBox::toggled, this, [=]{ updateEnabled(); });
    updateEnabled();

    v->addStretch(1);
    return panel;
}

QWidget* AddEntryDialog::buildKeypad() {
    auto *w = new QWidget(this);
    auto *g = new QGridLayout(w);
    g->setContentsMargins(0,0,0,0);
    g->setSpacing(6);

    auto mk = [&](const QString& t){
        auto *b = new QPushButton(t, w);
        b->setFixedHeight(54);
        return b;
    };

    QString keys[4][3] = {
        {"7","8","9"},
        {"4","5","6"},
        {"1","2","3"},
        {".","0","⌫"}
    };

    for (int r=0;r<4;r++){
        for (int c=0;c<3;c++){
            auto *b = mk(keys[r][c]);
            g->addWidget(b, r, c);

            connect(b, &QPushButton::clicked, this, [=]{
                if (pages && pages->currentIndex() == TodoPage) return;

                QLineEdit *edit = currentAmountEdit();
                if (!edit) return;

                QString cur = edit->text();
                QString k = b->text();

                if (k == "⌫") {
                    if (!cur.isEmpty()) cur.chop(1);
                } else if (k == ".") {
                    // 暫不支援小數
                } else {
                    cur += k;
                }
                edit->setText(cur);
            });
        }
    }

    return w;
}

void AddEntryDialog::switchPage(Page p) {
    if (!pages) return;

    pages->setCurrentIndex(int(p));
    currentIsIncome = (p == Income);

    if (segExpense) segExpense->setChecked(p == Expense);
    if (segIncome)  segIncome->setChecked(p == Income);
    if (segTodo)    segTodo->setChecked(p == TodoPage);
}

void AddEntryDialog::updateDateLabel() {
    static const QStringList wk = {"週一","週二","週三","週四","週五","週六","週日"};
    QString w = wk[(date.dayOfWeek()-1) % 7];
    QString txt = QString("%1年%2月%3日 %4")
                      .arg(date.year()).arg(date.month()).arg(date.day()).arg(w);

    if (dateLabel) dateLabel->setText(txt);
}

QLineEdit* AddEntryDialog::currentAmountEdit() const {
    return currentIsIncome ? amountIncome : amountExpense;
}

QComboBox* AddEntryDialog::currentCategoryBox() const {
    return currentIsIncome ? categoryIncome : categoryExpense;
}

void AddEntryDialog::onSave() {
    // Todo
    if (pages && pages->currentIndex() == TodoPage) {
        Todo td;
        td.title = todoTitle ? todoTitle->text().trimmed() : "";
        if (td.title.isEmpty()) { reject(); return; }

        td.allDay = allDay ? allDay->isChecked() : true;
        td.start = startDT ? startDT->dateTime() : QDateTime(date, QTime(9,0));
        td.end   = endDT   ? endDT->dateTime()   : QDateTime(date, QTime(10,0));

        emit savedTodo(td);
        accept();
        return;
    }

    // 記帳
    QLineEdit *edit = currentAmountEdit();
    QComboBox *cat  = currentCategoryBox();
    if (!edit || !cat) { reject(); return; }

    int amount = edit->text().toInt();
    if (amount <= 0) { reject(); return; }

    AccountItem item;
    item.date = date;
    item.category = cat->currentText();
    item.amount = amount;
    item.type = currentIsIncome ? "income" : "expense";
    item.note = "";

    emit savedExpenseIncome(item);
    accept();
}

void AddEntryDialog::applyStyle() {
    setStyleSheet(QString(R"(
        QDialog { background: %1; color: %2; }

        QPushButton { border: 1px solid #2A2A2A; border-radius: 10px; padding: 6px 10px; background: #121212; color: %2; }
        QPushButton:hover { background: #1A1A1A; }
        QPushButton:checked { background: #2A2A2A; border-color: #3A3A3A; }

        QLabel#pageTitle { font-size: 22px; font-weight: 700; color: #9A9A9A; padding: 8px 0px; }

        QLineEdit { background: %3; border: 1px solid #2A2A2A; border-radius: 10px; padding: 10px; color: %2; min-width: 160px; }
        QComboBox { background: %3; border: 1px solid #2A2A2A; border-radius: 10px; padding: 8px; color: %2; }
        QCheckBox { spacing: 8px; }

        QDateTimeEdit { background: %3; border: 1px solid #2A2A2A; border-radius: 10px; padding: 8px; color: %2; }
    )").arg(BG.name(), TEXT.name(), PANEL.name()));
}
