#include "calculator_window.hpp"
#include "style.hpp"
#include <QApplication>

int main(int argc, char** argv) {
    QApplication application(argc, argv);
    application.setApplicationName("Project1 Calculator");
    application.setOrganizationName("Project1");
    applyCalculatorStyle(application);
    CalculatorWindow window;
    window.show();
    return application.exec();
}
