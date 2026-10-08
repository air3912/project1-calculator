#include "calculator_window.hpp"
#include "fitted_result_label.hpp"
#include "video_canvas.hpp"
#include "usage_notes.hpp"
#include "calculator/error.hpp"
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFrame>
#include <QFile>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QUrl>
#include <QVideoSink>
#include <QVBoxLayout>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace {

void identify(QWidget* widget, const char* role, const char* name = nullptr) {
    widget->setProperty("role", role);
    widget->setObjectName(QString::fromLatin1(name ? name : role));
}

QLabel* label(const QString& text, const char* role, const char* name = nullptr) {
    auto* result = new QLabel(text);
    identify(result, role, name);
    result->setTextFormat(Qt::PlainText);
    return result;
}

QPushButton* button(const QString& text, const char* role = "secondary", const char* name = nullptr) {
    auto* result = new QPushButton(text);
    identify(result, role, name);
    result->setCursor(Qt::PointingHandCursor);
    return result;
}

QFrame* card(QVBoxLayout*& layout, const char* role = "card") {
    auto* frame = new QFrame;
    identify(frame, role);
    layout = new QVBoxLayout(frame);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(12);
    return frame;
}

QLineEdit* input(const char* name, const char* role = "polynomialInput") {
    auto* result = new QLineEdit;
    identify(result, role, name);
    result->setMaxLength(4096);
    result->setMinimumHeight(46);
    result->setProperty("invalid", false);
    return result;
}

void invalidAppearance(QLineEdit* editor, bool invalid) {
    editor->setProperty("invalid", invalid);
    editor->style()->unpolish(editor);
    editor->style()->polish(editor);
    editor->update();
}

QString normalized(const QString& text) {
    QString result = text;
    result.replace(QChar(0x00d7), '*');
    result.replace(QChar(0x00f7), '/');
    result.replace(QChar(0x2212), '-');
    return result;
}

QString errorMessage(const calculator::Error& error, QLineEdit* editor = nullptr) {
    QString text = QString::fromUtf8(error.what());
    if (editor && error.position() != calculator::Error::noPosition) {
        const auto prefix = normalized(editor->text()).toUtf8().left(static_cast<qsizetype>(error.position()));
        const auto character = QString::fromUtf8(prefix).size() + 1;
        text += QStringLiteral("（第 %1 个字符）").arg(character);
    }
    return text;
}

void focusError(QLineEdit* editor, const calculator::Error& error) {
    invalidAppearance(editor, true);
    editor->setFocus();
    if (error.position() == calculator::Error::noPosition) return;
    const QByteArray utf8 = normalized(editor->text()).toUtf8();
    // Backend offsets are bytes; QLineEdit selection offsets are UTF-16 units.
    const qsizetype bytes = static_cast<qsizetype>(error.position());
    const qsizetype count = QString::fromUtf8(utf8.left(bytes)).size();
    const int offset = static_cast<int>(count);
    if (offset < editor->text().size()) editor->setSelection(offset, 1);
    else editor->setCursorPosition(static_cast<int>(editor->text().size()));
}

void configureTable(QTableWidget* table) {
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(38);
    table->horizontalHeader()->setHighlightSections(false);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

long double parseReal(const QString& text) {
    const QByteArray data = text.trimmed().toUtf8();
    char* end = nullptr;
    errno = 0;
    const long double result = std::strtold(data.constData(), &end);
    if (end == data.constData() || *end != '\0' || errno == ERANGE || !std::isfinite(result))
        throw calculator::Error(calculator::ErrorCode::InvalidInput, "x 必须是可表示的有限实数");
    return result;
}

QString formatReal(long double number) {
    std::ostringstream output;
    output << std::setprecision(std::numeric_limits<long double>::max_digits10) << number;
    return QString::fromStdString(output.str());
}

} // namespace

