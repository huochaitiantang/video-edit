#include "imglabel.h"
#include <iostream>
#include<sys/timeb.h>
#include<windows.h>

ImgLabel::ImgLabel(QWidget *parent, QLabel *info, QSlider *progress) : QLabel(parent){
    this->info = info;
    this->progress = progress;
    this->setGeometry(10, 30, this->W, this->H);
    this->info->setGeometry(10, 30 + this->H, this->W, 20);
    this->progress->setGeometry(10, 50 + this->H, this->W, 20);
}


ImgLabel::~ImgLabel(){
    if(this->image != NULL){
        delete this->image;
    }
    clear_movie();
}

void ImgLabel::init_qimage(QImage * img, int H, int W){
    int h, w, base;
    unsigned char * line_data;
    for(h = 0; h < H; h++){
        line_data = img->scanLine(h);
        for(w = 0; w < W; w++){
            line_data[w * 3 + 0] = 53;
            line_data[w * 3 + 1] = 53;
            line_data[w * 3 + 2] = 53;
        }
    }
    return;
}

void ImgLabel::set_movie(std::string path){
    clear_movie();

    this->movie = new Movie();
    this->movie->init(path);

    // init qimage
    int h = movie->get_height();
    int w = movie->get_width();

    double ratio_h = (double)H / h;
    double ratio_w = (double)W / w;
    double ratio = ratio_h > ratio_w ? ratio_w : ratio_h;

    this->image_h = (int)(h * ratio);
    this->image_w = (int)(w * ratio);
    this->top_h = (H - image_h) / 2;
    this->top_w = (W - image_w) / 2;

    if(this->image){
        delete this->image;
    }

    this->image = new QImage(W, H, QImage::Format_RGB888);
    init_qimage(this->image, H, W);
    this->movie->init_rgb_frame(image_h, image_w);

    // init the qaudio
    QAudioFormat audio_fmt;
    audio_fmt.setSampleRate(movie->get_audio_sample_rate());
    audio_fmt.setSampleSize(16);
    audio_fmt.setChannelCount(movie->get_audio_sample_channel());
    audio_fmt.setCodec("audio/pcm");
    audio_fmt.setByteOrder(QAudioFormat::LittleEndian);
    audio_fmt.setSampleType(QAudioFormat::UnSignedInt);

    audio_output = new QAudioOutput(audio_fmt, this);
    audio_io = audio_output->start();

    // init the progress slider
    movie_duration = movie->get_duration() * 1000; // ms
    this->progress->setMinimum(0);
    this->progress->setMaximum((long long)(movie_duration));
    std::cout << "Slider Max: " << (long long)(movie_duration) << std::endl;

    // sliderPressed(), sliderReleased(), sliderMoved(int), valueChanged(int)
    connect(this->progress, SIGNAL(sliderPressed()), this, SLOT(set_progress_start()));
    connect(this->progress, SIGNAL(sliderReleased()), this, SLOT(set_progress_end()));
    connect(this->progress, SIGNAL(valueChanged(int)), this, SLOT(progress_change()));

    // play signal connect
    fetch_frame_thread = new FetchFrameThread(this);
    fetch_frame_thread->set_movie(movie);

    play_video_thread = new PlayVideoThread(this);
    play_video_thread->set_movie(movie);
    connect(play_video_thread, SIGNAL(play_one_frame_over()), this, SLOT(update_image()));

    play_audio_thread = new PlayAudioThread(this);
    play_audio_thread->set_movie(movie);
    connect(play_audio_thread, SIGNAL(play_one_frame_over()), this, SLOT(update_audio()));

    update_image();
}

