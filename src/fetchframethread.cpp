#include "fetchframethread.h"

FetchFrameThread::FetchFrameThread(QObject* parent) : QThread(parent){

}

FetchFrameThread::~FetchFrameThread(){
    if(this->movie) this->movie = NULL;
    QThread::~QThread();
}

void FetchFrameThread::set_movie(Movie* m){
    this->movie = m;
}

void FetchFrameThread::run(){
    if(this->movie){
        this->movie->fetch_frame();
    }
}