CalculatorWindow::CalculatorWindow(QWidget* parent) : QMainWindow(parent) {
    // Explicit registration also keeps resources linked from the static GUI
    // library, so both the app and GUI tests can load bundled assets.
    Q_INIT_RESOURCE(resources);
    setWindowTitle(QStringLiteral("Project1 · 多项式与表达式计算器"));
    resize(1280, 840);
    setMinimumSize(1120, 780);
    auto* canvas = new QWidget;
    identify(canvas, "canvas");
    setCentralWidget(canvas);
    auto* root = new QHBoxLayout(canvas);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* sidebar = new QFrame;
    identify(sidebar, "sidebar");
    sidebar->setFixedWidth(188);
    auto* navigation = new QVBoxLayout(sidebar);
    navigation->setContentsMargins(18, 30, 18, 24);
    navigation->setSpacing(9);
    auto* mark = new QLabel;
    mark->setFixedSize(46, 46);
    QPixmap src(QStringLiteral(":/avatar.jpg"));
    if (!src.isNull()) {
        const int s = 92;
        QPixmap rounded(s, s);
        rounded.fill(Qt::transparent);
        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath clip;
        clip.addRoundedRect(0, 0, s, s, 28, 28);
        painter.setClipPath(clip);
        painter.drawPixmap(0, 0, s, s, src.scaled(s, s, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        painter.end();
        rounded.setDevicePixelRatio(2.0);
        mark->setPixmap(rounded);
    }
    mark->setAlignment(Qt::AlignCenter);
    navigation->addWidget(mark);
    navigation->addSpacing(7);
    navigation->addWidget(label(QStringLiteral("Project 01"), "brand"));
    navigation->addWidget(label(QStringLiteral("多项式与表达式计算器"), "muted"));
    navigation->addSpacing(34);
    navigation->addWidget(label(QStringLiteral("计算工作台"), "eyebrow"));
    navigation->addSpacing(6);
    expressionNavigation_ = button(QStringLiteral("01   表达式计算"), "navigation", "expressionNavigation");
    polynomialNavigation_ = button(QStringLiteral("02   多项式计算"), "navigation", "polynomialNavigation");
    comingSoonNavigation_ = button(QStringLiteral("03   敬请期待"), "navigation", "comingSoonNavigation");
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);
    expressionNavigation_->setCheckable(true);
    polynomialNavigation_->setCheckable(true);
    comingSoonNavigation_->setCheckable(true);
    group->addButton(expressionNavigation_);
    group->addButton(polynomialNavigation_);
    group->addButton(comingSoonNavigation_);
    navigation->addWidget(expressionNavigation_);
    navigation->addWidget(polynomialNavigation_);
    navigation->addWidget(comingSoonNavigation_);
    navigation->addStretch();
    root->addWidget(sidebar);

    auto* workspace = new QWidget;
    auto* content = new QVBoxLayout(workspace);
    content->setContentsMargins(28, 28, 28, 24);
    content->setSpacing(22);
    auto* header = new QHBoxLayout;
    header->setSpacing(20);
    auto* heading = new QVBoxLayout;
    heading->setSpacing(8);
    heading->addWidget(label(QStringLiteral("PROJECT 01 / CALCULATOR"), "eyebrow"));
    pageTitle_ = label("", "pageTitle");
    pageSubtitle_ = label("", "subtitle");
    heading->addWidget(pageTitle_);
    heading->addWidget(pageSubtitle_);
    header->addLayout(heading, 1);
    usageHeader_ = new QWidget;
    usageHeader_->setObjectName("usageHeader");
    usageHeader_->setFixedWidth(230);
    auto* usageLayout = new QVBoxLayout(usageHeader_);
    usageLayout->setContentsMargins(0, 26, 0, 0);
    usageLayout->setSpacing(6);
    usageLayout->addWidget(label(QStringLiteral("使用说明"), "sectionTitle", "usageTitle"));
    usageContent_ = label("", "muted", "usageContent");
    usageContent_->setWordWrap(true);
    usageContent_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    usageContent_->setFixedHeight(42);
    usageContent_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    usageLayout->addWidget(usageContent_);
    header->addWidget(usageHeader_, 0, Qt::AlignTop);
    usageImage_ = new QLabel;
    usageImage_->setObjectName("usageImage");
    usageImage_->setAccessibleName(QStringLiteral("使用说明配图"));
    usageImage_->setFixedSize(96, 96);
    usageImage_->setAlignment(Qt::AlignCenter);
    const QPixmap usagePicture(QStringLiteral(":/usage_image.jpg"));
    usageImage_->setPixmap(usagePicture.scaled(192, 192, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QPixmap usageThumbnail = usageImage_->pixmap();
    usageThumbnail.setDevicePixelRatio(2.0);
    usageImage_->setPixmap(usageThumbnail);
    header->addWidget(usageImage_, 0, Qt::AlignTop);
    pageBadge_ = label("", "badge");
    header->addWidget(pageBadge_, 0, Qt::AlignTop);
    content->addLayout(header);
    pages_ = new QStackedWidget;
    pages_->setObjectName("pages");
    pages_->addWidget(createExpressionPage());
    pages_->addWidget(createPolynomialPage());
    pages_->addWidget(createComingSoonPage());
    content->addWidget(pages_, 1);
    root->addWidget(workspace, 1);
    statusBar()->setSizeGripEnabled(false);
    statusBar()->showMessage(QStringLiteral("就绪   ·   支持键盘输入，按 Enter 计算"));
    auto* footer = label(QStringLiteral(""), "muted");
    statusBar()->addPermanentWidget(footer);
    connect(expressionNavigation_, &QPushButton::clicked, this, [this] { changePage(0); });
    connect(polynomialNavigation_, &QPushButton::clicked, this, [this] { changePage(1); });
    connect(comingSoonNavigation_, &QPushButton::clicked, this, [this] { changePage(2); });
    changePage(0);
    calculatePolynomial();
    calculateExpression();
}

void CalculatorWindow::changePage(int index) {
    pages_->setCurrentIndex(index);
    expressionNavigation_->setChecked(index == 0);
    polynomialNavigation_->setChecked(index == 1);
    comingSoonNavigation_->setChecked(index == 2);
    usageHeader_->setVisible(index != 2);
    usageImage_->setVisible(index != 2);
    pageBadge_->setVisible(index == 2);
    usageContent_->setText(QString::fromUtf8(index == 0 ? usage_notes::expression : usage_notes::polynomial));
    if (index == 2) {
        pageTitle_->setText(QStringLiteral("敬请期待"));
        pageSubtitle_->setText(QStringLiteral("更多功能，敬请期待～"));
        pageBadge_->setText(QStringLiteral("COMING SOON"));
        statusBar()->showMessage(QStringLiteral("敬请期待"));
        if (!comingSoonVideoSource_->isOpen()) {
            if (comingSoonVideoSource_->open(QIODevice::ReadOnly)) {
                comingSoonPlayer_->setSourceDevice(comingSoonVideoSource_, QUrl(QStringLiteral("qrc:/coming_soon.mov")));
            } else {
                comingSoonVideoNotice_->setText(QStringLiteral("视频资源暂时不可用"));
                comingSoonVideoNotice_->show();
                return;
            }
        }
        comingSoonPlayer_->play();
        return;
    }
    if (comingSoonPlayer_->playbackState() == QMediaPlayer::PlayingState)
        comingSoonPlayer_->pause();
    statusBar()->showMessage(QStringLiteral("就绪   ·   支持键盘输入，按 Enter 计算"));
    pageTitle_->setText(index == 0 ? QStringLiteral("表达式计算") : QStringLiteral("多项式计算"));
    pageSubtitle_->setText(index == 0
        ? QStringLiteral("输入一个表达式，查看结果与完整的演算过程～")
        : QStringLiteral("让稀疏多项式的加减、相乘与求导一目了然～"));
    pageBadge_->setText(index == 0 ? QStringLiteral("双栈 · 逐步演算") : QStringLiteral("一元稀疏多项式"));
}

QWidget* CalculatorWindow::createComingSoonPage() {
    auto* page = new QWidget;
    page->setObjectName("comingSoonPage");
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);
    auto* video = new VideoCanvas;
    video->setObjectName("comingSoonVideo");
    video->setAccessibleName(QStringLiteral("敬请期待循环动画"));
    layout->addWidget(video, 0, Qt::AlignLeft);
    comingSoonVideoNotice_ = label(QStringLiteral("正在加载视频…"), "muted", "comingSoonVideoNotice");
    comingSoonVideoNotice_->setAlignment(Qt::AlignLeft);
    layout->addWidget(comingSoonVideoNotice_, 0, Qt::AlignLeft);
    layout->addStretch(1);

    comingSoonPlayer_ = new QMediaPlayer(this);
    comingSoonPlayer_->setObjectName("comingSoonPlayer");
    comingSoonPlayer_->setLoops(QMediaPlayer::Infinite);
    // No audio output: the decorative animation always stays silent.
    comingSoonPlayer_->setAudioOutput(nullptr);
    auto* sink = new QVideoSink(comingSoonPlayer_);
    comingSoonPlayer_->setVideoSink(sink);
    comingSoonVideoSource_ = new QFile(QStringLiteral(":/coming_soon.mov"), comingSoonPlayer_);
    connect(sink, &QVideoSink::videoFrameChanged, video, [this, video](const QVideoFrame& frame) {
        video->setFrame(frame);
        if (video->hasFrame()) comingSoonVideoNotice_->hide();
    });
    connect(comingSoonPlayer_, &QMediaPlayer::errorOccurred, this,
        [this](QMediaPlayer::Error, const QString& details) {
            comingSoonVideoNotice_->setText(QStringLiteral("视频暂时无法播放"));
            comingSoonVideoNotice_->setToolTip(details);
            comingSoonVideoNotice_->show();
        });
    return page;
}

QWidget* CalculatorWindow::createExpressionPage() {
    auto* page = new QWidget;
    auto* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);
    auto* left = new QWidget;
    left->setMinimumWidth(330);
    left->setMaximumWidth(420);
    auto* controls = new QVBoxLayout(left);
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(12);

    QVBoxLayout* editorLayout;
    auto* editor = card(editorLayout);
    editorLayout->setSpacing(10);
    auto* editorHeading = new QHBoxLayout;
    editorHeading->addWidget(label(QStringLiteral("算术表达式"), "sectionTitle"));
    editorHeading->addStretch();
    auto* calculate = button(QStringLiteral("计算  ↵"), "primary", "expressionCalculate");
    calculate->setMinimumHeight(34);
    editorHeading->addWidget(calculate);
    editorLayout->addLayout(editorHeading);
    expressionInput_ = input("expressionInput", "expressionInput");
    expressionInput_->setAccessibleName(QStringLiteral("算术表达式输入"));
    expressionInput_->setPlaceholderText(QStringLiteral("输入算术表达式"));
    expressionInput_->setText(QStringLiteral("10 - 2 * 3"));
    editorLayout->addWidget(expressionInput_);
    editorLayout->addWidget(label(QStringLiteral("支持 +  −  ×  ÷  ^  与括号"), "muted"));
    expressionError_ = label("", "error", "expressionError");
    expressionError_->setWordWrap(true);
    expressionError_->setMinimumHeight(28);
    expressionError_->hide();
    editorLayout->addWidget(expressionError_);
    controls->addWidget(editor);

    QVBoxLayout* resultLayout;
    auto* result = card(resultLayout, "resultCard");
    resultLayout->setContentsMargins(20, 16, 20, 16);
    resultLayout->setSpacing(8);
    auto* resultHeading = new QHBoxLayout;
    resultHeading->addWidget(label(QStringLiteral("计算结果"), "resultCaption"));
    resultHeading->addStretch();
    expressionCopy_ = button(QStringLiteral("复制"), "sample", "expressionCopy");
    expressionCopy_->setEnabled(false);
    resultHeading->addWidget(expressionCopy_);
    resultLayout->addLayout(resultHeading);
    expressionResult_ = new FittedResultLabel;
    identify(expressionResult_, "expressionResult", "expressionResult");
    expressionResult_->setText(QStringLiteral("—"));
    expressionResult_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    expressionResult_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    resultLayout->addWidget(expressionResult_);
    expressionContext_ = label(QStringLiteral("输入表达式后开始计算"), "resultContext");
    expressionContext_->setWordWrap(true);
    resultLayout->addWidget(expressionContext_);
    controls->addWidget(result);

    auto* keypad = new QGridLayout;
    keypad->setSpacing(8);
    const char* keys[4][5] = {
        {"AC", "⌫", "(", ")", "^"},
        {"7", "8", "9", "÷", "×"},
        {"4", "5", "6", "−", "+"},
        {"1", "2", "3", "0", "="}
    };
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 5; ++column) {
            const QString key = QString::fromUtf8(keys[row][column]);
            const char* role = key == "=" ? "equals" : column >= 3 ? "operatorKey" : "key";
            auto* keyButton = button(key, role);
            keyButton->setObjectName("key_" + key);
            keyButton->setMinimumHeight(44);
            keyButton->setFocusPolicy(Qt::NoFocus);
            keypad->addWidget(keyButton, row, column);
            connect(keyButton, &QPushButton::clicked, this, [this, key] {
                if (key == "AC") expressionInput_->clear();
                else if (key == QStringLiteral("⌫")) expressionInput_->backspace();
                else if (key == "=") calculateExpression();
                else expressionInput_->insert(normalized(key));
                expressionInput_->setFocus();
            });
        }
    }
    controls->addLayout(keypad);
    controls->addStretch();
    layout->addWidget(left, 4);

    QVBoxLayout* traceLayout;
    auto* trace = card(traceLayout);
    trace->setMinimumWidth(450);
    auto* traceHeading = new QHBoxLayout;
    traceHeading->addWidget(label(QStringLiteral("演算过程"), "sectionTitle"));
    traceHeading->addStretch();
    traceEnabled_ = new QCheckBox(QStringLiteral("记录过程"));
    traceEnabled_->setObjectName("traceEnabled");
    traceEnabled_->setChecked(true);
    traceHeading->addWidget(traceEnabled_);
    traceLayout->addLayout(traceHeading);
    traceCount_ = label("", "muted", "traceCount");
    traceLayout->addWidget(traceCount_);

    traceTable_ = new QTableWidget(0, 3);
    traceTable_->setObjectName("traceTable");
    traceTable_->setHorizontalHeaderLabels({QStringLiteral("步骤"), QStringLiteral("当前输入"), QStringLiteral("操作")});
    configureTable(traceTable_);
    traceTable_->setColumnWidth(0, 48);
    traceTable_->setColumnWidth(1, 80);
    traceTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    traceTable_->setMinimumHeight(180);
    traceLayout->addWidget(traceTable_, 1);

    auto* snapshots = new QHBoxLayout;
    for (int i = 0; i < 2; ++i) {
        QVBoxLayout* stackLayout;
        auto* stackCard = card(stackLayout, "stackCard");
        stackLayout->setContentsMargins(14, 12, 14, 12);
        stackLayout->setSpacing(8);
        stackLayout->addWidget(label(i == 0 ? QStringLiteral("运算符栈") : QStringLiteral("运算数栈"), "fieldLabel"));
        auto* value = label(QStringLiteral("[]"), "stackValue", i == 0 ? "operatorSnapshot" : "operandSnapshot");
        value->setWordWrap(true);
        value->setMinimumHeight(26);
        value->setMaximumHeight(90);
        value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (i == 0) operatorSnapshot_ = value;
        else operandSnapshot_ = value;
        stackLayout->addWidget(value);
        snapshots->addWidget(stackCard, 1);
    }
    traceLayout->addLayout(snapshots);
    auto* traceControls = new QHBoxLayout;
    tracePosition_ = label(QStringLiteral("栈底 → 栈顶"), "muted", "tracePosition");
    traceControls->addWidget(tracePosition_, 1);
    previousStep_ = button(QStringLiteral("上一步"), "secondary", "previousStep");
    nextStep_ = button(QStringLiteral("下一步"), "secondary", "nextStep");
    traceControls->addWidget(previousStep_);
    traceControls->addWidget(nextStep_);
    traceLayout->addLayout(traceControls);
    layout->addWidget(trace, 6);

    connect(calculate, &QPushButton::clicked, this, [this] { calculateExpression(); });
    connect(expressionInput_, &QLineEdit::returnPressed, this, [this] { calculateExpression(); });
    connect(expressionInput_, &QLineEdit::textChanged, this, [this] { invalidateExpression(); });
    connect(expressionCopy_, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(expressionResult_->text());
        statusBar()->showMessage(QStringLiteral("已复制计算结果"), 3000);
    });
    connect(traceEnabled_, &QCheckBox::toggled, this, [this] { calculateExpression(); });
    connect(traceTable_, &QTableWidget::currentCellChanged, this,
        [this](int row, int, int, int) { selectTraceStep(row); });
    connect(previousStep_, &QPushButton::clicked, this, [this] {
        if (traceTable_->currentRow() > 0) traceTable_->setCurrentCell(traceTable_->currentRow() - 1, 0);
    });
    connect(nextStep_, &QPushButton::clicked, this, [this] {
        const int next = traceTable_->currentRow() + 1;
        if (next < traceTable_->rowCount()) traceTable_->setCurrentCell(next, 0);
    });
    return page;
}

