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
#include <QSlider>
#include <QObject>
#include <QCoreApplication>
#include "movie.h"

class ImgLabel : public QLabel
{

    Q_OBJECT
    private:
        QImage * image = NULL;
        QLabel * info;
        Movie* movie = NULL;
        QSlider * progress;
        int W = 540;
        int H = 360;
        int image_h, image_w, top_h, top_w;
        bool display_lock = false;
        bool on_play = true;
        double system_anchor_time;
        double movie_anchor_time;


        bool display_next_frame();
        void clear_movie();
        void format_progress();
        std::string format_time(double second);
        double get_system_time();
        void sysnc_time();

    private slots:
        void set_progress_start();
        void set_progress_end();

    public:
        ImgLabel(QWidget *parent, QLabel *info, QSlider *progress);
        ~ImgLabel();
        void set_movie(std::string path);
        void play();
        void pause();

    protected:
        void mouseMoveEvent(QMouseEvent *event);
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void paintEvent(QPaintEvent *event);
        void getRelativeXY(int px, int py, int * x, int * y);


};


#endif // IMGLABEL_H
