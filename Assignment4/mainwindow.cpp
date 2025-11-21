#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/QSet>
#include <QtCore/qmath.h>      
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <algorithm>

static inline QPair<int,int> cellOfPx(int x_px, int y_px, int grid_box) {
    return qMakePair(qFloor(x_px / double(grid_box)), qFloor(y_px / double(grid_box)));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    draw_clicked = false;

    lastPoint1 = QPoint(-1, -1);
    lastPoint2 = QPoint(-1, -1);

    
    QPixmap pix(ui->frame->width(), ui->frame->height());
    pix.fill(Qt::black);
    ui->frame->setPixmap(pix);

    
    connect(ui->frame, SIGNAL(Mouse_Pos()), this, SLOT(Mouse_Pressed()));
    connect(ui->frame, SIGNAL(sendMousePosition(const QPoint&)),this, SLOT(showMousePosition(const QPoint&)));

    
    grid_box  = 10;                               
    grid_size = ui->frame->frameSize().width();   
    center    = 0;

    
    polygonFlag = false;
    currPoly.col = Qt::blue;
    currPoly.vertices.clear();

    
    currentAlgo = Algo::Flood;

    
    {
        
        auto *connLabel = new QLabel("Connectivity:", this);
        connectivitySelect = new QComboBox(this);
        connectivitySelect->setObjectName("connectivity_select");
        connectivitySelect->addItem("4-neighbour");
        connectivitySelect->addItem("8-neighbour");

        
        int idx = ui->verticalLayout->indexOf(ui->algo_select);
        ui->verticalLayout->insertWidget(idx + 1, connLabel);
        ui->verticalLayout->insertWidget(idx + 2, connectivitySelect);

        
        connect(connectivitySelect, &QComboBox::currentIndexChanged, this, [this](int i){
            connectivityMode = (i == 1) ? Connectivity::Eight : Connectivity::Four;
        });
        connectivitySelect->setCurrentIndex(0); 
    }

    
    on_draw_grid_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}




void MainWindow::showMousePosition(const QPoint &pos)
{
    sc_x = pos.x();
    sc_y = pos.y();

    int X = (sc_x / this->grid_box) - this->center;
    int Y = -(sc_y / this->grid_box) + this->center;

    ui->mouse_movement->setText("X : " + QString::number(X) + ", Y : " + QString::number(Y));
}

void MainWindow::Mouse_Pressed()
{
    org_x = sc_x;
    org_y = sc_y;

    int X = (sc_x / this->grid_box) - this->center;
    int Y = -(sc_y / this->grid_box) + this->center;

    lastPoint1 = lastPoint2;
    lastPoint2 = QPoint(org_x, org_y);

    if (polygonFlag) {
        
        currPoly.vertices.push_back(QPoint(X, Y));
        ui->mouse_pressed->setText("Vertex: X=" + QString::number(X) + ", Y=" + QString::number(Y));

        
        redraw();
        QPixmap pm = ui->frame->pixmap();
        QPainter painter(&pm);
        draw_polygon_preview(painter, currPoly.vertices);
        draw_vertex_markers(painter, currPoly.vertices);
        painter.end();
        ui->frame->setPixmap(pm);
    } else {
        
        ui->mouse_pressed->setText("Painted: X=" + QString::number(X) + ", Y=" + QString::number(Y));
        paintCellAtPixel(org_x, org_y);
    }
}




void MainWindow::on_draw_grid_clicked()
{
    draw_clicked = true;

    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);

    const int total_cells = this->grid_size / this->grid_box;
    this->center = total_cells / 2;

    
    draw_axes(painter);

    
    painter.setPen(QPen(Qt::white, 1));
    for (int i = 0; i <= total_cells; i++) {
        int pos = i * this->grid_box;
        painter.drawLine(QPoint(pos, 0), QPoint(pos, this->grid_size));
        painter.drawLine(QPoint(0, pos), QPoint(this->grid_size, pos));
    }

    painter.end();
    ui->frame->setPixmap(pm);
}

void MainWindow::draw_axes(QPainter& painter)
{
    int c = this->center;
    draw_line_grid(-c,  c, 0, 0, painter, Qt::white); 
    draw_line_grid(0, 0, -c,  c, painter, Qt::white); 
}




