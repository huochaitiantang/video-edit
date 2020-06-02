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
#include "fetchframethread.h"

class ImgLabel : public QLabel
{

    Q_OBJECT
    private:
        QImage * image = NULL;
        QLabel * info;
        Movie* movie = NULL;
        QSlider * progress;
        int W = 720;
        int H = 360;
        int image_h, image_w, top_h, top_w;
        bool display_lock = false;
        bool on_play = true;
        double system_anchor_time;
        double movie_anchor_time;
        double movie_duration;
        double movie_fps;
        int movie_frame_count;
        double play_times = 1.0;
        FetchFrameThread* fetch_frame_thread;


        bool display_next_frame();
        void clear_movie();
        std::string format_time(double second);
        double get_system_time();
        int64_t seek_almost(int64_t target_frame);
        int jump_to_frame(int64_t target_frame);
        int forward_until(int ind, int64_t target_frame);
        void init_qimage(QImage * img, int H, int W);

    private slots:
        void set_progress_start();
        void set_progress_end();
        void progress_change();

    public:
        ImgLabel(QWidget *parent, QLabel *info, QSlider *progress);
        ~ImgLabel();
        void set_movie(std::string path);
        void play();
        void pause();
        void set_play_times(double x);
        void set_frame(int frame_index);
        int get_W();
        int get_H();
        void fast_forward_frame(int delta);
        void fast_forward_second(double delta);

    protected:
        void mouseMoveEvent(QMouseEvent *event);
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void paintEvent(QPaintEvent *event);
        void getRelativeXY(int px, int py, int * x, int * y);


};


#endif // IMGLABEL_H
