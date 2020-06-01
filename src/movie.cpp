#include<assert.h>
#include<iostream>
#include<algorithm>
#include "movie.h"


Movie::Movie()
{
    format_ctx = avformat_alloc_context();
    frame = av_frame_alloc();
    rgb_frame = av_frame_alloc();
    packet = av_packet_alloc();
    assert(frame && rgb_frame && packet);
}

Movie::~Movie()
{
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);

    av_frame_unref(rgb_frame);
    av_frame_free(&rgb_frame);
    av_frame_unref(video_frame);
    av_frame_free(&video_frame);
    av_frame_unref(audio_frame);
    av_frame_free(&audio_frame);

    if(sws_context) sws_freeContext(sws_context);

    for(auto codec_ctx : codec_contexts){
        avcodec_free_context(&codec_ctx);
    }

    clear_frame_queues(video_frame_queues);
    clear_frame_queues(audio_frame_queues);

    av_packet_unref(packet);
    av_packet_free(&packet);
    av_frame_unref(frame);
    av_frame_free(&frame);
    av_frame_unref(rgb_frame);
    av_frame_free(&rgb_frame);
}


void Movie::clear_frame_queues(std::queue<AVFrame*> queues){
    AVFrame* tmp_frame;
    while(queues.size() > 0){
        tmp_frame = queues.front();
        av_frame_unref(tmp_frame);
        av_frame_free(&tmp_frame);
        queues.pop();
    }
    return;
}

void Movie::parse_video_codec(int stream_ind, AVCodecParameters* codec_parameters){
    assert(codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO);

    // init video infomation
    height = codec_parameters->height;
    width = codec_parameters->width;
    fps = this->format_ctx->streams[stream_ind]->avg_frame_rate;
    timebase = this->format_ctx->streams[stream_ind]->time_base;
    current_pts = -1;
    pts_per_frame = timebase.den / timebase.num / fps.num * fps.den;

    // finds the registered decoder for a codec ID
    AVCodec* video_codec = avcodec_find_decoder(codec_parameters->codec_id);
    assert(video_codec);

    // find the codec ctx
    AVCodecContext* video_codec_ctx = avcodec_alloc_context3(video_codec);
    assert(video_codec_ctx);

    // Fill the codec context based on the values from the supplied codec parameters
    assert(avcodec_parameters_to_context(video_codec_ctx, codec_parameters) >= 0);

    // Initialize the AVCodecContext to use the given AVCodec.
    assert(avcodec_open2(video_codec_ctx, video_codec, NULL) >= 0);

    assert(codecs.size() == stream_ind);
    codecs.push_back(video_codec);
    codec_contexts.push_back(video_codec_ctx);
    video_stream_indexs.push_back(stream_ind);
    video_stream_index = stream_ind;

    std::cout << "Video parameters init:" <<
                 " fps=" << fps.num << "/" << fps.den <<
                 " timebase=" << timebase.num << "/" << timebase.den <<
                 " pts_per_frame=" << pts_per_frame <<
                 " duration=" << duration <<
                 " height=" << height << " width=" << width << std::endl;
    return;
}


void Movie::parse_audio_codec(int stream_ind, AVCodecParameters* codec_parameters){
    assert(codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO);

    // finds the registered decoder for a codec ID
    AVCodec* audio_codec = avcodec_find_decoder(codec_parameters->codec_id);

    // find the codec ctx
    AVCodecContext* audio_codec_ctx = avcodec_alloc_context3(audio_codec);
    assert(audio_codec_ctx);

    // Fill the codec context based on the values from the supplied codec parameters
    assert(avcodec_parameters_to_context(audio_codec_ctx, codec_parameters) >= 0);

    // Initialize the AVCodecContext to use the given AVCodec.
    assert(avcodec_open2(audio_codec_ctx, audio_codec, NULL) >= 0);

    assert(codecs.size() == stream_ind);
    codecs.push_back(audio_codec);
    codec_contexts.push_back(audio_codec_ctx);
    audio_stream_indexs.push_back(stream_ind);
    audio_stream_index = stream_ind;

    audio_sample_rate = audio_codec_ctx->sample_rate;
    audio_channel = audio_codec_ctx->channels;
    switch(audio_codec_ctx->sample_fmt){
        case AV_SAMPLE_FMT_S16:
            audio_sample_size = 16;
            break;
        case AV_SAMPLE_FMT_S32:
            audio_sample_size = 32;
            break;
        case AV_SAMPLE_FMT_S64:
            audio_sample_size = 64;
            break;
        default:
            break;
    }

    std::cout << "Audio parameters init:" <<
                 " sample_rate:" << audio_sample_rate <<
                 " sample_channel:" << audio_channel <<
                 " sample_size:" << audio_sample_size << std::endl;
    return;
}