void MainWindow::draw_line_grid(int X1, int X2, int Y1, int Y2,
                                QPainter& painter, QColor col)
{
    const int cells = this->grid_size / this->grid_box;

    
    int gx1 = X1 + this->center;
    int gy1 = this->center - Y1;
    int gx2 = X2 + this->center;
    int gy2 = this->center - Y2;

    QSet<QRect> rects;

    int dx = std::abs(gx2 - gx1);
    int dy = std::abs(gy2 - gy1);
    int sx = (gx1 < gx2) ? 1 : -1;
    int sy = (gy1 < gy2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (gy1 >= 0 && gy1 < cells && gx1 >= 0 && gx1 < cells) {
            rects.insert(QRect(gx1 * this->grid_box, gy1 * this->grid_box,
                               this->grid_box, this->grid_box));
        }
        if (gx1 == gx2 && gy1 == gy2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; gx1 += sx; }
        if (e2 <  dx) { err += dx; gy1 += sy; }
    }

    for (const QRect &rect : rects) {
        painter.fillRect(rect, QBrush(col, Qt::SolidPattern));
    }
}




void MainWindow::on_clear_clicked()
{
    lastPoint1 = QPoint(-1, -1);
    lastPoint2 = QPoint(-1, -1);
    currPoly.vertices.clear();
    polygons.clear();
    paintedCells.clear();

    QPixmap pix(ui->frame->width(), ui->frame->height());
    pix.fill(Qt::black);
    ui->frame->setPixmap(pix);

    on_draw_grid_clicked(); 
}

void MainWindow::refreshCanvas()
{
    QPixmap pix(ui->frame->width(), ui->frame->height());
    pix.fill(Qt::black);
    ui->frame->setPixmap(pix);
    on_draw_grid_clicked();
}

void MainWindow::drawPaintedCells(QPainter& painter)
{
    for (auto it = paintedCells.constBegin(); it != paintedCells.constEnd(); ++it) {
        const QPair<int,int>& cell = it.key();
        const QColor& color = it.value();
        QRect rect(cell.first * this->grid_box, cell.second * this->grid_box,
                   this->grid_box, this->grid_box);
        painter.fillRect(rect, QBrush(color, Qt::SolidPattern));
    }
}

void MainWindow::redraw()
{
    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);

    
    drawPaintedCells(painter);

    
    for (const auto &poly : std::as_const(polygons)) {
        if (poly.vertices.size() >= 2) {
            draw_polygon(painter, poly);
        }
    }

    painter.end();
    ui->frame->setPixmap(pm);

    
    for (const auto &poly : std::as_const(polygons)) {
        if (poly.boundaryFilled && poly.boundarySeedPx != QPoint(-1, -1)) {
            boundaryFill(poly.boundarySeedPx.x(), poly.boundarySeedPx.y(),
                         poly.boundaryCol, QColor(poly.col));
        }
        if (poly.floodFilled && poly.floodSeedPx != QPoint(-1, -1)) {
            floodFill(poly.floodSeedPx.x(), poly.floodSeedPx.y(), poly.floodCol);
        }
        if (poly.scanFilled) {
            scanlineFill(poly, poly.scanCol, false);
        }
    }
}




void MainWindow::on_grid_size_valueChanged(int grid)
{
    if (grid < 2) return;
    this->grid_box = grid;

    if (!draw_clicked) return;

    refreshCanvas();
    redraw();
}




void MainWindow::draw_polygon(QPainter& painter, const Polygon& poly)
{
    if (poly.vertices.size() < 2) return;

    for (int i = 0; i < poly.vertices.size() - 1; ++i) {
        const QPoint p1 = poly.vertices[i];
        const QPoint p2 = poly.vertices[i + 1];
        draw_line_grid(p1.x(), p2.x(), p1.y(), p2.y(), painter, QColor(poly.col));
    }
    
    draw_line_grid(poly.vertices.back().x(), poly.vertices.front().x(),
                   poly.vertices.back().y(), poly.vertices.front().y(),
                   painter, QColor(poly.col));
}

