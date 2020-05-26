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
    delete movie;
}

double ImgLabel::get_system_time(){
    timeb t;
    ftime(&t);
    return (double)(t.time * 1000 + t.millitm) / 1000.0;
}

void ImgLabel::set_movie(std::string path){
    clear_movie();

    this->movie = new Movie();
    this->movie->init(path);

    int h = movie->get_height();
    int w = movie->get_width();

    double ratio_h = (double)H / h;
    double ratio_w = (double)W / w;
    //double ratio = std::min(ratio_h, ratio_w);
    double ratio = ratio_h > ratio_w ? ratio_w : ratio_h;

    this->image_h = (int)(h * ratio);
    this->image_w = (int)(w * ratio);
    this->top_h = (H - image_h) / 2;
    this->top_w = (W - image_w) / 2;

    if(this->image){
        delete this->image;
    }

    this->image = new QImage(W, H, QImage::Format_RGB888);
    this->movie->init_rgb_frame(image_h, image_w);

    // init the progress slider
    movie_duration = movie->get_video_duration();
    movie_fps = movie->get_fps();
    movie_frame_count = movie->get_frame_count();
    this->progress->setMinimum(0);
    this->progress->setMaximum(movie_frame_count);
    std::cout << "Slider Max: " << movie_frame_count << std::endl;

    // sliderPressed(), sliderReleased(), sliderMoved(int), valueChanged(int)
    connect(this->progress, SIGNAL(sliderPressed()), this, SLOT(set_progress_start()));
    connect(this->progress, SIGNAL(sliderReleased()), this, SLOT(set_progress_end()));
    connect(this->progress, SIGNAL(valueChanged(int)), this, SLOT(progress_change()));

    display_next_frame();
}

void ImgLabel::clear_movie(){
    if(this->movie){
        delete this->movie;
        this->movie = NULL;
        this->progress->setValue(0);
        disconnect(this->progress);
    }
}

bool ImgLabel::display_next_frame(){
    if((this->movie) && (this->movie->next_video_frame())){
        this->movie->write_qimage(image, top_h, top_w);
        this->setPixmap(QPixmap::fromImage(*(this->image)));

        int ind = this->movie->get_video_frame_index();
        this->progress->setValue(ind);
        return true;
    }
    else return false;
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

// seek the key frame that near the target frame index
int64_t ImgLabel::seek_almost(int64_t target_frame){
    this->movie->seek_frame(target_frame);
    display_next_frame();
    int ind = movie->get_video_frame_index();
    assert(abs(target_frame - ind) <= 300);
    return ind;
}

// accurate jump, may cost much
void ImgLabel::jump_to_frame(int64_t target_frame){
    int tmp_target_frame = target_frame;
    int ind = seek_almost(tmp_target_frame);

    // found key frame with the bigger frame index
    while(ind > target_frame){
        std::cout << "Warning: Seek Key Frame = " << ind << " > Target Frame:" << target_frame << std::endl;
        tmp_target_frame = tmp_target_frame < 50 ? 0 : tmp_target_frame - 50;
        ind = seek_almost(tmp_target_frame);
    }

    // found key frame with the smaller frame index
    while((ind < target_frame) && (display_next_frame())){
        ind = movie->get_video_frame_index();
    }
    if(ind < target_frame){
        std::cout << "Warning: Real Max Frame Index = " << ind << std::endl;
    }
    std::cout << "Seek Frame: Target=" << target_frame << ", Real=" << ind << std::endl;

    return;
}

void ImgLabel::set_progress_start(){
    display_lock = true;
}

void ImgLabel::set_progress_end(){
    int64_t target_frame = this->progress->value();
    //jump_to_frame(target_frame);
    seek_almost(target_frame);
    display_lock = false;
}

void ImgLabel::progress_change(){
    int64_t target_frame = this->progress->value();
    double show_stamp = (double)target_frame / movie_fps;
    this->info->setText(QString::fromStdString(format_time(show_stamp)) + " / " +
                        QString::fromStdString(format_time(movie_duration)) + " " +
                        QString("Frame[%1 / %2]").arg(target_frame).arg(movie_frame_count));
    return;
}

void ImgLabel::play(){
    on_play = true;
    this->update();
}

void ImgLabel::pause(){
    on_play = false;
    this->update();
}

void ImgLabel::set_play_times(double x){
    this->play_times = x;
}

int ImgLabel::get_H(){
    return this->H;
}

int ImgLabel::get_W(){
    return this->W;
}

void ImgLabel::set_frame(int frame_index){
    if(frame_index < 0 || frame_index > movie_frame_count) return;
    display_lock = true;
    jump_to_frame(frame_index);
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
    if(!display_lock && on_play){
        timeb t1, t2;
        ftime(&t1);
        display_next_frame();
        ftime(&t2);
        double dt = (double)(t2.time - t1.time) * 1000 + (double)(t2.millitm - t1.millitm);
        double sleep_dt = 900.0 / movie_fps / play_times - dt;
        sleep_dt = sleep_dt < 0 ? 0 : sleep_dt;
        Sleep(sleep_dt);
    }

}
