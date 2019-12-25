
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "ffmpeg.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    imglabel = new ImgLabel(ui->img_label_info);


    char movie_name[64] = "E:/QT/videos/S2-P5-160912.mp4";
    unsigned char data[1280];
    int channel, height, width;

    read_one_frame(movie_name, data, &channel, &height, &width);
    printf("read channel=%d height=%d width=%d\n", channel, height, width);

    imglabel->info->setText("Hello world!");


}

MainWindow::~MainWindow()
{
    delete ui;
}