void ImgLabel::clear_movie(){
    if(this->audio_output != NULL){
        audio_output->bytesFree();
        delete audio_output;
    }
    if(fetch_frame_thread != NULL){
        fetch_frame_thread->quit();
        fetch_frame_thread->wait();
        delete fetch_frame_thread;
    }
    // for dead lock
    if(on_play == false){
        play_video_thread->resume();
        play_audio_thread->resume();
        first_play_click = true;
    }
    if(play_video_thread != NULL){
        play_video_thread->quit();
        play_video_thread->wait();
        delete play_video_thread;
    }
    if(play_audio_thread != NULL){
        play_audio_thread->quit();
        play_audio_thread->wait();
        delete play_audio_thread;
    }
    if(this->movie){
        delete this->movie;
        this->movie = NULL;
        this->progress->setValue(0);
    }
}

void ImgLabel::update_image(){
    this->movie->write_qimage(image, top_h, top_w);
    this->setPixmap(QPixmap::fromImage(*(this->image)));
    double stamp = this->movie->get_video_frame_ms();
    this->progress->setValue((long long)stamp);
    this->update();
}


void ImgLabel::update_audio(){
    this->movie->write_qaudio(audio_io);
    double stamp = this->movie->get_audio_frame_ms();
    this->progress->setValue((long long)stamp);
    this->update();
}


std::string ImgLabel::format_time(double second){
    int int_second = (int)second;
    int h = int_second / 3600;
    int m = (int_second % 3600) / 60;
    int s = int_second % 60;
    int ms = (int)((second - int_second) * 100);

    char ss[16];
    assert(h < 100);
    sprintf(ss, "%02d:%02d:%02d.%02d", h, m, s, ms);
    std::string str = std::string(ss);
    return str;
}

void ImgLabel::set_progress_start(){
    display_lock = true;
}

void ImgLabel::set_progress_end(){
    double target_ms = (double)(this->progress->value());
    set_second(target_ms);
    display_lock = false;
}

void ImgLabel::progress_change(){
    double cur_secs = double(this->progress->value()) / 1000;
    double all_secs = movie_duration / 1000;
    this->info->setText(QString::fromStdString(format_time(cur_secs)) + " / " +
                        QString::fromStdString(format_time(all_secs)) + " [" +
                        QString("%1 / %2]").arg(cur_secs).arg(all_secs)
                        );
    return;
}

void ImgLabel::play(){
    on_play = true;
    this->movie->restart();

    if(first_play_click != true){
        audio_output->resume();
        play_video_thread->resume();
        play_audio_thread->resume();
    }
    else{
        first_play_click = false;
    }
    this->update();
}

void ImgLabel::pause(){
    on_play = false;
    audio_output->suspend();
    play_video_thread->pause();
    play_audio_thread->pause();
    this->update();
}

void ImgLabel::set_play_times(double x){
    this->play_times = x;
    this->movie->set_play_times(x);
}

int ImgLabel::get_H(){
    return this->H;
}

int ImgLabel::get_W(){
    return this->W;
}

void ImgLabel::fast_forward_second(double delta){
    double target_ms = (double)(this->progress->value()) + delta * 1000;
    set_second(target_ms);
}

void ImgLabel::set_second(double target_ms){
    if(target_ms < 0 || target_ms > movie_duration){
        return;
    }
    display_lock = true;
    if(on_play){
        play_video_thread->pause();
        play_audio_thread->pause();
    }
    this->movie->seek(target_ms);
    update_image();
    if(on_play){
        play_video_thread->resume();
        play_audio_thread->resume();
    }
    display_lock = false;
}

void ImgLabel::mouseMoveEvent(QMouseEvent *event){
    return;
}

void ImgLabel::mousePressEvent(QMouseEvent *event){
    return;
}

void ImgLabel::mouseReleaseEvent(QMouseEvent *event){
    return;
}

void ImgLabel::paintEvent(QPaintEvent *event){
    QLabel::paintEvent(event);

    if(fetch_frame_thread){
        fetch_frame_thread->start();
    }
    if(on_play && !display_lock && play_video_thread && play_audio_thread){
        play_video_thread->start();
        play_audio_thread->start();
    }
}
