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
        QImage *image = NULL;
        QLabel * info;
        int W = 540;
        int H = 360;
        ImgLabel(QWidget *parent, QLabel *info);
        ~ImgLabel();
        void set_image(int channel, int height, int width, unsigned char * data);
        void set_pixel();


    protected:
        void mouseMoveEvent(QMouseEvent *event);
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void paintEvent(QPaintEvent *event);
        void getRelativeXY(int px, int py, int * x, int * y);

};


#endif // IMGLABEL_H
