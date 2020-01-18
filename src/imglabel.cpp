#include "imglabel.h"
#include <iostream>

ImgLabel::ImgLabel(QWidget *parent, QLabel *info) : QLabel(parent){
    this->info = info;
    this->setGeometry(20, 40, this->W, this->H);
    this->info->setGeometry(20, 40 + this->H, this->W, 20);
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
}

void ImgLabel::clear_movie(){
    if(this->movie){
        delete this->movie;
        this->movie = NULL;
    }
}


bool ImgLabel::display_next_frame(){
    if((this->movie) && (this->movie->next_video_frame())){

        // TODO: still a little memory leak
        this->movie->write_qimage(image, top_h, top_w);
        this->setPixmap(QPixmap::fromImage(*(this->image)));

        int ind = movie->get_video_frame_index();
        double stamp = movie->get_video_frame_timestamp();
        this->info->setText(QString("frame index: %1, stamp: %2 s").arg(ind).arg(stamp));
        return true;
    }
    else return false;
}


void ImgLabel::getRelativeXY(int px, int py, int * x, int * y){
    *x = px - 20;
    *y = py - 40;
    if( *x < 0 ) *x = 0;
    if( *y < 0 ) *y = 0;
    if( *x >= this->W) *x = this->W - 1;
    if( *y >= this->H) *y = this->H - 1;
}

void ImgLabel::mouseMoveEvent(QMouseEvent *event){
    /*
    int abs_x = event->x();
    int abs_y = event->y();
    int x,y;
    getRelativeXY(abs_x, abs_y, &x, &y);
    char ss[100];
    sprintf(ss, "x=%d, y=%d", x, y);
    QString qss(ss);
    this->info->setText(qss);
    */
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
    display_next_frame();
}

