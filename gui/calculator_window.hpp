#pragma once

#include "calculator/expression.hpp"
#include "calculator/polynomial.hpp"
#include <QMainWindow>
#include <QString>

class QCheckBox;
class QComboBox;
class QFrame;
class QFile;
class QLabel;
class QLineEdit;
class QMediaPlayer;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QWidget;
class FittedResultLabel;

class CalculatorWindow : public QMainWindow {
public:
    explicit CalculatorWindow(QWidget* parent = nullptr);

private:
    QWidget* createExpressionPage();
    QWidget* createPolynomialPage();
    QWidget* createComingSoonPage();
    void changePage(int index);
    void calculateExpression();
    void invalidateExpression();
    void appendTrace(const calculator::TraceStep& step);
    void selectTraceStep(int row);
    void updatePolynomialPreview(QLineEdit* input, QLabel* preview);
    void invalidatePolynomial();
    void calculatePolynomial();
    void setPolynomialOperation(int index);
    static void receiveTrace(const calculator::TraceStep& step, void* context);

    QStackedWidget* pages_ = nullptr;
    QPushButton* expressionNavigation_ = nullptr;
    QPushButton* polynomialNavigation_ = nullptr;
    QPushButton* comingSoonNavigation_ = nullptr;
    QMediaPlayer* comingSoonPlayer_ = nullptr;
    QFile* comingSoonVideoSource_ = nullptr;
    QLabel* comingSoonVideoNotice_ = nullptr;
    QLabel* pageTitle_ = nullptr;
    QLabel* pageSubtitle_ = nullptr;
    QLabel* pageBadge_ = nullptr;
    QWidget* usageHeader_ = nullptr;
    QLabel* usageContent_ = nullptr;
    QLabel* usageImage_ = nullptr;

    QLineEdit* expressionInput_ = nullptr;
    FittedResultLabel* expressionResult_ = nullptr;
    QLabel* expressionContext_ = nullptr;
    QLabel* expressionError_ = nullptr;
    QPushButton* expressionCopy_ = nullptr;
    QCheckBox* traceEnabled_ = nullptr;
    QTableWidget* traceTable_ = nullptr;
    QLabel* traceCount_ = nullptr;
    QLabel* tracePosition_ = nullptr;
    QLabel* operatorSnapshot_ = nullptr;
    QLabel* operandSnapshot_ = nullptr;
    QPushButton* previousStep_ = nullptr;
    QPushButton* nextStep_ = nullptr;

    QLineEdit* polynomialA_ = nullptr;
    QLineEdit* polynomialB_ = nullptr;
    QLineEdit* polynomialX_ = nullptr;
    QLabel* previewA_ = nullptr;
    QLabel* previewB_ = nullptr;
    QFrame* polynomialBCard_ = nullptr;
    QWidget* xField_ = nullptr;
    QPushButton* swapPolynomials_ = nullptr;
    QComboBox* polynomialOperation_ = nullptr;
    QComboBox* polynomialSample_ = nullptr;
    QLabel* polynomialResult_ = nullptr;
    QLabel* polynomialContext_ = nullptr;
    QLabel* polynomialError_ = nullptr;
    QLineEdit* polynomialSequence_ = nullptr;
    QTableWidget* polynomialTerms_ = nullptr;
    QFrame* polynomialTermsCard_ = nullptr;
    QPushButton* polynomialCopy_ = nullptr;
    QPushButton* useResult_ = nullptr;
    bool hasPolynomialResult_ = false;
    calculator::Polynomial lastPolynomial_;
};
