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

class Movie
{
private:
    std::string movie_name = "";
    int height,width;
    AVRational fps, timebase;
    int video_frame_index;
    bool ret_packet = false;
    double duration;

    int video_stream_index, audio_stream_index;
    AVFormatContext * format_ctx;

    AVCodec * video_codec = NULL;
    AVCodec * audio_codec = NULL;

    AVFrame * video_frame = NULL;
    AVFrame * audio_frame = NULL;
    AVFrame * rgb_frame = NULL;

    AVPacket * video_packet = NULL;
    AVPacket * audio_packet = NULL;

    AVCodecContext * video_codec_ctx = NULL;
    AVCodecContext * audio_codec_ctx = NULL;

    AVCodecParameters * video_codec_parameters = NULL;
    AVCodecParameters * audio_codec_parameters = NULL;

    SwsContext* sws_context = NULL;

    bool next_video_packet();
    void write_rgb_frame();


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
    void seek_frame(int target_second);

};

#endif // MOVIE_H