void CalculatorWindow::invalidateExpression() {
    invalidAppearance(expressionInput_, false);
    expressionError_->clear();
    expressionError_->hide();
    expressionResult_->setText(QStringLiteral("—"));
    expressionContext_->setText(QStringLiteral("输入已更新，按 Enter 或点击计算"));
    expressionCopy_->setEnabled(false);
    traceTable_->setRowCount(0);
    traceCount_->setText(QStringLiteral("计算后，点击任一步骤查看两栈状态。"));
    selectTraceStep(-1);
}

void CalculatorWindow::calculateExpression() {
    invalidateExpression();
    const std::string expression = normalized(expressionInput_->text()).toStdString();
    traceTable_->setUpdatesEnabled(false);
    try {
        const auto result = calculator::evaluateExpression(expression,
            traceEnabled_->isChecked() ? receiveTrace : nullptr, this);
        expressionResult_->setText(QString::number(static_cast<qlonglong>(result)));
        expressionContext_->setText(QStringLiteral("计算完成 · 整数除法向零截断"));
        expressionCopy_->setEnabled(true);
        traceCount_->setText(traceEnabled_->isChecked()
            ? QStringLiteral("共 %1 步 · 选择一行，查看该步完成后的两栈状态。 ").arg(traceTable_->rowCount())
            : QStringLiteral("过程记录已关闭；勾选「记录过程」即可查看。"));
        statusBar()->showMessage(QStringLiteral("表达式计算完成"), 3000);
    } catch (const calculator::Error& error) {
        expressionError_->setText(errorMessage(error, expressionInput_));
        expressionError_->show();
        expressionContext_->setText(QStringLiteral("请检查输入后重新计算"));
        traceCount_->setText(QStringLiteral("计算中断 · 已保留错误发生前的 %1 步记录。 ").arg(traceTable_->rowCount()));
        focusError(expressionInput_, error);
        statusBar()->showMessage(QStringLiteral("表达式计算未完成"), 3000);
    } catch (const std::exception& error) {
        expressionError_->setText(QStringLiteral("运行失败：") + QString::fromUtf8(error.what()));
        expressionError_->show();
    }
    traceTable_->setUpdatesEnabled(true);
    if (traceTable_->rowCount() > 0) traceTable_->setCurrentCell(0, 0);
    else selectTraceStep(-1);
}

