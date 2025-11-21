#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QTimer>
#include <QQueue>
#include <QSet>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    lastPoint1 = QPoint(-100000, -100000);
    lastPoint2 = QPoint(-100000, -100000);

    connect(ui->frame, SIGNAL(Mouse_Pos()), this, SLOT(Mouse_Pressed()));
    connect(ui->frame, SIGNAL(sendMousePosition(QPoint&)), this, SLOT(showMousePosition(QPoint&)));

    gap = 10;
    fcircle = 0;
    acircle = true;

    width = 750;
    height = 450;

    draw_grid();
    ui->polarcircletime->setText("Polar: 0 ns");
    ui->bresenhamcircletime->setText("Bresenham: 0 ns");

    ui->filled->setText("Filled: 0 pixels");

    ui->undo->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::showMousePosition(QPoint &pos)
{
    sc_x = pos.x();
    sc_y = pos.y();
    ui->mouse_movement->setText("X : " + QString::number((sc_x-sc_x%gap-((width/2)-(width/2)%gap))/gap) + ", Y : " + QString::number(-(sc_y-sc_y%gap-((height/2)-(height/2)%gap))/gap));
}

void MainWindow::Mouse_Pressed()
{
    org_x = sc_x;
    org_y = sc_y;

    int x = (sc_x-sc_x%gap-((width/2)-(width/2)%gap))/gap;
    int y = (sc_y-sc_y%gap-((height/2)-(height/2)%gap))/gap;

    ui->mouse_pressed->setText("X : " + QString::number(x) + ", Y : " + QString::number(y));

    lastPoint1 = lastPoint2;

    lastPoint2 = QPoint(x, y);
}

void MainWindow::on_clear_clicked()
{
    lastPoint1 = lastPoint2 = QPoint(-100000, -100000);
    colormap.clear();
    draw_grid();
    ui->polarcircletime->setText("Polar: 0 ns");
    ui->bresenhamcircletime->setText("Bresenham: 0 ns");

    ui->filled->setText("Filled: 0 pixels");
}

void MainWindow::draw_grid()
{
    QPixmap pix(ui->frame->width(), ui->frame->height());
    pix.fill(QColor(255, 255, 255));
    ui->frame->setPixmap(pix);

    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    painter.setPen(QPen(QColor(200, 200, 200)));
    for(int i=0; i<=width; i+=gap) painter.drawLine(QPoint(i,0), QPoint(i, height));
    for(int i=0; i<=height; i+=gap) painter.drawLine(QPoint(0, i), QPoint(width, i));

    painter.end();
    ui->frame->setPixmap(pm);

    paint(draw_line_bresenham(QPoint(-100000, 0), QPoint(100000, 0)), QColor(0, 0, 0));
    paint(draw_line_bresenham(QPoint(0, -100000), QPoint(0, 100000)), QColor(0, 0, 0));
}

void MainWindow::paint(std::vector<QPoint> points, QColor color){
    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    for(QPoint p:points){
        int x = p.x()*gap, y = p.y()*gap;
        painter.fillRect(QRect(x+((width/2) - (width/2)%gap), y+((height/2) - (height/2)%gap), gap, gap), color);
    }
    painter.end();
    ui->frame->setPixmap(pm);
    ui->filled->setText("Filled: " + QString::number(points.size()) + " pixels");
}

void MainWindow::paint(QPoint point, QColor color, QPainter& painter){
    int x = point.x()*gap, y = point.y()*gap;
    painter.fillRect(QRect(x+((width/2) - (width/2)%gap), y+((height/2) - (height/2)%gap), gap, gap), color);
}

std::vector<QPoint> MainWindow::draw_line_bresenham(QPoint a, QPoint b) {
    int x1 = a.x(), y1 = a.y();
    int x2 = b.x(), y2 = b.y();

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;

    std::vector<QPoint> point;

    while (true) {
        point.push_back(QPoint(x1, y1));

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }

    return point;
}

void MainWindow::on_spinBox_valueChanged(int value)
{
    gap = value;
    repaint();
}


void MainWindow::repaint(){
    draw_grid();
    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    for(const auto& pair : colormap){
        for(QPoint p : pair.first){
            paint(p, pair.second, painter);
        }
    }
    painter.end();
    ui->frame->setPixmap(pm);
}

void MainWindow::on_draw_circle_clicked()
{
    int cx = lastPoint1.x();
    int cy = lastPoint1.y();
    int dx = lastPoint2.x() - lastPoint1.x();
    int dy = lastPoint2.y() - lastPoint1.y();
    int radius = std::round(std::sqrt(dx * dx + dy * dy));

    QPoint centre = QPoint(cx, cy);

    draw_circle(centre, radius);
}


