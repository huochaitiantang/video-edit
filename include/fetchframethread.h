#ifndef FETCHFRAMETHREAD_H
#define FETCHFRAMETHREAD_H

#endif // FETCHFRAMETHREAD_H

#include<QThread>
#include<movie.h>

class FetchFrameThread : public QThread
{
    Q_OBJECT

public:
    FetchFrameThread(QObject* parent);
    ~FetchFrameThread();
    void run();
    void pause();
    void resume();
    void set_movie(Movie* m);

private:
    Movie* movie = NULL;
    QMutex mutex;

//signals:
    //void message(const QString& info);

};



class PlayVideoThread : public QThread
{
    Q_OBJECT

public:
    PlayVideoThread(QObject* parent);
    ~PlayVideoThread();
    void run();
    void pause();
    void resume();
    void set_movie(Movie* m);

private:
    Movie* movie = NULL;
    QMutex mutex;

signals:
    void play_one_frame_over();

};



class PlayAudioThread : public QThread
{
    Q_OBJECT

public:
    PlayAudioThread(QObject* parent);
    ~PlayAudioThread();
    void run();
    void pause();
    void resume();
    void set_movie(Movie* m);

private:
    Movie* movie = NULL;
    QMutex mutex;

signals:
    void play_one_frame_over();

};