void CalculatorWindow::receiveTrace(const calculator::TraceStep& step, void* context) {
    static_cast<CalculatorWindow*>(context)->appendTrace(step);
}

void CalculatorWindow::appendTrace(const calculator::TraceStep& step) {
    const int row = traceTable_->rowCount();
    traceTable_->insertRow(row);
    auto* index = new QTableWidgetItem(QString::number(row + 1));
    index->setTextAlignment(Qt::AlignCenter);
    index->setData(Qt::UserRole, QString::fromStdString(step.operatorStack));
    index->setData(Qt::UserRole + 1, QString::fromStdString(step.operandStack));
    index->setData(Qt::UserRole + 2, static_cast<qulonglong>(step.position));
    auto* token = new QTableWidgetItem(step.inputToken.empty()
        ? QStringLiteral("—") : QString::fromStdString(step.inputToken));
    token->setTextAlignment(Qt::AlignCenter);
    token->setToolTip(token->text());
    auto* action = new QTableWidgetItem(QString::fromStdString(step.action));
    action->setToolTip(action->text() + QStringLiteral("\n运算符栈：") + QString::fromStdString(step.operatorStack)
        + QStringLiteral("\n运算数栈：") + QString::fromStdString(step.operandStack));
    traceTable_->setItem(row, 0, index);
    traceTable_->setItem(row, 1, token);
    traceTable_->setItem(row, 2, action);
}

