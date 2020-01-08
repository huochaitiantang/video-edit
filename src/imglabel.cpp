#include "imglabel.h"


ImgLabel::ImgLabel(QWidget *parent, QLabel *info) : QLabel(parent){
    this->info = info;
    this->setGeometry(20, 40, this->W, this->H);
    this->info->setGeometry(20, 40 + this->H, this->W, 20);
}


ImgLabel::~ImgLabel(){
    if(this->image != NULL){
        delete this->image;
    }
}



void ImgLabel::set_image(int channel, int height, int width, unsigned char * data){

    if(this->image == NULL){
        QImage * tmpImage = new QImage(width, height, QImage::Format_RGB888);
        this->image = tmpImage;
    }

    if(this->image->width() != width || this->image->height() != height){
        delete this->image;
        QImage * tmpImage = new QImage(width, height, QImage::Format_RGB888);
        this->image = tmpImage;
    }

    for(int h = 0; h < height; h++){
        for(int w = 0; w < width; w++){
            int base = (h * width + w) * 3;
            this->image->setPixel(w, h, qRgb(data[base], data[base+1], data[base+2]));
        }
    }
    return;
}


void ImgLabel::set_pixel(){

    int h = this->image->height();
    int w = this->image->width();
    int H = this->H;
    int W = this->W;

    double ratio_h = (double)H / h;
    double ratio_w = (double)W / w;
    double ratio = std::min(ratio_h, ratio_w);
    int new_h = (int)(h * ratio);
    int new_w = (int)(w * ratio);

    QImage tmpImage = this->image->scaled(this->W, this->H, Qt::KeepAspectRatioByExpanding);
    //this->setPixmap(QPixmap::fromImage(*this->image));
    this->setPixmap(QPixmap::fromImage(tmpImage));

    return;
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
    int abs_x = event->x();
    int abs_y = event->y();
    int x,y;
    getRelativeXY(abs_x, abs_y, &x, &y);
    char ss[100];
    sprintf(ss, "x=%d, y=%d", x, y);
    QString qss(ss);
    this->info->setText(qss);
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
}

