#include "fetchframethread.h"
#include<sys/timeb.h>
#include<windows.h>

FetchFrameThread::FetchFrameThread(QObject* parent) : QThread(parent){

}

PlayVideoThread::PlayVideoThread(QObject* parent) : QThread(parent){

}

PlayAudioThread::PlayAudioThread(QObject* parent) : QThread(parent){

}

FetchFrameThread::~FetchFrameThread(){

}

PlayVideoThread::~PlayVideoThread(){

}

PlayAudioThread::~PlayAudioThread(){

}

void FetchFrameThread::set_movie(Movie* m){
    this->movie = m;
}

void PlayVideoThread::set_movie(Movie *m){
    this->movie = m;
}

void PlayAudioThread::set_movie(Movie *m){
    this->movie = m;
}

void FetchFrameThread::run(){
    if(this->movie){
        // mutex_fetch.lock();
        this->movie->fetch_frames();
        // mutex_fetch.unlock();
    }
}

void PlayVideoThread::run(){
    if(this->movie){
        mutex_video.lock();
        if(this->movie->play_video_frame()){
            emit play_one_frame_over();
        }
        mutex_video.unlock();
    }
}

void PlayAudioThread::run(){
    if(this->movie){
        mutex_audio.lock();
        if(this->movie->play_audio_frame()){
            emit play_one_frame_over();
        }
        mutex_audio.unlock();
    }
}

void FetchFrameThread::pause(){
    // mutex_fetch.lock();
}

void PlayVideoThread::pause(){
    mutex_video.lock();
}

void PlayAudioThread::pause(){
    mutex_audio.lock();
}

void FetchFrameThread::resume(){
    // mutex_fetch.unlock();
}

void PlayVideoThread::resume(){
    mutex_video.unlock();
}

void PlayAudioThread::resume(){
    mutex_audio.unlock();
}
