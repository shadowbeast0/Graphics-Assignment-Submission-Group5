#include "my_label.h"

my_label::my_label(QWidget *parent) : QLabel(parent)
{
    setMouseTracking(true);
}

void my_label::mouseMoveEvent(QMouseEvent *ev)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QPointF pf = ev->position();
    QPoint p = pf.toPoint();
#else
    QPoint p = ev->pos();
#endif
    if (p.x() >= 0 && p.y() >= 0 && p.x() < width() && p.y() < height()) {
        emit sendMousePosition(p);
    }
}

void my_label::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        QPoint p = ev->position().toPoint();
#else
        QPoint p = ev->pos();
#endif
        lastX = p.x();
        lastY = p.y();
        emit Mouse_Pos();
    }
}