void MainWindow::draw_polygon_preview(QPainter& painter, const QVector<QPoint>& verts)
{
    if (verts.size() < 1) return;
    
    for (int i = 0; i + 1 < verts.size(); ++i) {
        const QPoint p1 = verts[i];
        const QPoint p2 = verts[i + 1];
        draw_line_grid(p1.x(), p2.x(), p1.y(), p2.y(), painter, QColor(Qt::blue));
    }
}

void MainWindow::draw_vertex_markers(QPainter& painter, const QVector<QPoint>& verts)
{
    const int cells = this->grid_size / this->grid_box;
    for (const QPoint &v : verts) {
        int gx = v.x() + this->center;
        int gy = this->center - v.y();
        if (gx < 0 || gy < 0 || gx >= cells || gy >= cells) continue;
        QRect rect(gx * this->grid_box, gy * this->grid_box, this->grid_box, this->grid_box);
        painter.fillRect(rect, QBrush(Qt::cyan, Qt::SolidPattern)); 
    }
}

void MainWindow::on_start_polygon_clicked()
{
    polygonFlag = true;
    currPoly = Polygon();          
    currPoly.col = Qt::blue;
    currPoly.vertices.clear();
}

void MainWindow::on_close_polygon_clicked()
{
    if (currPoly.vertices.size() < 3) {
        qDebug() << "Need at least 3 points to form a polygon.";
    } else {
        polygonFlag = false;
        polygons.push_back(currPoly);

        
        QPixmap pm = ui->frame->pixmap();
        QPainter painter(&pm);
        draw_polygon(painter, currPoly);
        painter.end();
        ui->frame->setPixmap(pm);
    }
    currPoly.vertices.clear();
}




void MainWindow::paintCellAtPixel(int x_px, int y_px)
{
    auto cell = cellOfPx(x_px, y_px, this->grid_box);
    const int gx = cell.first;
    const int gy = cell.second;

    const int cells = this->grid_size / this->grid_box;
    if (gx < 0 || gy < 0 || gx >= cells || gy >= cells) return;

    paintedCells[qMakePair(gx, gy)] = Qt::yellow;

    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);

    QRect rect(gx * this->grid_box, gy * this->grid_box,
               this->grid_box, this->grid_box);
    painter.fillRect(rect, QBrush(Qt::yellow, Qt::SolidPattern));
    painter.end();

    ui->frame->setPixmap(pm);
}




void MainWindow::floodFill(int x, int y, const QColor& newColor)
{
    QPixmap pm = ui->frame->pixmap();
    if (pm.isNull()) return;

    const int cells = this->grid_size / this->grid_box;
    int gx0 = qFloor(x / (double)this->grid_box);
    int gy0 = qFloor(y / (double)this->grid_box);
    if (gx0 < 0 || gy0 < 0 || gx0 >= cells || gy0 >= cells) return;

    QImage img = pm.toImage();

    
    int cx0 = gx0 * this->grid_box + this->grid_box / 2;
    int cy0 = gy0 * this->grid_box + this->grid_box / 2;
    QColor seed = img.pixelColor(cx0, cy0);
    const bool seedIsYellow = (seed == Qt::yellow);

    QColor regionColor;
    if (seed == Qt::white || seedIsYellow) {
        regionColor = Qt::black;         
    } else {
        regionColor = seed;
    }

    if (regionColor == newColor) return;

    
    paintedCells.remove(qMakePair(gx0, gy0));

    QSet<QPair<int, int>> visited;
    QVector<QPoint> stack;
    stack.append(QPoint(gx0, gy0));

    QPainter painter(&pm);

    auto maybeAnimate = [&]() {
        ui->frame->setPixmap(pm);
        QCoreApplication::processEvents();
        QThread::msleep(15);
    };

    auto push4 = [&](int gx, int gy) {
        stack.append(QPoint(gx + 1, gy));
        stack.append(QPoint(gx - 1, gy));
        stack.append(QPoint(gx, gy + 1));
        stack.append(QPoint(gx, gy - 1));
    };
    auto push8 = [&](int gx, int gy) {
        push4(gx, gy);
        stack.append(QPoint(gx + 1, gy + 1));
        stack.append(QPoint(gx - 1, gy + 1));
        stack.append(QPoint(gx + 1, gy - 1));
        stack.append(QPoint(gx - 1, gy - 1));
    };

    while (!stack.isEmpty()) {
        QPoint p = stack.takeLast();
        int gx = p.x();
        int gy = p.y();

        if (gx < 0 || gy < 0 || gx >= cells || gy >= cells) continue;

        QPair<int, int> key(gx, gy);
        if (visited.contains(key)) continue;
        visited.insert(key);

        int cx = gx * this->grid_box + this->grid_box / 2;
        int cy = gy * this->grid_box + this->grid_box / 2;

        QColor cur = img.pixelColor(cx, cy);

        const bool isSeed = (gx == gx0 && gy == gy0);

        
        if (cur == Qt::yellow && !isSeed) continue;

        
        if (cur == Qt::white) cur = regionColor;

        
        if (isSeed && seedIsYellow) cur = regionColor;

        if (cur != regionColor) continue;

        QRect rect(gx * this->grid_box, gy * this->grid_box, this->grid_box, this->grid_box);
        painter.fillRect(rect, QBrush(newColor, Qt::SolidPattern));

        maybeAnimate();

        if (connectivityMode == Connectivity::Eight) {
            push8(gx, gy);
        } else {
            push4(gx, gy);
        }
    }

    painter.end();
    ui->frame->setPixmap(pm);
}


