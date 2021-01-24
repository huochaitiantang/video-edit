#include "imglabel.h"
#include <iostream>
#include<sys/timeb.h>
#include<windows.h>
#include<QPushButton>
#include<QLineEdit>

ImgLabel::ImgLabel(QWidget *parent, int start_w, int start_h, int w, int h) : QLabel(parent){
    if(w > 0) W = w;
    if(h > 0) H = h;
    int item_h = 20;
    this->setGeometry(start_w, start_h, W, H);

    // play progress slider
    progress = new QSlider(parent);
    progress->setOrientation(Qt::Horizontal);
    progress->setGeometry(start_w, start_h + H + item_h * 0, W, item_h);

    // current time / all time
    play_info = new QLabel(parent);
    play_info->setGeometry(start_w, start_h + H + item_h * 1, W, item_h);

    // open movie button
    open_button = new QPushButton(parent);
    open_button->setText("Open");
    connect(open_button, SIGNAL(clicked()), this, SLOT(click_open()));
    open_button->setGeometry(start_w + W / 5 * 0, start_h + H + item_h * 2, W / 5, item_h);

    // play & pause buttong
    control_button = new QPushButton(parent);
    control_button->setText("Pause");
    connect(control_button, SIGNAL(clicked()), this, SLOT(click_control()));
    control_button->setGeometry(start_w + W / 5 * 1, start_h + H + item_h * 2, W / 5, item_h);

    // movie play times
    play_times_box = new QComboBox(parent);
    for(int i = 0; i < play_times.size(); i++)
        play_times_box->addItem(QString("%1x").arg(play_times[i]), play_times[i]);
    play_times_box->setCurrentIndex(0);
    connect(play_times_box, SIGNAL(currentIndexChanged(int)), this, SLOT(play_times_changed()));
    play_times_box->setGeometry(start_w + W / 5 * 2, start_h + H + item_h * 2, W / 5, item_h);

    // jump button to any frame
    jump_text = new QLineEdit(parent);
    jump_button = new QPushButton(parent);
    jump_button->setText("Jump");
    connect(jump_button, SIGNAL(clicked()), this, SLOT(click_jump()));
    jump_text->setGeometry(start_w + W / 5 * 3, start_h + H + item_h * 2, W / 5, item_h);
    jump_button->setGeometry(start_w + W / 5 * 4, start_h + H + item_h * 2, W / 5, item_h);

    // fast forward for second
    int avg_w = W / (fast_forwards.size() + 1);
    QButtonGroup * fast_forward_button_group = new QButtonGroup(parent);
    connect(fast_forward_button_group,SIGNAL(buttonClicked(int)), this, SLOT(fast_forward_clicked(int)));

    QLabel* second_fast_forward_info = new QLabel(parent);
    second_fast_forward_info->setText(" Adjust:");
    second_fast_forward_info->setGeometry(start_w, start_h + H + item_h * 3, avg_w, item_h);

    // add button
    for(int i = 0; i < fast_forwards.size(); i++){
        QPushButton * btn = new QPushButton(parent);
        double val = fast_forwards[i];
        QString ss = val > 0 ? QString("+%1 s").arg(val) : QString("%1 s").arg(val);
        btn->setText(ss);
        btn->setGeometry(start_w + avg_w * (i + 1), start_h + H + item_h * 3, avg_w, item_h);
        fast_forward_button_group->addButton(btn, i);
    }
}

ImgLabel::~ImgLabel(){
    if(image) delete image;
    clear_movie();
    if(play_info) delete play_info;
    if(progress) delete progress;
    if(open_button) delete open_button;
    if(control_button) delete control_button;
    if(play_times_box) delete play_times_box;
    if(jump_text) delete jump_text;
    if(jump_button) delete jump_button;
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
    fetch_frame_thread->start();

    play_video_thread = new PlayVideoThread(this);
    play_video_thread->set_movie(movie);
    connect(play_video_thread, SIGNAL(play_one_frame_over()), this, SLOT(update_image()));
    // play_video_thread->start();

    play_audio_thread = new PlayAudioThread(this);
    play_audio_thread->set_movie(movie);
    connect(play_audio_thread, SIGNAL(play_one_frame_over()), this, SLOT(update_audio()));
    // play_audio_thread->start();

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
    //double stamp = this->movie->get_audio_frame_ms();
    //this->progress->setValue((long long)stamp);
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
    play_info->setText(" " + QString::fromStdString(format_time(cur_secs)) + " / " +
                        QString::fromStdString(format_time(all_secs)) + " [" +
                        QString("%1 / %2]").arg(cur_secs).arg(all_secs)
                        );
    jump_text->setText(QString("%1").arg(cur_secs));
    return;
}

int ImgLabel::get_H(){
    return this->H;
}

int ImgLabel::get_W(){
    return this->W;
}

void ImgLabel::set_second(double target_ms){
    if(!movie || target_ms < 0 || target_ms > movie_duration){
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

void ImgLabel::click_open(){
    // parent, caption, default dir, filter
    QString qpath = QFileDialog::getOpenFileName(this, tr("Chose a Movie"), "../", tr("Movie (*.mp4 *.mkv *.avi)"));
    if(qpath.isEmpty()) return;
    set_movie(qpath.toStdString());
}

void ImgLabel::click_control(){
    if(!movie) return;
    if(control_button->text() == "Play"){
        control_button->setText("Pause");
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
    }
    else{
        control_button->setText("Play");
        on_play = false;
        audio_output->suspend();
        play_video_thread->pause();
        play_audio_thread->pause();
    }
    this->update();

}

void ImgLabel::play_times_changed(){
    if(!movie) return;
    int ind = play_times_box->currentIndex();
    std::cout << "Play times changed:" << play_times[ind] << std::endl;

    if(play_times[ind] < 1){
        audio_fmt.setSampleRate(movie->get_audio_sample_rate() * play_times[ind]);
    }
    else{
        audio_fmt.setSampleRate(movie->get_audio_sample_rate());
    }
    this->movie->set_play_times(play_times[ind]);
}

void ImgLabel::click_jump(){
    if(!movie) return;
    QString val = jump_text->text();
    if(val.contains(QRegExp("^\\d+\.*\\d*$"))){
        set_second(val.toDouble() * 1000);
    }
}

void ImgLabel::fast_forward_clicked(int index){
    if(!movie) return;
    double target_ms = (double)(this->progress->value()) + fast_forwards[index] * 1000;
    set_second(target_ms);
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

//    if(fetch_frame_thread){
//        fetch_frame_thread->start();
//    }
    if(on_play && !display_lock && play_video_thread && play_audio_thread){
        play_video_thread->start();
        play_audio_thread->start();
    }
}
