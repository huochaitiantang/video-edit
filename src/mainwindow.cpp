
#include "include/mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <iostream>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    imglabel = new ImgLabel(this, ui->img_label_info, ui->movie_progress);
    this->adjust_size();
    ui->open_movie->setGeometry(10, 10, 200, 20);

    int w = imglabel->get_W();
    int h = imglabel->get_H();
    ui->control->setGeometry(10, 80 + h, w / 4, 20);
    ui->play_times->setGeometry(10 + w / 4, 80 + h, w / 4, 20);
    ui->jump_frame_text->setGeometry(10 + w / 2, 80 + h, w / 4, 20);
    ui->jump_frame->setGeometry(10 + w * 3 / 4, 80 + h, w / 4, 20);

    // play times set
    for(int i = 0; i < cond_count; i++)
        ui->play_times->addItem(QString("%1x").arg(cond_times[i]), cond_times[i]);
    ui->play_times->setCurrentIndex(0);
    connect(this->ui->play_times, SIGNAL(currentIndexChanged(int)), this, SLOT(play_times_changed()));

    // fast forward
    QButtonGroup * fast_forward_button_group = new QButtonGroup(this);
    connect(fast_forward_button_group,SIGNAL(buttonClicked(int)), this, SLOT(fast_forward_clicked(int)));
    int info_w = 100;

    // fast forward for second
    QLabel* second_fast_forward_info = new QLabel(this);
    second_fast_forward_info->setText("ControlSecond:");
    second_fast_forward_info->setGeometry(10, 120 + h, info_w, 20);
    int avg_w = (w - info_w) / fast_forward_count;
    // add button
    for(int i = 0; i < fast_forward_count; i++){
        QPushButton * btn = new QPushButton(this);
        double val = fast_forward_cands[i];
        QString ss = val > 0 ? QString("+%1 s").arg(val) : QString("%1 s").arg(val);
        btn->setText(ss);
        btn->setGeometry(info_w + avg_w * i, 120 + h, avg_w, 20);
        fast_forward_button_group->addButton(btn, i);
    }

    //for debug
    //std::string path = "E:/QT/videos/S2-P5-160912.mp4";
    //imglabel->set_movie(path);
}

void MainWindow::adjust_size(){
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect screenRect = desktopWidget->availableGeometry();
    this->resize(QSize(screenRect.width(), screenRect.height()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_open_movie_clicked()
{
    // parent, caption, default dir, filter
    QString qpath = QFileDialog::getOpenFileName(this, tr("Chose a Movie"), "../", tr("Movie (*.mp4 *.mkv *.avi)"));
    if(qpath.isEmpty()) return;
    imglabel->set_movie(qpath.toStdString());
}

void MainWindow::on_control_clicked(){
    if(ui->control->text() == "Play"){
        imglabel->play();
        ui->control->setText("Pause");
    }
    else{
        imglabel->pause();
        ui->control->setText("Play");
    }
}

void MainWindow::play_times_changed(){
    int ind = ui->play_times->currentIndex();
    this->imglabel->set_play_times(cond_times[ind]);
    std::cout << "Play times changed:" << cond_times[ind] << std::endl;
}

void MainWindow::on_jump_frame_clicked(){
    QString val = ui->jump_frame_text->text();
    if(val.contains(QRegExp("^\\d+\.*\\d*$"))){
        imglabel->set_second(val.toDouble() * 1000);
    }
}

void MainWindow::fast_forward_clicked(int index){
    assert(index < fast_forward_count);
    this->imglabel->fast_forward_second(fast_forward_cands[index]);
}