void MainWindow::boundaryFill(int x, int y, const QColor& newColor, const QColor& boundaryColor)
{
    QPixmap pm = ui->frame->pixmap();
    if (pm.isNull()) return;

    const int cells = this->grid_size / this->grid_box;
    int gx0 = qFloor(x / (double)this->grid_box);
    int gy0 = qFloor(y / (double)this->grid_box);
    if (gx0 < 0 || gy0 < 0 || gx0 >= cells || gy0 >= cells) return;

    QImage img = pm.toImage();

    QSet<QPair<int,int>> visited;
    QVector<QPoint> stack;
    stack.append(QPoint(gx0, gy0));

    QPainter painter(&pm);

    auto maybeAnimate = [&]() {
        ui->frame->setPixmap(pm);
        QCoreApplication::processEvents();
        QThread::msleep(15);
    };

    auto push4 = [&](int gx, int gy) {
        stack.append(QPoint(gx + 1, gy));
        stack.append(QPoint(gx - 1, gy));
        stack.append(QPoint(gx, gy + 1));
        stack.append(QPoint(gx, gy - 1));
    };
    auto push8 = [&](int gx, int gy) {
        push4(gx, gy);
        stack.append(QPoint(gx + 1, gy + 1));
        stack.append(QPoint(gx - 1, gy + 1));
        stack.append(QPoint(gx + 1, gy - 1));
        stack.append(QPoint(gx - 1, gy - 1));
    };

    while (!stack.isEmpty()) {
        QPoint p = stack.takeLast();
        int gx = p.x();
        int gy = p.y();
        if (gx < 0 || gy < 0 || gx >= cells || gy >= cells) continue;

        QPair<int,int> key(gx, gy);
        if (visited.contains(key)) continue;
        visited.insert(key);

        int cx = gx * this->grid_box + this->grid_box / 2;
        int cy = gy * this->grid_box + this->grid_box / 2;

        QColor cur = img.pixelColor(cx, cy);
        if (cur == newColor) continue;             
        if (cur == boundaryColor) continue;        

        QRect rect(gx * this->grid_box, gy * this->grid_box, this->grid_box, this->grid_box);
        painter.fillRect(rect, QBrush(newColor, Qt::SolidPattern));

        maybeAnimate();

        if (connectivityMode == Connectivity::Eight) {
            push8(gx, gy);
        } else {
            push4(gx, gy);
        }
    }

    painter.end();
    ui->frame->setPixmap(pm);
}

