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

class Movie
{
private:
    std::string movie_name;
    int height,width;
    double fps;

    int video_stream_index, audio_stream_index;
    AVFormatContext * format_ctx;

    AVCodec * video_codec = NULL;
    AVCodec * audio_codec = NULL;

    AVFrame * video_frame = NULL;
    AVFrame * audio_frame = NULL;

    AVPacket * video_packet = NULL;
    AVPacket * audio_packet = NULL;

    AVCodecContext * video_codec_ctx = NULL;
    AVCodecContext * audio_codec_ctx = NULL;

    AVCodecParameters * video_codec_parameters = NULL;
    AVCodecParameters * audio_codec_parameters = NULL;

    int next_video_packet();
    int next_video_frame();


public:
    Movie();
    ~Movie();
    void Movie::init(std::string move_path);

};

#endif // MOVIE_H
