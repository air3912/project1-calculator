#include "calculator_window.hpp"
#include "style.hpp"
#include "video_canvas.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVideoSink>
#include <QtTest>

class GuiTests : public QObject {
    Q_OBJECT

private:
    CalculatorWindow* window_ = nullptr;
    QString previousClipboard_;

    template <typename T>
    T* child(const char* name) const {
        T* result = window_->findChild<T*>(QString::fromUtf8(name));
        if (!result) qFatal("Missing GUI control: %s", name);
        return result;
    }

    void expression(const QString& text) {
        child<QLineEdit>("expressionInput")->setText(text);
        child<QPushButton>("expressionCalculate")->click();
    }

private slots:
    void initTestCase() {
        applyCalculatorStyle(*qobject_cast<QApplication*>(QApplication::instance()));
        previousClipboard_ = QApplication::clipboard()->text();
    }

    void init() {
        window_ = new CalculatorWindow;
        window_->show();
        QApplication::processEvents();
    }

    void cleanup() {
        delete window_;
        window_ = nullptr;
    }

    void cleanupTestCase() {
        QApplication::clipboard()->setText(previousClipboard_);
    }

    void initialResultsAndNavigation() {
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("4"));
        QCOMPARE(child<QLabel>("polynomialResult")->text(), QStringLiteral("5x^3 + 5x + 4"));
        QCOMPARE(child<QStackedWidget>("pages")->currentIndex(), 0);
        child<QPushButton>("polynomialNavigation")->click();
        QCOMPARE(child<QStackedWidget>("pages")->currentIndex(), 1);
        QVERIFY(child<QPushButton>("polynomialNavigation")->isChecked());
        child<QPushButton>("expressionNavigation")->click();
        QCOMPARE(child<QStackedWidget>("pages")->currentIndex(), 0);
    }

    void comingSoonNavigationPreservesCalculations() {
        expression("(2+3)*4");
        const int traceRows = child<QTableWidget>("traceTable")->rowCount();
        child<QLineEdit>("polynomialA")->setText("1 7 0");
        child<QComboBox>("polynomialOperation")->setCurrentIndex(3);
        child<QPushButton>("polynomialCalculate")->click();
        child<QPushButton>("comingSoonNavigation")->click();
        QCOMPARE(child<QStackedWidget>("pages")->count(), 3);
        QCOMPARE(child<QStackedWidget>("pages")->currentIndex(), 2);
        QVERIFY(child<QPushButton>("comingSoonNavigation")->isChecked());
        QVERIFY(!child<QPushButton>("expressionNavigation")->isChecked());
        QVERIFY(!child<QPushButton>("polynomialNavigation")->isChecked());
        auto* video = dynamic_cast<VideoCanvas*>(child<QWidget>("comingSoonVideo"));
        QVERIFY(video);
        QVERIFY(video->isVisible());
        auto* player = child<QMediaPlayer>("comingSoonPlayer");
        QVERIFY(player->audioOutput() == nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(video->hasFrame(), 5000);
        QCOMPARE(player->error(), QMediaPlayer::NoError);
        QTRY_COMPARE_WITH_TIMEOUT(player->playbackState(), QMediaPlayer::PlayingState, 5000);
        child<QPushButton>("expressionNavigation")->click();
        QTRY_COMPARE(player->playbackState(), QMediaPlayer::PausedState);
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("20"));
        QCOMPARE(child<QTableWidget>("traceTable")->rowCount(), traceRows);
        child<QPushButton>("polynomialNavigation")->click();
        QCOMPARE(child<QLineEdit>("polynomialA")->text(), QStringLiteral("1 7 0"));
        QCOMPARE(child<QLabel>("polynomialResult")->text(), QStringLiteral("0"));
    }

    void comingSoonVideoAnimatesLoopsAndResumes() {
        auto* player = child<QMediaPlayer>("comingSoonPlayer");
        QCOMPARE(player->playbackState(), QMediaPlayer::StoppedState);
        child<QPushButton>("comingSoonNavigation")->click();
        auto* video = dynamic_cast<VideoCanvas*>(child<QWidget>("comingSoonVideo"));
        QVERIFY(video);
        QTRY_VERIFY_WITH_TIMEOUT(video->hasFrame() && player->duration() > 0, 5000);
        const QImage first = player->videoSink()->videoFrame().toImage();
        QVERIFY(!first.isNull());
        QTest::qWait(150);
        const QImage next = player->videoSink()->videoFrame().toImage();
        QVERIFY(!next.isNull());
        QVERIFY(first != next); // Actual changing decoded frames, not a poster.
        player->setPosition(player->duration() - 80);
        QTRY_VERIFY_WITH_TIMEOUT(player->position() < player->duration() / 2, 2000);
        QCOMPARE(player->playbackState(), QMediaPlayer::PlayingState);
        child<QPushButton>("expressionNavigation")->click();
        QTRY_COMPARE(player->playbackState(), QMediaPlayer::PausedState);
        const qint64 pausedAt = player->position();
        QTest::qWait(80);
        QVERIFY(qAbs(player->position() - pausedAt) < 50);
        child<QPushButton>("comingSoonNavigation")->click();
        QTRY_COMPARE(player->playbackState(), QMediaPlayer::PlayingState);
        QVERIFY(child<QLabel>("comingSoonVideoNotice")->isHidden());
    }

    void keyboardAndUnicodeOperators() {
        auto* editor = child<QLineEdit>("expressionInput");
        editor->clear();
        editor->setFocus();
        QTest::keyClicks(editor, "3*(-2)+2^3");
        QTest::keyClick(editor, Qt::Key_Return);
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("2"));
        expression(QStringLiteral("10 − 2 × 3 + 8 ÷ 2"));
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("8"));
        child<QPushButton>("expressionCopy")->click();
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("8"));
    }

    void keypadEditingAndCalculation() {
        child<QPushButton>("key_AC")->click();
        child<QPushButton>("key_7")->click();
        child<QPushButton>("key_×")->click();
        child<QPushButton>("key_3")->click();
        QCOMPARE(child<QLineEdit>("expressionInput")->text(), QStringLiteral("7*3"));
        child<QPushButton>("key_⌫")->click();
        child<QPushButton>("key_2")->click();
        child<QPushButton>("key_=")->click();
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("14"));
    }

    void traceSelectionAndNavigation() {
        expression("10-2*3");
        auto* table = child<QTableWidget>("traceTable");
        QVERIFY(table->rowCount() > 6);
        QCOMPARE(child<QLabel>("operatorSnapshot")->text(), QStringLiteral("[#]"));
        QCOMPARE(child<QLabel>("operandSnapshot")->text(), QStringLiteral("[]"));
        QVERIFY(!child<QPushButton>("previousStep")->isEnabled());
        child<QPushButton>("nextStep")->click();
        QCOMPARE(child<QLabel>("operandSnapshot")->text(), QStringLiteral("[10]"));
        child<QPushButton>("previousStep")->click();
        QCOMPARE(table->currentRow(), 0);
        table->setCurrentCell(table->rowCount() - 1, 0);
        QCOMPARE(child<QLabel>("operatorSnapshot")->text(), QStringLiteral("[]"));
        QCOMPARE(child<QLabel>("operandSnapshot")->text(), QStringLiteral("[4]"));
        QVERIFY(!child<QPushButton>("nextStep")->isEnabled());
    }

    void traceToggleAndStaleResult() {
        child<QCheckBox>("traceEnabled")->setChecked(false);
        QCOMPARE(child<QTableWidget>("traceTable")->rowCount(), 0);
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("4"));
        child<QCheckBox>("traceEnabled")->setChecked(true);
        QVERIFY(child<QTableWidget>("traceTable")->rowCount() > 0);
        child<QLineEdit>("expressionInput")->setText("20/4");
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("—"));
        QCOMPARE(child<QTableWidget>("traceTable")->rowCount(), 0);
        QVERIFY(!child<QPushButton>("expressionCopy")->isEnabled());
    }

    void expressionErrorsAndRecovery() {
        expression("10/(3-3)");
        QVERIFY(child<QLabel>("expressionError")->text().contains(QStringLiteral("除数不能为 0")));
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("—"));
        QVERIFY(!child<QPushButton>("expressionCopy")->isEnabled());
        QVERIFY(child<QTableWidget>("traceTable")->rowCount() > 0);
        QCOMPARE(child<QLineEdit>("expressionInput")->selectedText(), QStringLiteral("/"));
        expression(QStringLiteral("2×3a"));
        QCOMPARE(child<QLineEdit>("expressionInput")->selectedText(), QStringLiteral("a"));
        expression(QStringLiteral("2+汉"));
        QCOMPARE(child<QLineEdit>("expressionInput")->selectedText(), QStringLiteral("汉"));
        expression("(2+3");
        QVERIFY(child<QLabel>("expressionError")->text().contains(QStringLiteral("左括号")));
        expression("2^63");
        QVERIFY(child<QLabel>("expressionError")->text().contains(QStringLiteral("64 位")));
        expression("2^-1");
        QVERIFY(child<QLabel>("expressionError")->text().contains(QStringLiteral("负指数")));
        expression("2+3");
        QVERIFY(child<QLabel>("expressionError")->text().isEmpty());
        QCOMPARE(child<QLabel>("expressionResult")->text(), QStringLiteral("5"));
    }

    void allPolynomialReferenceExamples() {
        child<QPushButton>("polynomialNavigation")->click();
        const char* expected[] = {"5x^3 + 5x + 4", "0", "-x", "x^2 - 1", "9x^2 + 4x", "8"};
        for (int i = 0; i < 6; ++i) {
            child<QComboBox>("polynomialSample")->setCurrentIndex(i + 1);
            QCOMPARE(child<QLabel>("polynomialResult")->text(), QString::fromLatin1(expected[i]));
            QVERIFY(child<QLabel>("polynomialError")->text().isEmpty());
        }
        QVERIFY(!child<QLineEdit>("polynomialB")->isVisible());
        QVERIFY(child<QLineEdit>("polynomialX")->isVisible());
        QVERIFY(!child<QPushButton>("usePolynomialResult")->isEnabled());
        QVERIFY(!child<QLineEdit>("polynomialSequence")->isVisible());
    }

    void polynomialTermsClipboardAndReuse() {
        child<QPushButton>("polynomialNavigation")->click();
        QCOMPARE(child<QLineEdit>("polynomialSequence")->text(), QStringLiteral("3 5 3 5 1 4 0"));
        auto* terms = child<QTableWidget>("polynomialTerms");
        QCOMPARE(terms->rowCount(), 3);
        QCOMPARE(terms->item(0, 0)->text(), QStringLiteral("5"));
        QCOMPARE(terms->item(0, 1)->text(), QStringLiteral("3"));
        child<QPushButton>("polynomialCopy")->click();
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("5x^3 + 5x + 4"));
        child<QPushButton>("usePolynomialResult")->click();
        QCOMPARE(child<QLineEdit>("polynomialA")->text(), QStringLiteral("3 5 3 5 1 4 0"));
        QCOMPARE(child<QLabel>("polynomialResult")->text(), QStringLiteral("—"));
        child<QPushButton>("polynomialCalculate")->click();
        QCOMPARE(child<QLabel>("polynomialResult")->text(), QStringLiteral("8x^3 + 5x + 8"));
    }

    void polynomialInputErrorsAndUnaryIsolation() {
        child<QPushButton>("polynomialNavigation")->click();
        child<QLineEdit>("polynomialB")->setText("2 1 0 2 1");
        child<QPushButton>("polynomialCalculate")->click();
        QVERIFY(child<QLabel>("polynomialError")->text().contains(QStringLiteral("严格降序")));
        QVERIFY(child<QLineEdit>("polynomialB")->property("invalid").toBool());
        QCOMPARE(child<QLabel>("polynomialResult")->text(), QStringLiteral("—"));
        child<QComboBox>("polynomialOperation")->setCurrentIndex(3);
        child<QPushButton>("polynomialCalculate")->click();
        QCOMPARE(child<QLabel>("polynomialResult")->text(), QStringLiteral("6x^2 + 5"));
        QVERIFY(child<QLabel>("polynomialError")->text().isEmpty());
        child<QLineEdit>("polynomialA")->setText("1 9223372036854775807 2");
        child<QPushButton>("polynomialCalculate")->click();
        QVERIFY(child<QLabel>("polynomialError")->text().contains(QStringLiteral("64 位")));
        QVERIFY(!child<QPushButton>("usePolynomialResult")->isEnabled());
    }

    void polynomialRealEvaluationAndSwap() {
        child<QPushButton>("polynomialNavigation")->click();
        const QString a = child<QLineEdit>("polynomialA")->text();
        const QString b = child<QLineEdit>("polynomialB")->text();
        child<QPushButton>("swapPolynomials")->click();
        QCOMPARE(child<QLineEdit>("polynomialA")->text(), b);
        QCOMPARE(child<QLineEdit>("polynomialB")->text(), a);
        child<QComboBox>("polynomialOperation")->setCurrentIndex(4);
        child<QLineEdit>("polynomialA")->setText("2 2 2 1 0");
        child<QLineEdit>("polynomialX")->setText("0.5");
        QTest::keyClick(child<QLineEdit>("polynomialX"), Qt::Key_Return);
        QCOMPARE(child<QLabel>("polynomialResult")->text(), QStringLiteral("1.5"));
        child<QLineEdit>("polynomialX")->setText("nan");
        child<QPushButton>("polynomialCalculate")->click();
        QVERIFY(child<QLabel>("polynomialError")->text().contains(QStringLiteral("有限实数")));
        QVERIFY(child<QLineEdit>("polynomialX")->property("invalid").toBool());
    }

    void captureForVisualReview() {
        const QString directory = QString::fromLocal8Bit(qgetenv("PROJECT1_GUI_SCREENSHOTS"));
        if (directory.isEmpty()) return;
        QVERIFY(QDir().mkpath(directory));
        expression("10 - 2 * 3");
        child<QTableWidget>("traceTable")->setCurrentCell(6, 0);
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/expression.png"));
        child<QPushButton>("polynomialNavigation")->click();
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/polynomial.png"));
        window_->resize(window_->minimumSize());
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/polynomial-minimum.png"));
        child<QPushButton>("expressionNavigation")->click();
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/expression-minimum.png"));
        expression("10/(3-3)");
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/expression-error.png"));
        expression("9223372036854775807");
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/expression-large-result.png"));
        child<QPushButton>("polynomialNavigation")->click();
        child<QComboBox>("polynomialSample")->setCurrentIndex(6);
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/polynomial-evaluation.png"));
        child<QPushButton>("comingSoonNavigation")->click();
        auto* video = dynamic_cast<VideoCanvas*>(child<QWidget>("comingSoonVideo"));
        QVERIFY(video);
        QTRY_VERIFY_WITH_TIMEOUT(video->hasFrame(), 5000);
        child<QMediaPlayer>("comingSoonPlayer")->pause();
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/coming-soon-minimum.png"));
        window_->resize(1280, 840);
        QApplication::processEvents();
        QVERIFY(window_->grab().save(directory + "/coming-soon.png"));
    }
};

QTEST_MAIN(GuiTests)
#include "test_gui.moc"
