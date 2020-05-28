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
    void set_movie(Movie* m);

private:
    Movie* movie = NULL;

//signals:
    //void message(const QString& info);

};