void CalculatorWindow::selectTraceStep(int row) {
    const bool valid = row >= 0 && row < traceTable_->rowCount() && traceTable_->item(row, 0);
    previousStep_->setEnabled(valid && row > 0);
    nextStep_->setEnabled(valid && row + 1 < traceTable_->rowCount());
    if (!valid) {
        operatorSnapshot_->setText(QStringLiteral("[]"));
        operandSnapshot_->setText(QStringLiteral("[]"));
        tracePosition_->setText(QStringLiteral("栈底 → 栈顶"));
        return;
    }
    const auto* item = traceTable_->item(row, 0);
    const QString operators = item->data(Qt::UserRole).toString();
    const QString operands = item->data(Qt::UserRole + 1).toString();
    operatorSnapshot_->setText(operators);
    operatorSnapshot_->setToolTip(operators);
    operandSnapshot_->setText(operands);
    operandSnapshot_->setToolTip(operands);
    tracePosition_->setText(QStringLiteral("第 %1 / %2 步 · 栈底 → 栈顶").arg(row + 1).arg(traceTable_->rowCount()));
}

QWidget* CalculatorWindow::createPolynomialPage() {
    auto* page = new QWidget;
    auto* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);
    auto* left = new QWidget;
    left->setMinimumWidth(370);
    auto* controls = new QVBoxLayout(left);
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(14);

    QVBoxLayout* operationLayout;
    auto* operationCard = card(operationLayout);
    auto* operationHeading = new QHBoxLayout;
    operationHeading->addWidget(label(QStringLiteral("选择运算"), "sectionTitle"));
    operationHeading->addStretch();
    swapPolynomials_ = button(QStringLiteral("交换 A / B"), "sample", "swapPolynomials");
    operationHeading->addWidget(swapPolynomials_);
    operationLayout->addLayout(operationHeading);
    polynomialOperation_ = new QComboBox;
    polynomialOperation_->setObjectName("polynomialOperation");
    polynomialOperation_->addItems({QStringLiteral("加法     A + B"), QStringLiteral("减法     A − B"),
        QStringLiteral("乘法     A × B"), QStringLiteral("求导     A′"), QStringLiteral("求值     A(x)")});
    operationLayout->addWidget(polynomialOperation_);
    controls->addWidget(operationCard);

    for (int i = 0; i < 2; ++i) {
        QVBoxLayout* polynomialLayout;
        auto* polynomialCard = card(polynomialLayout);
        polynomialLayout->addWidget(label(i == 0 ? QStringLiteral("多项式 A") : QStringLiteral("多项式 B"), "sectionTitle"));
        auto* editor = input(i == 0 ? "polynomialA" : "polynomialB");
        editor->setAccessibleName(i == 0 ? QStringLiteral("多项式 A 的整数序列") : QStringLiteral("多项式 B 的整数序列"));
        editor->setPlaceholderText(QStringLiteral("n c1 e1 ... cn en"));
        editor->setText(i == 0 ? QStringLiteral("2 2 3 5 1") : QStringLiteral("2 3 3 4 0"));
        auto* preview = label("", "preview", i == 0 ? "previewA" : "previewB");
        preview->setWordWrap(true);
        preview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        polynomialLayout->addWidget(editor);
        polynomialLayout->addWidget(preview);
        if (i == 0) { polynomialA_ = editor; previewA_ = preview; }
        else { polynomialB_ = editor; previewB_ = preview; polynomialBCard_ = polynomialCard; }
        controls->addWidget(polynomialCard);
    }

    xField_ = new QWidget;
    auto* xLayout = new QHBoxLayout(xField_);
    xLayout->setContentsMargins(0, 0, 0, 0);
    xLayout->addWidget(label(QStringLiteral("代入 x ="), "fieldLabel"));
    polynomialX_ = input("polynomialX", "realInput");
    polynomialX_->setText(QStringLiteral("2"));
    polynomialX_->setPlaceholderText(QStringLiteral("输入实数"));
    xLayout->addWidget(polynomialX_, 1);
    xField_->hide();
    controls->addWidget(xField_);
    polynomialError_ = label("", "error", "polynomialError");
    polynomialError_->setWordWrap(true);
    polynomialError_->setMinimumHeight(32);
    controls->addWidget(polynomialError_);
    auto* calculate = button(QStringLiteral("开始计算    ↵"), "primary", "polynomialCalculate");
    calculate->setMinimumHeight(46);
    controls->addWidget(calculate);
    controls->addStretch();
    layout->addWidget(left, 5);

    auto* right = new QWidget;
    right->setMinimumWidth(410);
    auto* results = new QVBoxLayout(right);
    results->setContentsMargins(0, 0, 0, 0);
    results->setSpacing(14);
    QVBoxLayout* resultLayout;
    auto* result = card(resultLayout, "resultCard");
    result->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    resultLayout->setContentsMargins(18, 14, 18, 14);
    resultLayout->setSpacing(8);
    auto* heading = new QHBoxLayout;
    heading->addWidget(label(QStringLiteral("计算结果"), "resultCaption"));
    heading->addStretch();
    polynomialCopy_ = button(QStringLiteral("复制结果"), "sample", "polynomialCopy");
    heading->addWidget(polynomialCopy_);
    resultLayout->addLayout(heading);
    polynomialResult_ = label(QStringLiteral("—"), "polynomialResult", "polynomialResult");
    polynomialResult_->setWordWrap(true);
    polynomialResult_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    polynomialResult_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    polynomialResult_->setMinimumHeight(42);
    resultLayout->addWidget(polynomialResult_);
    polynomialContext_ = label(QStringLiteral("选择运算并输入多项式"), "resultContext", "polynomialContext");
    polynomialContext_->setWordWrap(true);
    resultLayout->addWidget(polynomialContext_);
    polynomialSequence_ = input("polynomialSequence", "resultSequence");
    polynomialSequence_->setReadOnly(true);
    polynomialSequence_->setMinimumHeight(36);
    polynomialSequence_->setPlaceholderText(QStringLiteral("结果的整数序列"));
    resultLayout->addWidget(polynomialSequence_);
    useResult_ = button(QStringLiteral("将结果作为 A，继续计算"), "secondary", "usePolynomialResult");
    resultLayout->addWidget(useResult_);
    results->addWidget(result);

    QVBoxLayout* termsLayout;
    auto* termsCard = card(termsLayout);
    polynomialTermsCard_ = termsCard;
    termsLayout->setContentsMargins(18, 14, 18, 14);
    termsLayout->setSpacing(8);
    termsLayout->addWidget(label(QStringLiteral("结果项明细"), "sectionTitle"));
    polynomialTerms_ = new QTableWidget(0, 2);
    polynomialTerms_->setObjectName("polynomialTerms");
    polynomialTerms_->setHorizontalHeaderLabels({QStringLiteral("系数"), QStringLiteral("指数")});
    configureTable(polynomialTerms_);
    polynomialTerms_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    polynomialTerms_->setMinimumHeight(150);
    termsLayout->addWidget(polynomialTerms_, 1);
    results->addWidget(termsCard, 1);

    polynomialSample_ = new QComboBox(this);
    polynomialSample_->setObjectName("polynomialSample");
    polynomialSample_->addItems({QStringLiteral("试试题目中的参考示例…"), QStringLiteral("加法 · 合并同类项"),
        QStringLiteral("加法 · 完全抵消"), QStringLiteral("减法 · 得到 −x"), QStringLiteral("乘法 · 平方差"),
        QStringLiteral("求导 · 三次多项式"), QStringLiteral("求值 · 代入 x = 2")});
    polynomialSample_->hide();
    results->addStretch();
    layout->addWidget(right, 5);

    connect(calculate, &QPushButton::clicked, this, [this] { calculatePolynomial(); });
    connect(polynomialA_, &QLineEdit::returnPressed, this, [this] { calculatePolynomial(); });
    connect(polynomialB_, &QLineEdit::returnPressed, this, [this] { calculatePolynomial(); });
    connect(polynomialX_, &QLineEdit::returnPressed, this, [this] { calculatePolynomial(); });
    connect(polynomialA_, &QLineEdit::textChanged, this, [this] {
        invalidatePolynomial(); updatePolynomialPreview(polynomialA_, previewA_);
    });
    connect(polynomialB_, &QLineEdit::textChanged, this, [this] {
        invalidatePolynomial(); updatePolynomialPreview(polynomialB_, previewB_);
    });
    connect(polynomialX_, &QLineEdit::textChanged, this, [this] { invalidatePolynomial(); });
    connect(polynomialOperation_, &QComboBox::currentIndexChanged, this,
        [this](int index) { setPolynomialOperation(index); });
    connect(swapPolynomials_, &QPushButton::clicked, this, [this] {
        const QString oldA = polynomialA_->text();
        polynomialA_->setText(polynomialB_->text());
        polynomialB_->setText(oldA);
    });
    connect(polynomialCopy_, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(polynomialResult_->text());
        statusBar()->showMessage(QStringLiteral("已复制计算结果"), 3000);
    });
    connect(useResult_, &QPushButton::clicked, this, [this] {
        if (!hasPolynomialResult_) return;
        const QString sequence = QString::fromStdString(lastPolynomial_.toSequence());
        polynomialA_->setText(sequence);
        // setText does not emit textChanged if the value is unchanged.
        invalidatePolynomial();
        polynomialA_->setFocus();
    });
    connect(polynomialSample_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index == 0) return;
        switch (index) {
        case 1:
            polynomialOperation_->setCurrentIndex(0);
            polynomialA_->setText("2 2 3 5 1"); polynomialB_->setText("2 3 3 4 0"); break;
        case 2:
            polynomialOperation_->setCurrentIndex(0);
            polynomialA_->setText("2 3 4 2 2"); polynomialB_->setText("2 -3 4 -2 2"); break;
        case 3:
            polynomialOperation_->setCurrentIndex(1);
            polynomialA_->setText("1 1 2"); polynomialB_->setText("2 1 2 1 1"); break;
        case 4:
            polynomialOperation_->setCurrentIndex(2);
            polynomialA_->setText("2 1 1 1 0"); polynomialB_->setText("2 1 1 -1 0"); break;
        case 5:
            polynomialOperation_->setCurrentIndex(3);
            polynomialA_->setText("3 3 3 2 2 7 0"); break;
        case 6:
            polynomialOperation_->setCurrentIndex(4);
            polynomialA_->setText("1 2 2"); polynomialX_->setText("2"); break;
        }
        calculatePolynomial();
        // Restore the prompt so selecting the same example again reloads it.
        const QSignalBlocker blocker(polynomialSample_);
        polynomialSample_->setCurrentIndex(0);
    });
    updatePolynomialPreview(polynomialA_, previewA_);
    updatePolynomialPreview(polynomialB_, previewB_);
    invalidatePolynomial();
    return page;
}

