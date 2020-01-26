
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
    ui->open_movie->setGeometry(20, 80 + 360, 540, 20);

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