void MainWindow::scanlineFill(const Polygon& poly, const QColor& fillColor, bool animate)
{
    if (poly.vertices.size() < 3) return;

    int ymin = poly.vertices.front().y();
    int ymax = ymin;
    for (const QPoint &v : poly.vertices) {
        ymin = std::min(ymin, v.y());
        ymax = std::max(ymax, v.y());
    }

    QPixmap pm = ui->frame->pixmap();
    if (pm.isNull()) return;

    const int cells = this->grid_size / this->grid_box;
    const int n = poly.vertices.size();

    QPainter painter(&pm);
    QImage   img = pm.toImage();

    auto maybeAnimate = [&]() {
        if (animate) {
            ui->frame->setPixmap(pm);
            QCoreApplication::processEvents();
            QThread::msleep(20);
        }
    };

    const double epsx = 1e-6;

    for (int y = ymin; y <= ymax; ++y) {
        QVector<double> interX;
        interX.reserve(n);

        for (int i = 0; i < n; ++i) {
            const QPoint p1 = poly.vertices[i];
            const QPoint p2 = poly.vertices[(i + 1) % n];
            const int x1 = p1.x(), y1 = p1.y();
            const int x2 = p2.x(), y2 = p2.y();

            if (y1 == y2) continue;

            const int yminEdge = std::min(y1, y2);
            const int ymaxEdge = std::max(y1, y2);
            if (y >= yminEdge && y < ymaxEdge) {
                const double t  = ((y + 0.5) - y1) / double(y2 - y1);
                const double ix = x1 + t * (x2 - x1);
                interX.append(ix);
            }
        }

        if (interX.isEmpty()) continue;
        std::sort(interX.begin(), interX.end());

        for (int k = 0; k + 1 < interX.size(); k += 2) {
            const double L = std::min(interX[k],   interX[k+1]);
            const double R = std::max(interX[k],   interX[k+1]);

            int xLeft  = int(std::ceil (L + epsx));
            int xRight = int(std::floor(R - epsx));
            if (xRight < xLeft) continue;

            for (int lx = xLeft; lx <= xRight; ++lx) {
                int gx = lx + this->center;
                int gy = this->center - y;
                if (gx < 0 || gy < 0 || gx >= cells || gy >= cells) continue;

                const int cx = gx * this->grid_box + this->grid_box / 2;
                const int cy = gy * this->grid_box + this->grid_box / 2;
                const QColor cur = img.pixelColor(cx, cy);

                if (cur == QColor(poly.col) ) continue;

                QRect rect(gx * this->grid_box, gy * this->grid_box,
                           this->grid_box, this->grid_box);
                painter.fillRect(rect, QBrush(fillColor, Qt::SolidPattern));
            }
            maybeAnimate();
        }
    }

    painter.end();
    ui->frame->setPixmap(pm);
}


void MainWindow::on_fill_polygon_clicked()
{
    if (polygons.isEmpty()) return;

    Polygon &poly = polygons.last();

    if (currentAlgo == Algo::Boundary) {
        if (lastPoint2 == QPoint(-1, -1)) return;
        poly.boundaryFilled = true;
        poly.boundaryCol = Qt::green;
        poly.boundarySeedPx = lastPoint2;
        boundaryFill(lastPoint2.x(), lastPoint2.y(), poly.boundaryCol, QColor(poly.col));
    } else if (currentAlgo == Algo::Flood) {
        if (lastPoint2 == QPoint(-1, -1)) return;
        poly.floodFilled = true;
        poly.floodCol = Qt::red;
        poly.floodSeedPx = lastPoint2;
        floodFill(lastPoint2.x(), lastPoint2.y(), poly.floodCol);
    } else { 
        poly.scanFilled = true;
        poly.scanCol = Qt::magenta;
        scanlineFill(poly, poly.scanCol, true);
    }
}

void MainWindow::on_clear_fill_clicked()
{
    
    for (auto &poly : polygons) {
        poly.floodFilled = false;     poly.floodSeedPx = QPoint(-1,-1);
        poly.boundaryFilled = false;  poly.boundarySeedPx = QPoint(-1,-1);
        poly.scanFilled = false;
    }
    refreshCanvas();
    redraw();
}




void MainWindow::on_algo_select_currentIndexChanged(int)
{
    const QString txt = ui->algo_select->currentText().toLower();
    if (txt.contains("boundary")) { currentAlgo = Algo::Boundary; return; }
    if (txt.contains("flood"))    { currentAlgo = Algo::Flood;    return; }
    if (txt.contains("scan"))     { currentAlgo = Algo::Scan;     return; }
    currentAlgo = Algo::Flood; 
}
