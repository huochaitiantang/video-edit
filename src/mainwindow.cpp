
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
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect screenRect = desktopWidget->availableGeometry();
    int W = screenRect.width() * 0.6;
    int H = screenRect.height() * 0.6;
    this->resize(QSize(W, H));
    imglabel = new ImgLabel(this, 0, 0, W, H - 80);

    //for debug
    //std::string path = "E:/QT/videos/S2-P5-160912.mp4";
    //imglabel->set_movie(path);
}

MainWindow::~MainWindow()
{
    delete imglabel;
    delete ui;
}