void MainWindow::draw_circle(QPoint centre, int radius){
    if(acircle && ui->animate_circle->isChecked()){
        if(fcircle == 0){
            auto theta = std::make_shared<double>(0);
            QTimer *timer = new QTimer(this);
            connect(timer, &QTimer::timeout, this, [this, timer, theta, centre, radius]() {
                if (*theta > M_PI/4) {
                    timer->stop();
                    timer->deleteLater();
                    ui->undo->setEnabled(true);
                }
                animate_circle_polar(centre, radius, *theta);
                *theta += 1.0/radius;
            });
            timer->start(100);
        }
        else if(fcircle == 1){
            QTimer *timer = new QTimer(this);
            auto x = std::make_shared<int>(0);
            auto y = std::make_shared<int>(radius);
            auto d = std::make_shared<int>(3 - 2*radius);
            connect(timer, &QTimer::timeout, this, [this, timer, centre, x, y, d]() {
                if (*x > *y) {
                    timer->stop();
                    timer->deleteLater();
                }
                animate_circle_bresenham(centre, *x, *y, *d);
            });
            timer->start(100);
        }
        else{
            int r = std::round(radius / std::sqrt(2));
            QTimer *timer = new QTimer(this);
            auto y = std::make_shared<int>(0);
            connect(timer, &QTimer::timeout, this, [this, timer, y, r, centre, radius]() {
                if (*y > r) {
                    timer->stop();
                    timer->deleteLater();
                    ui->undo->setEnabled(true);
                }
                animate_circle_cartesian(centre, radius, *y);
                (*y)++;
            });
            timer->start(100);
        }
        std::vector<QPoint> point = fcircle ? draw_circle_cartesian(centre, radius) : draw_circle_bresenham(centre, radius);
        colormap.push_back(std::pair(point, fcircle==0? QColor(150, 0, 0) : fcircle==1? QColor(255, 150, 0) : QColor(255, 200, 0)));
        ui->undo->setEnabled(true);
    }


    else{
        QElapsedTimer timer;
        qint64 time = 0;
        std::vector<QPoint> point;
        if(fcircle == 0){
            for(int i=0; i<100; i++){
                timer.start();
                point = draw_circle_polar(centre, radius);
                time += timer.nsecsElapsed();
            }
            ui->polarcircletime->setText("Polar: " + QString::number(time/100) + " ns");
        }
        else if(fcircle == 1){
            for(int i=0; i<100; i++){
                timer.start();
                point = draw_circle_bresenham(centre, radius);
                time += timer.nsecsElapsed();
            }
            ui->bresenhamcircletime->setText("Bresenham: " + QString::number(time/100) + " ns");
        }
        else{
            point = draw_circle_cartesian(centre, radius);
        }
        colormap.push_back(std::pair(point, fcircle==0? QColor(150, 0, 0) : fcircle==1? QColor(255, 150, 0) : QColor(255, 200, 0)));
        paint(point, fcircle==0? QColor(150, 0, 0) : fcircle==1? QColor(255, 150, 0) : QColor(255, 200, 0));
    }
}


std::vector<QPoint> MainWindow::draw_circle_polar(QPoint centre, int radius){
    int cx = centre.x();
    int cy = centre.y();

    std::vector<QPoint> point;
    for(double theta = 0; theta <= M_PI/4; theta += 1.0/radius){
        int x = std::round(radius * cos(theta)), y = std::round(radius * sin(theta));
        point.push_back(QPoint(cx + x, cy + y));
        point.push_back(QPoint(cx - x, cy + y));
        point.push_back(QPoint(cx + x, cy - y));
        point.push_back(QPoint(cx - x, cy - y));
        point.push_back(QPoint(cx + y, cy + x));
        point.push_back(QPoint(cx - y, cy + x));
        point.push_back(QPoint(cx + y, cy - x));
        point.push_back(QPoint(cx - y, cy - x));
    }

    return point;
}


std::vector<QPoint> MainWindow::draw_circle_cartesian(QPoint centre, int radius){
    int r = std::round(radius / std::sqrt(2));

    int cx = centre.x();
    int cy = centre.y();

    std::vector<QPoint> point;
    for(int y = 0; y<=r; y++){
        int x = std::round(std::sqrt(radius * radius - y * y));
        point.push_back(QPoint(cx + x, cy + y));
        point.push_back(QPoint(cx - x, cy + y));
        point.push_back(QPoint(cx + x, cy - y));
        point.push_back(QPoint(cx - x, cy - y));
        point.push_back(QPoint(cx + y, cy + x));
        point.push_back(QPoint(cx - y, cy + x));
        point.push_back(QPoint(cx + y, cy - x));
        point.push_back(QPoint(cx - y, cy - x));
    }

    return point;
}