void Movie::init(std::string path)
{
    mutex.lock();
    movie_name = path;
    assert(avformat_open_input(&format_ctx, path.c_str(), NULL, NULL) == 0);
    assert(avformat_find_stream_info(format_ctx,  NULL) >= 0);
    duration = (double)format_ctx->duration / 1000000; // us -> s
    n_streams = format_ctx->nb_streams;
    std::cout << "Movie: " << movie_name << " Streams:" << n_streams << std::endl;

    // find the video codec parameters and audio codec parameters
    for(int i = 0; i < format_ctx->nb_streams; i++)
    {
        AVCodecParameters *codec_parameters = NULL;
        codec_parameters = format_ctx->streams[i]->codecpar;
        if(codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO){
            parse_video_codec(i, codec_parameters);
        }
        else if(codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO){
            parse_audio_codec(i, codec_parameters);
        }
        else{
            assert(false);
        }
    }
    mutex.unlock();
    return;
}


void Movie::init_rgb_frame(int h, int w){
    rgb_frame->height = h;
    rgb_frame->width = w;
    rgb_frame->format = AV_PIX_FMT_RGB24;
    assert(av_frame_get_buffer(rgb_frame, 0) >= 0);
}


// "busy sleep" while suggesting that other threads run for a small amount of time
void Movie::thread_sleep(std::chrono::microseconds us){
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + us;
    do {
        std::this_thread::yield();
    } while (std::chrono::high_resolution_clock::now() < end);
}


// busy thread
void Movie::fetch_frame(){
    while(true){
        mutex.lock();
        // fill > half not fill
        if((video_frame_queues.size() > queue_max / 2) &&
            (audio_frame_queues.size() > queue_max / 2)){
            mutex.unlock();
            thread_sleep(std::chrono::microseconds(10000)); // 10ms
        }
        else{
            // read a frame
            av_packet_unref(packet);
            if(av_read_frame(format_ctx, packet) >= 0){
                if((packet->stream_index == video_stream_index) ||
                    (packet->stream_index == audio_stream_index)){
                    AVFrame* tmp_frame = av_frame_alloc();
                    assert(avcodec_send_packet(codec_contexts[packet->stream_index], packet) >= 0)
                    int ret = avcodec_receive_frame(codec_contexts[packet->stream_index], tmp_frame);
                    if(!(ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)){
                        if(packet->stream_index == video_stream_index){
                            video_frame_queues.push(tmp_frame);
                        }
                        else{
                            audio_frame_queues.push(tmp_frame);
                        }
                    }
                }
            }
            mutex.unlock();
        }
    }
}


bool Movie::next_frame(){
    mutex.lock();
    av_frame_unref(video_frame);
    av_frame_free(&video_frame);
    av_frame_unref(audio_frame);
    av_frame_free(&audio_frame);
    video_frame = audio_frame = NULL;
    if(video_frame_queues.size() > 0){
        video_frame = video_frame_queues.front();
        video_frame_queues.pop();
        current_pts = video_frame->pts;
        write_rgb_frame(video_frame);
    }
    if(audio_frame_queues.size() > 0){
        audio_frame = audio_frame_queues.front();
        audio_frame_queues.pop();
    }
    mutex.unlock();
    if(video_frame) return true;
    else return false;
}


void Movie::seek_frame(int64_t target_frame){
    mutex.lock();
    int frame_buf_size = video_frame_queues.size();
    int cur_frame = get_video_frame_index();
    if(target_frame == cur_frame) return;

    // target frame is inside the queues
    if((target_frame > cur_frame) &&
       (target_frame <= cur_frame + frame_buf_size)){
        int diff = target_frame - cur_frame;
        // make the target frame in the head of queue
        while(diff > 1){
            av_frame_unref(video_frame);
            av_frame_free(&video_frame);
            video_frame = video_frame_queues.front();
            video_frame_queues.pop();
            diff--;
        }
        current_pts = video_frame->pts;
        write_rgb_frame(video_frame);

        // find the nearest audio frame
        while((audio_frame->pts < current_pts) &&
              (audio_frame_queues.size() > 0)){
            if(audio_frame_queues.front()->pts >= current_pts){
                break;
            }
            av_frame_unref(audio_frame);
            av_frame_free(&audio_frame);
            audio_frame = audio_frame_queues.front();
            audio_frame_queues.pop();
        }
        mutex.unlock();
    }
    else{
        clear_frame_queues(video_frame_queues);
        clear_frame_queues(audio_frame_queues);
        // frame_index = timebase * pts * fps, av_q2d: avrational to double
        // second = pts * timebase = frame_index / fps
        int64_t seek_time_pts = (int64_t)(target_frame * pts_per_frame);
        av_seek_frame(format_ctx, video_stream_index, seek_time_pts, AVSEEK_FLAG_BACKWARD);

        //std::cout << "seek frame:" << target_frame << "/" << get_frame_count()
        //          << " seek pts:" << seek_time_pts << "/" << get_max_pts() << std::endl;

        avcodec_flush_buffers(codec_contexts[video_stream_index]);
        avcodec_flush_buffers(codec_contexts[audio_stream_index]);
        mutex.unlock();

        // wait to fill the queues
        thread_sleep(std::chrono::microseconds(40000)); // 40ms
    }
    return;
}


void Movie::write_rgb_frame(AVFrame* video_frame){

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
