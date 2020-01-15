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
#include <string>
#include "movie.h"

class ImgLabel : public QLabel
{
    private:
        QImage * image = NULL;
        QLabel * info;
        Movie* movie = NULL;
        int W = 540;
        int H = 360;
        int image_h, image_w, top_h, top_w;
        bool has_movie = false;

        void set_image(int channel, int height, int width, unsigned char * data);
        void set_pixel();
        bool display_next_frame();

    public:
        ImgLabel(QWidget *parent, QLabel *info);
        ~ImgLabel();
        void set_movie(std::string path);
        void clear_movie();

    protected:
        void mouseMoveEvent(QMouseEvent *event);
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void paintEvent(QPaintEvent *event);
        void getRelativeXY(int px, int py, int * x, int * y);


};


#endif // IMGLABEL_H
