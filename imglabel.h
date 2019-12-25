#ifndef IMGLABEL_H
#define IMGLABEL_H


#include <QLabel>
#include <QScrollArea>
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QPen>
#include <QPainter>
#include <QButtonGroup>

class ImgLabel : public QLabel
{
    public:
        QImage *image;
        QLabel * info;
        int W = 640;
        int H = 360;

        ImgLabel(QLabel *info);
        void set_pixel();

    protected:
        //void mouseMoveEvent(QMouseEvent *event);
        //void mousePressEvent(QMouseEvent *event);
        //void mouseReleaseEvent(QMouseEvent *event);
        //void paintEvent(QPaintEvent *event);

};


#endif // IMGLABEL_H
