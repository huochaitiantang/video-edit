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
#include <QComboBox>
#include <QPushButton>
#include "movie.h"
#include "fetchframethread.h"

class ImgLabel : public QLabel
{

    Q_OBJECT
    private:
        Movie* movie = NULL;
        QImage * image = NULL;
        QLabel * play_info = NULL;
        QSlider * progress = NULL;
        QPushButton * open_button = NULL;
        QPushButton * control_button = NULL;
        QComboBox * play_times_box = NULL;
        QLineEdit* jump_text = NULL;
        QPushButton* jump_button = NULL;

        std::vector<double> play_times = {1.0, 0.25, 0.5, 0.75, 1.25, 1.5, 2.0, 2.5, 3.0, 5.0};
        std::vector<double> fast_forwards = {-20, -5, -1, -0.2, -0.05, 0.05, 0.2, 1, 5, 20};

        int W = 720;
        int H = 360;
        int image_h, image_w, top_h, top_w;
        bool display_lock = false;
        bool on_play = true;
        double movie_duration;

        QAudioOutput *audio_output = NULL;
        QIODevice *audio_io = NULL;
        QAudioFormat audio_fmt;

        FetchFrameThread* fetch_frame_thread = NULL;
        PlayVideoThread* play_video_thread = NULL;
        PlayAudioThread* play_audio_thread = NULL;

        bool first_play_click = false;
        bool display_next_frame();
        void clear_movie();
        std::string format_time(double second);
        void init_qimage(QImage * img, int H, int W);
        void set_second(double target_ms);

    private slots:
        void set_progress_start();
        void set_progress_end();
        void progress_change();
        void update_image();
        void update_audio();
        void click_open();
        void click_control();
        void play_times_changed();
        void click_jump();
        void fast_forward_clicked(int index);

    public:
        ImgLabel(QWidget *parent, int start_w, int start_h, int w, int h);
        ~ImgLabel();
        void set_movie(std::string path);
        int get_W();
        int get_H();

    protected:
        void mouseMoveEvent(QMouseEvent *event);
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void paintEvent(QPaintEvent *event);
        void getRelativeXY(int px, int py, int * x, int * y);


};


#endif // IMGLABEL_H
