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
#include <QAudioOutput>
#include <QAudioFormat>
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
        double movie_duration;
        double movie_fps;
        double play_times = 1.0;

        QAudioOutput *audio_output = NULL;
        QIODevice *audio_io = NULL;
        int audio_play_sample_rate = 48000;
        int audio_play_sample_size = 16;
        int audio_play_channel = 2;

        FetchFrameThread* fetch_frame_thread = NULL;
        PlayVideoThread* play_video_thread = NULL;
        PlayAudioThread* play_audio_thread = NULL;

        bool display_next_frame();
        void clear_movie();
        std::string format_time(double second);
        void init_qimage(QImage * img, int H, int W);

    private slots:
        void set_progress_start();
        void set_progress_end();
        void progress_change();
        void update_image();
        void update_audio();

    public:
        ImgLabel(QWidget *parent, QLabel *info, QSlider *progress);
        ~ImgLabel();
        void set_movie(std::string path);
        void play();
        void pause();

        int get_W();
        int get_H();
        void set_play_times(double x);
        void set_second(double target_ms);
        void fast_forward_second(double delta);

    protected:
        void mouseMoveEvent(QMouseEvent *event);
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void paintEvent(QPaintEvent *event);
        void getRelativeXY(int px, int py, int * x, int * y);


};


#endif // IMGLABEL_H
