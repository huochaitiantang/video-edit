
#include "include/mainwindow.h"
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
    imglabel = new ImgLabel(this, ui->img_label_info);
    this->adjust_size();

    char movie_name[64] = "E:/QT/videos/S2-P5-160912.mp4";
    unsigned char** data; // TODO: vector
    int channel, height, width;

    data = (unsigned char**)malloc(sizeof(unsigned char*));
    read_one_frame(movie_name, data, &channel, &height, &width);
    printf("read channel=%d height=%d width=%d\n", channel, height, width);

    int base = height * width * channel;
    printf("RGB : %d %d %d\n", (*data)[0], (*data)[1], (*data)[2]);
    printf("RGB : %d %d %d\n", (*data)[base - 3], (*data)[base - 2], (*data)[base - 1]);

    this->imglabel->set_image(channel, height, width, *data);
    this->imglabel->set_pixel();
    this->imglabel->info->setText("Hello world!");


    free(*data);
    free(data);


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