void CalculatorWindow::updatePolynomialPreview(QLineEdit* editor, QLabel* preview) {
    try {
        const auto polynomial = calculator::Polynomial::fromSequence(normalized(editor->text()).toStdString());
        preview->setText(QString::fromStdString(polynomial.toString()));
        invalidAppearance(editor, false);
        preview->setToolTip(QStringLiteral("当前输入的代数式"));
    } catch (const calculator::Error& error) {
        preview->setText(QStringLiteral("等待有效的整数序列…"));
        preview->setToolTip(QString::fromUtf8(error.what()));
    } catch (const std::exception&) {
        preview->setText(QStringLiteral("暂时无法预览"));
    }
}

void CalculatorWindow::invalidatePolynomial() {
    hasPolynomialResult_ = false;
    polynomialResult_->setText(QStringLiteral("—"));
    polynomialContext_->setText(QStringLiteral("输入或运算已更新，点击开始计算"));
    polynomialSequence_->clear();
    polynomialSequence_->show();
    polynomialTerms_->setRowCount(0);
    polynomialError_->clear();
    polynomialCopy_->setEnabled(false);
    useResult_->setEnabled(false);
    invalidAppearance(polynomialA_, false);
    invalidAppearance(polynomialB_, false);
    invalidAppearance(polynomialX_, false);
}

