#ifndef MOVIE_H
#define MOVIE_H
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

#include<string>
#include<QImage>
#include<vector>
#include<queue>
#include<chrono>
#include<thread>
#include<QMutex>

class Movie
{
private:
    std::string movie_name = "";
    int height, width;
    AVRational fps, timebase;
    int video_frame_index;
    bool ret_packet = false;
    double duration;
    int64_t current_pts;
    int64_t pts_per_frame;

    int audio_sample_rate = 48000;
    int audio_sample_size = 16;
    int audio_channel = 2;

    AVFormatContext * format_ctx;
    AVFrame * rgb_frame = NULL;
    AVFrame * frame = NULL;
    AVPacket * packet = NULL;
    SwsContext* sws_context = NULL;

    int n_streams = 0;
    std::vector<int> video_stream_indexs;
    std::vector<int> audio_stream_indexs;
    int video_stream_index;
    int audio_stream_index;
    std::vector<AVCodec*> codecs;
    std::vector<AVCodecContext*> codec_contexts;
    //std::vector<std::queue<AVFrame*> > frame_queues;
    std::queue<AVFrame*> video_frame_queues;
    std::queue<AVFrame*> audio_frame_queues;
    int queue_max = 500;
    QMutex mutex;

    void parse_video_codec(int stream_ind, AVCodecParameters* codec_parameters);
    void parse_audio_codec(int stream_ind, AVCodecParameters* codec_parameters);
    void thread_sleep(std::chrono::microseconds us);

    bool next_video_packet();
    void write_rgb_frame(AVFrame* video_frame);
    void clear_frame_queues(std::queue<AVFrame*> queues);


public:
    Movie();
    ~Movie();
    void init(std::string move_path);
    void init_rgb_frame(int h, int w);
    bool next_video_frame();
    void write_qimage(QImage * img, int top_h, int top_w);
    int get_width();
    int get_height();
    int get_video_frame_index();
    std::string get_movie_name();
    double get_video_frame_timestamp();
    double get_video_duration();
    double get_fps();
    int get_frame_count();
    int64_t get_max_pts();
    void seek_frame(int64_t target_frame);
    void fetch_frame();

};

#endif // MOVIE_H
