#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QSet>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void Mouse_Pressed();
    void showMousePosition(QPoint&);
    void on_clear_clicked();
    void draw_grid();
    std::vector<QPoint> draw_line_bresenham(QPoint, QPoint);

    void on_spinBox_valueChanged(int);
    void repaint();

    void paint(std::vector<QPoint>, QColor);
    void paint(QPoint, QColor, QPainter&);

    void on_draw_circle_clicked();
    void draw_circle(QPoint, int);
    std::vector<QPoint> draw_circle_polar(QPoint, int);
    std::vector<QPoint> draw_circle_bresenham(QPoint, int);
    std::vector<QPoint> draw_circle_cartesian(QPoint, int);

    void animate_circle_polar(QPoint, int, double);
    void animate_circle_cartesian(QPoint, int, int);
    void animate_circle_bresenham(QPoint, int&, int&, int&);

    void on_circle_type_currentIndexChanged(int);

    void on_undo_clicked();

private:
    Ui::MainWindow *ui;
    void addPoint(int x, int y, int c = 1);

    QPoint lastPoint1, lastPoint2;
    int sc_x, sc_y;
    int org_x, org_y;
    int width, height;

    std::vector<std::pair<std::vector<QPoint>, QColor>> colormap;
    int gap;
    bool acircle;
    int fcircle;
};

#endif // MAINWINDOW_H
