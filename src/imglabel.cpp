#include "imglabel.h"
#include <iostream>

ImgLabel::ImgLabel(QWidget *parent, QLabel *info, QSlider *progress) : QLabel(parent){
    this->info = info;
    this->progress = progress;
    this->setGeometry(20, 40, this->W, this->H);
    this->progress->setGeometry(20, 40 + this->H, this->W, 20);
    this->info->setGeometry(20, 60 + this->H, this->W, 20);
}


ImgLabel::~ImgLabel(){
    if(this->image != NULL){
        delete this->image;
    }
    delete movie;
}


void ImgLabel::set_movie(std::string path){
    clear_movie();

    this->movie = new Movie();
    this->movie->init(path);

    int h = movie->get_height();
    int w = movie->get_width();

    double ratio_h = (double)H / h;
    double ratio_w = (double)W / w;
    double ratio = std::min(ratio_h, ratio_w);

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
    int duration = (int)movie->get_video_duration();
    this->progress->setMinimum(0);
    this->progress->setMaximum(duration);
    std::cout << "Slider Max: " << duration << std::endl;
    // sliderPressed(), sliderReleased(), sliderMoved(int), valueChanged(int)
    connect(this->progress, SIGNAL(sliderPressed()), this, SLOT(set_progress_start()));
    connect(this->progress, SIGNAL(sliderReleased()), this, SLOT(set_progress_end()));

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

        // TODO: still a little memory leak
        this->movie->write_qimage(image, top_h, top_w);
        this->setPixmap(QPixmap::fromImage(*(this->image)));

        format_progress();
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

void ImgLabel::format_progress(){
    // display the progress info
    int ind = movie->get_video_frame_index();
    double stamp = movie->get_video_frame_timestamp();
    double duration = movie->get_video_duration();

    this->info->setText(QString::fromStdString(format_time(stamp)) + " / " +
                        QString::fromStdString(format_time(duration)) + " " +
                        QString("Frame[%1]").arg(ind));

    // move the progress slider
    this->progress->setValue((int)stamp);
}

void ImgLabel::set_progress_start(){
    display_lock = true;
}

void ImgLabel::set_progress_end(){
    int target_second = this->progress->value();
    this->movie->seek_frame(target_second);
    display_next_frame();
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
    if(!display_lock){
        display_next_frame();
    }
}