std::vector<QPoint> MainWindow::draw_circle_bresenham(QPoint centre, int radius){
    int cx = centre.x();
    int cy = centre.y();

    int x = 0, y = radius;
    int d = 3 - 2 * radius;

    std::vector<QPoint> point;

    while(x <= y){
        point.push_back(QPoint(cx + x, cy + y));
        point.push_back(QPoint(cx - x, cy + y));
        point.push_back(QPoint(cx + x, cy - y));
        point.push_back(QPoint(cx - x, cy - y));
        point.push_back(QPoint(cx + y, cy + x));
        point.push_back(QPoint(cx - y, cy + x));
        point.push_back(QPoint(cx + y, cy - x));
        point.push_back(QPoint(cx - y, cy - x));
        if(d < 0) d += 4 * x + 6;
        else{
            y--;
            d += 4 * (x - y) + 10;
        }
        x++;
    }

    return point;
}

void MainWindow::animate_circle_polar(QPoint centre, int radius, double theta){
    int cx = centre.x();
    int cy = centre.y();

    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);

    int x = std::round(radius * cos(theta)), y = std::round(radius * sin(theta));

    paint(QPoint(cx + x, cy + y), QColor(150, 0, 0), painter);
    paint(QPoint(cx - x, cy + y), QColor(150, 0, 0), painter);
    paint(QPoint(cx + x, cy - y), QColor(150, 0, 0), painter);
    paint(QPoint(cx - x, cy - y), QColor(150, 0, 0), painter);
    paint(QPoint(cx + y, cy + x), QColor(150, 0, 0), painter);
    paint(QPoint(cx - y, cy + x), QColor(150, 0, 0), painter);
    paint(QPoint(cx + y, cy - x), QColor(150, 0, 0), painter);
    paint(QPoint(cx - y, cy - x), QColor(150, 0, 0), painter);
    painter.end();
    ui->frame->setPixmap(pm);
}

void MainWindow::animate_circle_cartesian(QPoint centre, int radius, int y){
    int cx = centre.x();
    int cy = centre.y();

    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    int x = std::round(std::sqrt(radius * radius - y * y));
    paint(QPoint(cx + x, cy + y), QColor(255, 200, 0), painter);
    paint(QPoint(cx - x, cy + y), QColor(255, 200, 0), painter);
    paint(QPoint(cx + x, cy - y), QColor(255, 200, 0), painter);
    paint(QPoint(cx - x, cy - y), QColor(255, 200, 0), painter);
    paint(QPoint(cx + y, cy + x), QColor(255, 200, 0), painter);
    paint(QPoint(cx - y, cy + x), QColor(255, 200, 0), painter);
    paint(QPoint(cx + y, cy - x), QColor(255, 200, 0), painter);
    paint(QPoint(cx - y, cy - x), QColor(255, 200, 0), painter);
    painter.end();
    ui->frame->setPixmap(pm);
}

void MainWindow::animate_circle_bresenham(QPoint centre, int& x, int& y, int& d){
    int cx = centre.x();
    int cy = centre.y();

    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    paint(QPoint(cx + x, cy + y), QColor(255, 150, 0), painter);
    paint(QPoint(cx - x, cy + y), QColor(255, 150, 0), painter);
    paint(QPoint(cx + x, cy - y), QColor(255, 150, 0), painter);
    paint(QPoint(cx - x, cy - y), QColor(255, 150, 0), painter);
    paint(QPoint(cx + y, cy + x), QColor(255, 150, 0), painter);
    paint(QPoint(cx - y, cy + x), QColor(255, 150, 0), painter);
    paint(QPoint(cx + y, cy - x), QColor(255, 150, 0), painter);
    paint(QPoint(cx - y, cy - x), QColor(255, 150, 0), painter);
    painter.end();
    ui->frame->setPixmap(pm);
    if(d < 0) d += 4 * x + 6;
    else{
        y--;
        d += 4 * (x - y) + 10;
    }
    x++;
}

void MainWindow::on_circle_type_currentIndexChanged(int index)
{
    fcircle = index;
}

void MainWindow::on_undo_clicked()
{
    colormap.pop_back();
    if(colormap.empty()) ui->undo->setEnabled(false);
    repaint();
}