void CalculatorWindow::setPolynomialOperation(int index) {
    polynomialBCard_->setVisible(index < 3);
    swapPolynomials_->setEnabled(index < 3);
    xField_->setVisible(index == 4);
    polynomialTermsCard_->setVisible(index != 4);
    invalidatePolynomial();
}

void CalculatorWindow::calculatePolynomial() {
    invalidatePolynomial();
    QLineEdit* activeInput = polynomialA_;
    try {
        const auto a = calculator::Polynomial::fromSequence(normalized(polynomialA_->text()).toStdString());
        const int operation = polynomialOperation_->currentIndex();
        calculator::Polynomial result;
        if (operation < 3) {
            activeInput = polynomialB_;
            const auto b = calculator::Polynomial::fromSequence(normalized(polynomialB_->text()).toStdString());
            activeInput = nullptr; // Later errors concern the operation/result.
            if (operation == 0) result = a + b;
            else if (operation == 1) result = a - b;
            else result = a * b;
        } else if (operation == 3) {
            activeInput = nullptr;
            result = a.derivative();
        } else {
            activeInput = polynomialX_;
            const long double x = parseReal(polynomialX_->text());
            activeInput = nullptr;
            polynomialResult_->setText(formatReal(a.evaluate(x)));
            polynomialContext_->setText(QStringLiteral("A(%1) · 实数求值结果").arg(polynomialX_->text().trimmed()));
            polynomialSequence_->hide();
            polynomialCopy_->setEnabled(true);
            statusBar()->showMessage(QStringLiteral("多项式求值完成"), 3000);
            return;
        }
        polynomialResult_->setText(QString::fromStdString(result.toString()));
        polynomialContext_->setText(QStringLiteral("%1 · %2 个非零项").arg(polynomialOperation_->currentText().trimmed())
            .arg(static_cast<qulonglong>(result.termCount())));
        polynomialSequence_->setText(QString::fromStdString(result.toSequence()));
        std::istringstream terms(result.toSequence());
        std::size_t count = 0;
        terms >> count;
        for (std::size_t i = 0; i < count; ++i) {
            calculator::Polynomial::Coefficient coefficient = 0;
            int exponent = 0;
            terms >> coefficient >> exponent;
            const int row = static_cast<int>(i);
            polynomialTerms_->insertRow(row);
            auto* coefficientItem = new QTableWidgetItem(QString::number(static_cast<qlonglong>(coefficient)));
            auto* exponentItem = new QTableWidgetItem(QString::number(exponent));
            coefficientItem->setTextAlignment(Qt::AlignCenter);
            exponentItem->setTextAlignment(Qt::AlignCenter);
            polynomialTerms_->setItem(row, 0, coefficientItem);
            polynomialTerms_->setItem(row, 1, exponentItem);
        }
        lastPolynomial_ = std::move(result);
        hasPolynomialResult_ = true;
        useResult_->setEnabled(true);
        polynomialCopy_->setEnabled(true);
        statusBar()->showMessage(QStringLiteral("多项式计算完成"), 3000);
    } catch (const calculator::Error& error) {
        polynomialError_->setText(errorMessage(error, activeInput));
        polynomialContext_->setText(QStringLiteral("请检查输入后重新计算"));
        if (activeInput) focusError(activeInput, error);
        statusBar()->showMessage(QStringLiteral("多项式计算未完成"), 3000);
    } catch (const std::exception& error) {
        polynomialError_->setText(QStringLiteral("运行失败：") + QString::fromUtf8(error.what()));
    }
}
