#include<assert.h>
#include<iostream>
#include<algorithm>
#include "movie.h"


Movie::Movie()
{
    format_ctx = avformat_alloc_context();
    video_frame = av_frame_alloc();
    audio_frame = av_frame_alloc();

    video_packet = av_packet_alloc();
    audio_packet = av_packet_alloc();

    assert(format_ctx && video_frame && audio_frame &&
           video_packet && audio_packet);
}

Movie::~Movie()
{
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);

    av_packet_unref(video_packet);
    av_packet_unref(audio_packet);
    av_packet_free(&video_packet);
    av_packet_free(&audio_packet);

    av_frame_unref(video_frame);
    av_frame_unref(audio_frame);
    av_frame_unref(rgb_frame);
    av_frame_free(&video_frame);
    av_frame_free(&audio_frame);
    av_frame_free(&rgb_frame);

    if(video_codec_ctx) avcodec_free_context(&video_codec_ctx);
    if(audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
    if(sws_context) sws_freeContext(sws_context);
}

void Movie::init(std::string path)
{
    movie_name = path;
    assert(avformat_open_input(&format_ctx, path.c_str(), NULL, NULL) == 0);
    assert(avformat_find_stream_info(format_ctx,  NULL) >= 0);
    duration = (double)format_ctx->duration / 1000000; // us -> s

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
        fps = format_ctx->streams[video_stream_index]->avg_frame_rate;
        timebase = format_ctx->streams[video_stream_index]->time_base;
        current_pts = -1;
        pts_per_frame = timebase.den / timebase.num / fps.num * fps.den;

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

        std::cout << "Video init: path=" << movie_name <<
                     " fps=" << fps.num << "/" << fps.den <<
                     " timebase=" << timebase.num << "/" << timebase.den <<
                     " pts_per_frame=" << pts_per_frame <<
                     " duration=" << duration <<
                     " height=" << height << " width=" << width << std::endl;
    }

    // init the contex, codec of audio
    if(audio_codec_parameters){
        audio_codec = avcodec_find_decoder(audio_codec_parameters->codec_id);
        audio_codec_ctx = avcodec_alloc_context3(video_codec);
    }

    return;
}

// get the next video packet to the video_packet
bool Movie::next_video_packet(){
    while(av_read_frame(format_ctx, video_packet) >= 0){
        if(video_packet->stream_index == video_stream_index){
            assert(avcodec_send_packet(video_codec_ctx, video_packet) >= 0);
            av_packet_unref(video_packet);
            return true;
        }
        else{
            av_packet_unref(video_packet);
        }
    }
    return false;
}


// get the next video frame to the video_frame
bool Movie::next_video_frame(){
    while(true){
        if(!ret_packet){
            ret_packet = next_video_packet();
        }
        if(ret_packet){
            av_frame_unref(video_frame);
            int ret = avcodec_receive_frame(video_codec_ctx, video_frame);
            // if current packet not satisfied, get net packet
            if(ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                ret_packet = false;
                continue;
            }
            // frame that we want, write to the rgb frame buff
            else{
                current_pts = video_frame->pts;
                //if(video_frame->key_frame){
                //    std::cout << "Frame-" << get_video_frame_index() << " IS key frame" << std::endl;
                //}
                write_rgb_frame();
                return true;
            }
        }
        // no more packet
        else{
            return false;
        }
    }
}

void Movie::seek_frame(int64_t target_frame){
    // frame_index = timebase * pts * fps, av_q2d: avrational to double
    // second = pts * timebase = frame_index / fps
    int64_t seek_time_pts = (int64_t)(target_frame * pts_per_frame);
    av_seek_frame(format_ctx, video_stream_index, seek_time_pts, AVSEEK_FLAG_BACKWARD);

    //std::cout << "seek frame:" << target_frame << "/" << get_frame_count()
    //          << " seek pts:" << seek_time_pts << "/" << get_max_pts() << std::endl;

    ret_packet = false;
    avcodec_flush_buffers(video_codec_ctx);
    return;
}


void Movie::init_rgb_frame(int h, int w){
    rgb_frame = av_frame_alloc();
    assert(rgb_frame);

    rgb_frame->height = h;
    rgb_frame->width = w;
    rgb_frame->format = AV_PIX_FMT_RGB24;
    assert(av_frame_get_buffer(rgb_frame, 0) >= 0);
}


void Movie::write_rgb_frame(){

    // create a swith context, for example, AV_PIX_FMT_YUV420P to AV_PIX_FMT_RGB24
    if(sws_context == NULL){
        sws_context = sws_getContext(video_frame->width,
                                             video_frame->height,
                                             AVPixelFormat(video_frame->format),
                                             rgb_frame->width,
                                             rgb_frame->height,
                                             AVPixelFormat(rgb_frame->format),
                                             SWS_BICUBIC, 0, 0, 0);
    }
    // perform conversion
    sws_scale(sws_context, video_frame->data, video_frame->linesize, 0,
              video_frame->height, rgb_frame->data, rgb_frame->linesize);

//    std::cout << "After conversion: linesize: " << rgb_frame->linesize[0] <<
//            " width=" << rgb_frame->width <<
//            " height=" << rgb_frame->height <<
//            " format=" << rgb_frame->format << std::endl;
    return;
}


// write the rgb frame to qimage
void Movie::write_qimage(QImage * img, int top_h, int top_w){
    assert((img->height() >= rgb_frame->height) &&
           (img->width() >= rgb_frame->width));
    int h, w, base;
    unsigned char * line_data;
    for(h = 0; h < rgb_frame->height; h++){
        line_data = img->scanLine(h + top_h);
        for(w = 0; w < rgb_frame->width; w++){
            base = h * rgb_frame->linesize[0] + w * 3;
            line_data[(top_w + w) * 3 + 0] = rgb_frame->data[0][base + 0];
            line_data[(top_w + w) * 3 + 1] = rgb_frame->data[0][base + 1];
            line_data[(top_w + w) * 3 + 2] = rgb_frame->data[0][base + 2];
        }
    }
    return;
}

int Movie::get_width(){
    return width;
}

int Movie::get_height(){
    return height;
}

int Movie::get_video_frame_index(){
    if(current_pts < 0) return -1;
    // frame_index = timebase * pts * fps
    video_frame_index = (int)(av_q2d(timebase) * current_pts * av_q2d(fps) + 0.5);
    return video_frame_index;
}

std::string Movie::get_movie_name(){
    return movie_name;
}

double Movie::get_video_frame_timestamp(){
    //av_q2d: avrational to double
    double stamp = (double)current_pts * av_q2d(timebase);
    return stamp; // second
}

double Movie::get_video_duration(){
    return duration; // second
}

double Movie::get_fps(){
    return (double)fps.num / (double)fps.den;
}

// is not accurate
int Movie::get_frame_count(){
    return (int)(duration * get_fps() + 0.5);
}

int64_t Movie::get_max_pts(){
    return (int64_t)(get_frame_count() * pts_per_frame);
}
