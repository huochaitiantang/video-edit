
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
    imglabel = new ImgLabel(this, ui->img_label_info);
    this->adjust_size();

    std::string path = "E:/QT/videos/S2-P5-160912.mp4";
    imglabel->set_movie(path);

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


