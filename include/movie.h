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
    //int video_frame_index;
    double duration;
    //int64_t current_pts;
    //int64_t pts_per_frame;

    int audio_sample_rate = 48000;
    int audio_sample_size = 16;
    int audio_channel = 2;

    AVFormatContext * format_ctx;
    AVFrame * rgb_frame = NULL;
    AVFrame * frame = NULL;
    AVFrame * video_frame = NULL;
    AVFrame * audio_frame = NULL;
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

    //QMutex mutex;
    char *pcm_buf = new char[48000 * 4 * 2];
    char pcm_len = 0;

    void clear_frame_vectors(std::vector<AVFrame*> vs);
    void adjust_frame_vectors();

    void parse_video_codec(int stream_ind, AVCodecParameters* codec_parameters);
    void parse_audio_codec(int stream_ind, AVCodecParameters* codec_parameters);

    void write_rgb_frame(AVFrame* video_frame);
    void write_audio_frame(AVFrame * audio_frame);

    double audio_pts_to_ms(int64_t pts);
    double video_pts_to_ms(int64_t pts);

    void decode_one_packet();
    bool fetch_some_packets();



public:
    Movie();
    ~Movie();
    void init(std::string move_path);
    void init_rgb_frame(int h, int w);

    void write_qimage(QImage * img, int top_h, int top_w);
    void write_qaudio(QIODevice * audio_io);
    int get_width();
    int get_height();
    //int get_video_frame_index();
    std::string get_movie_name();
    //double get_video_frame_timestamp();
    double get_duration();
    double get_video_fps();
    //int get_frame_count();
    //int64_t get_max_pts();
    bool next_frame();
    void seek_frame(int64_t target_frame);

    int get_audio_sample_channel();
    int get_audio_sample_size();
    int get_audio_sample_rate();

};

#endif // MOVIE_H
