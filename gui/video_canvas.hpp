#pragma once

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QVideoFrame>
#include <QWidget>

// A compact video surface for the decorative animation. Keeping only the
// latest decoded image makes painting work in native and offscreen widgets.
class VideoCanvas : public QWidget {
public:
    explicit VideoCanvas(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedSize(292, 274);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    QSize sizeHint() const override { return QSize(292, 274); }
    bool hasFrame() const noexcept { return !image_.isNull(); }

    void setFrame(const QVideoFrame& frame) {
        if (!frame.isValid()) return;
        const QImage next = frame.toImage();
        if (next.isNull()) return;
        image_ = next;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath clip;
        clip.addRoundedRect(QRectF(rect()), 16, 16);
        painter.setClipPath(clip);
        painter.fillRect(rect(), Qt::black);
        if (image_.isNull()) return;
        // Letterbox instead of cropping or stretching the source animation.
        const QSize fitted = image_.size().scaled(size(), Qt::KeepAspectRatio);
        const QRect target((width() - fitted.width()) / 2,
                           (height() - fitted.height()) / 2,
                           fitted.width(), fitted.height());
        painter.drawImage(target, image_);
    }

private:
    QImage image_;
};
