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
        while(true){

            mutex.lock();
            this->movie->adjust_audio_frames();
            this->movie->adjust_video_frames();
            mutex.unlock();
            Sleep(5);
        }
    }
}

void PlayVideoThread::run(){
    if(this->movie){
        //while(true){
            mutex.lock();
            if(this->movie->play_video_frame()){
                emit play_one_frame_over();
            }
            mutex.unlock();
        //}
    }
}

void PlayAudioThread::run(){
    if(this->movie){
        //while(true){
            mutex.lock();
            if(this->movie->play_audio_frame()){
                emit play_one_frame_over();
            }
            mutex.unlock();
        //}
    }
}

void FetchFrameThread::pause(){
    mutex.lock();
}

void PlayVideoThread::pause(){
    mutex.lock();
}

void PlayAudioThread::pause(){
    mutex.lock();
}

void FetchFrameThread::resume(){
    mutex.unlock();
}

void PlayVideoThread::resume(){
    mutex.unlock();
}

void PlayAudioThread::resume(){
    mutex.unlock();
}
