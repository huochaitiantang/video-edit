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
#include "libswresample/swresample.h"

}

#include<string>
#include<QImage>
#include<vector>
#include<queue>
#include<chrono>
#include<thread>
#include<QMutex>
#include<QTime>
#include<QCoreApplication>
#include <QAudioOutput>
#include <QAudioFormat>

class Movie
{
private:
    std::string movie_name = "";
    int height, width;
    AVRational video_fps, video_timebase, audio_timebase;
    double duration;
    int audio_sample_rate = 48000;
    int audio_sample_size = 16;
    int audio_channel = 2;
    int audio_layout_channel = 2;

    AVFormatContext * format_ctx;
    AVFrame * rgb_frame = NULL;
    AVFrame * frame = NULL;
    AVPacket * packet = NULL;
    SwsContext* sws_context = NULL;
    SwrContext* swr_context = NULL;

    int n_streams = 0;
    std::vector<int> video_stream_indexs;
    std::vector<int> audio_stream_indexs;
    int video_stream_index;
    int audio_stream_index;
    std::vector<AVCodec*> codecs;
    std::vector<AVCodecContext*> codec_contexts;
    std::vector<AVFrame*> video_frames;
    std::vector<AVFrame*> audio_frames;
    int video_frame_ind = 0;
    int audio_frame_ind = 0;

    double base_sys_ms;
    double base_pts_ms;

    QMutex mutex;
    char *pcm_buf = new char[48000];
    int pcm_len = 0;
    double play_times = 1.0;

    void clear_frame_vectors(std::vector<AVFrame*> &vs);

    void parse_video_codec(int stream_ind, AVCodecParameters* codec_parameters);
    void parse_audio_codec(int stream_ind, AVCodecParameters* codec_parameters);

    void write_rgb_frame();
    void write_audio_frame();

    double audio_pts_to_ms(int64_t pts);
    double video_pts_to_ms(int64_t pts);

    void decode_one_packet();
    bool fetch_some_packets();

    void first_display();
    int search_video_frame_by_ms(double millsecs);
    int search_audio_frame_by_ms(double millsecs);
    bool hard_seek(double millsecs);

public:
    Movie();
    ~Movie();
    void init(std::string move_path);
    void init_rgb_frame(int h, int w);

    void write_qimage(QImage * img, int top_h, int top_w);
    void write_qaudio(QIODevice * audio_io);

    int get_width();
    int get_height();
    std::string get_movie_name();

    double get_duration();
    double get_video_fps();

    int get_audio_sample_channel();
    int get_audio_layout_channel();
    int get_audio_sample_size();
    int get_audio_sample_rate();

    bool play_video_frame();
    bool play_audio_frame();
    double get_video_frame_ms();
    double get_audio_frame_ms();

    bool seek(double millsecs);

    void adjust_video_frames();
    void adjust_audio_frames();
    void restart();
    void set_play_times(double x);

};

#endif // MOVIE_H
