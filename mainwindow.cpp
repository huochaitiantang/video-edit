// in case of 无法解析的外部符号 "struct AVFormatContext * __cdecl avformat_alloc_context(void)"
// add the shared dll to debug dir in case of crashed exit
extern "C" {
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"

}

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{


    // Allocate memory
    AVFormatContext *pFormatContext = avformat_alloc_context();
    if(!pFormatContext){
        printf("ERROR could not allocate memory for Format Context");
    }

    // open movie format
    char movie_name[64] = "E:/QT/videos/S2-P5-160912.mp4";
    if(avformat_open_input(&pFormatContext, movie_name, NULL, NULL) != 0){
        printf("ERROR could not open the file");
    }

    // load stream info
    if(avformat_find_stream_info(pFormatContext,  NULL) < 0){
       printf("ERROR could not get the stream info");
    }

    // initial avcodec variable
    AVCodec *pCodec = NULL;
    AVCodecParameters *pCodecParameters =  NULL;
    int video_stream_index = -1;

    // loop though all the streams and print its main information
    for (int i = 0; i < pFormatContext->nb_streams; i++){
        AVCodecParameters *pLocalCodecParameters =  NULL;
        pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        printf("AVStream->time_base before open coded %d/%d\n", pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
        printf("AVStream->r_frame_rate before open coded %d/%d\n", pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
        printf("AVStream->start_time % \n" PRId64, pFormatContext->streams[i]->start_time);
        printf("AVStream->duration % \n" PRId64, pFormatContext->streams[i]->duration);

        printf("finding the proper decoder (CODEC)\n");
    }


    ui->setupUi(this);
    ui->InfoLabel->setText("Hello world!");


}

MainWindow::~MainWindow()
{
    delete ui;
}


