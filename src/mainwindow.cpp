
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
    if(val.contains(QRegExp("^\\d+$"))){
        imglabel->set_frame(val.toInt());
    }
}
