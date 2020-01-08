#include<assert.h>
#include<iostream>
#include "movie.h"

Movie::Movie()
{
    format_ctx = avformat_alloc_context();
    video_frame = av_frame_alloc();
    audio_frame = av_frame_alloc();
    video_packet = av_packet_alloc();
    audio_packet = av_packet_alloc();
    assert(format_ctx && video_frame && audio_frame && video_packet && audio_packet);
}

Movie::~Movie()
{
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);
    av_packet_free(&video_packet);
    av_packet_free(&audio_packet);
    av_frame_free(&video_frame);
    av_frame_free(&audio_frame);

    if(video_codec_ctx) avcodec_free_context(&video_codec_ctx);
    if(audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
}

void Movie::init(std::string path)
{
    movie_name = path;
    assert(avformat_open_input(&format_ctx, path.c_str(), NULL, NULL));
    assert(avformat_find_stream_info(format_ctx,  NULL) >= 0);

    // find the video codec parameters and audio codec parameters
    for(int i = 0; i < format_ctx->nb_streams; i++)
    {
        AVCodecParameters *cur_codec_parameters =  NULL;
        cur_codec_parameters = format_ctx->streams[i]->codecpar;
        if(cur_codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = i;
            video_codec_parameters = video_codec_parameters != NULL?
                        video_codec_parameters : cur_codec_parameters;
        }
        else if(cur_codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
            audio_codec_parameters = audio_codec_parameters != NULL?
                        audio_codec_parameters : cur_codec_parameters;
        }
    }

    // init the contex, codec of video
    if(video_codec_parameters){
        height = video_codec_parameters->height;
        width = video_codec_parameters->width;

        // finds the registered decoder for a codec ID
        video_codec = avcodec_find_decoder(video_codec_parameters->codec_id);
        assert(video_codec);

        // find the codec ctx
        video_codec_ctx = avcodec_alloc_context3(video_codec);
        assert(video_codec_ctx);

        // Fill the codec context based on the values from the supplied codec parameters
        assert(avcodec_parameters_to_context(video_codec_ctx, video_codec_parameters) >= 0);

        // Initialize the AVCodecContext to use the given AVCodec.
        assert(avcodec_open2(video_codec_ctx, video_codec, NULL) >= 0);
    }

    // init the contex, codec of audio
    if(audio_codec_parameters){
        //audio_codec = avcodec_find_decoder(audio_codec_parameters->codec_id);
        //audio_codec_ctx = avcodec_alloc_context3(video_codec);
        assert(0);
    }

    std::cout << "Movie Init:" << std::endl;

}
