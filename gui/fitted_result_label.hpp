#pragma once

#include <QFontMetrics>
#include <QLabel>
#include <QResizeEvent>

// Keep every digit visible, including a 20-character signed 64-bit result.
// Recalculate after resizing instead of truncating large results.
class FittedResultLabel : public QLabel {
public:
    explicit FittedResultLabel(QWidget* parent = nullptr) : QLabel(parent) {
        setWordWrap(false);
    }

    void setText(const QString& text) {
        QLabel::setText(text);
        fit();
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        fit();
    }

private:
    void fit() {
        QFont next = font();
        int pixels = 44;
        next.setPixelSize(pixels);
        while (pixels > 16 && QFontMetrics(next).horizontalAdvance(text()) > contentsRect().width())
            next.setPixelSize(--pixels);
        if (font().pixelSize() != pixels) setFont(next);
    }
};
