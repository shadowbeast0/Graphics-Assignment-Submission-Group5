#include "my_label.h"

my_label::my_label(QWidget *parent) : QLabel(parent)
{
    setMouseTracking(true);
}

void my_label::mouseMoveEvent(QMouseEvent *ev)
{
    const QPoint pos = ev->pos();
    if (pos.x() >= 0 && pos.y() >= 0 &&
        pos.x() < width() && pos.y() < height()) {
        emit sendMousePosition(pos);
    }
    
    QLabel::mouseMoveEvent(ev);
}

void my_label::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        x = ev->x();
        y = ev->y();
        emit Mouse_Pos();
    }
    QLabel::mousePressEvent(ev);
}
