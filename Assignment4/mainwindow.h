#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtCore/QPoint>
#include <QtCore/QVector>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QColor>
#include <QtCore/QMap>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct Polygon {
    QVector<QPoint> vertices;           
    Qt::GlobalColor col = Qt::blue;     

    
    bool   floodFilled  = false;
    QColor floodCol     = Qt::red;
    QPoint floodSeedPx  = QPoint(-1, -1);

    
    bool   boundaryFilled  = false;
    QColor boundaryCol     = Qt::green;          
    QPoint boundarySeedPx  = QPoint(-1, -1);

    
    bool   scanFilled  = false;
    QColor scanCol     = Qt::blue;               
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showMousePosition(const QPoint& pos);
    void Mouse_Pressed();

    
    void on_draw_grid_clicked();
    void on_clear_clicked();
    void on_grid_size_valueChanged(int grid);

    void on_start_polygon_clicked();
    void on_close_polygon_clicked();
    void on_fill_polygon_clicked();
    void on_clear_fill_clicked();

    
    void on_algo_select_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;

    
    int  grid_box;     
    int  grid_size;    
    int  center;       
    bool draw_clicked;

    
    int sc_x, sc_y;
    int org_x, org_y;
    QPoint lastPoint1, lastPoint2;

    
    bool polygonFlag;
    Polygon currPoly;
    QVector<Polygon> polygons;

    
    QMap<QPair<int,int>, QColor> paintedCells;

    
    enum class Algo { Flood, Boundary, Scan };
    Algo currentAlgo;

    
    enum class Connectivity { Four, Eight };
    Connectivity connectivityMode = Connectivity::Four;
    QComboBox* connectivitySelect = nullptr; 

    
    void draw_axes(QPainter& painter);
    void refreshCanvas();          
    void redraw();                 
    void drawPaintedCells(QPainter& painter);

    
    void draw_line_grid(int X1, int X2, int Y1, int Y2, QPainter& painter, QColor col);

    
    void draw_polygon(QPainter& painter, const Polygon& poly);                  
    void draw_polygon_preview(QPainter& painter, const QVector<QPoint>& verts); 
    void draw_vertex_markers(QPainter& painter, const QVector<QPoint>& verts);

    
    void floodFill(int x_px, int y_px, const QColor& newColor);                          
    void boundaryFill(int x_px, int y_px, const QColor& newColor, const QColor& boundaryColor);
    void scanlineFill(const Polygon& poly, const QColor& fillColor, bool animate);

    
    void paintCellAtPixel(int x_px, int y_px);
};

#endif 
