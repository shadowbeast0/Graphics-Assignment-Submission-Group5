#ifndef MY_LABEL_H
#define MY_LABEL_H

#include <QtWidgets/QLabel>
#include <QtGui/QMouseEvent>
#include <QtCore/QPoint>

class my_label : public QLabel
{
    Q_OBJECT
public:
    explicit my_label(QWidget *parent = nullptr);
    int x = 0;
    int y = 0;

protected:
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;

signals:
    void sendMousePosition(const QPoint &);
    void Mouse_Pos();
};

#endif 
