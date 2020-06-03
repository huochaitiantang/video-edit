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

    clear_frame_vectors(video_frames);
    clear_frame_vectors(audio_frames);

    av_packet_unref(packet);
    av_packet_free(&packet);
    av_frame_unref(frame);
    av_frame_free(&frame);
    av_frame_unref(rgb_frame);
    av_frame_free(&rgb_frame);

    delete pcm_buf;
}


void Movie::clear_frame_vectors(std::vector<AVFrame*> vs){
    AVFrame* tmp_frame;
    while(vs.size() > 0){
        tmp_frame = vs.back();
        av_frame_unref(tmp_frame);
        av_frame_free(&tmp_frame);
        vs.pop_back();
    }

}

void Movie::parse_video_codec(int stream_ind, AVCodecParameters* codec_parameters){
    assert(codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO);

    // init video infomation
    height = codec_parameters->height;
    width = codec_parameters->width;
    video_fps = this->format_ctx->streams[stream_ind]->avg_frame_rate;
    video_timebase = this->format_ctx->streams[stream_ind]->time_base;

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
                 " fps=" << video_fps.num << "/" << video_fps.den <<
                 " timebase=" << video_timebase.num << "/" << video_timebase.den <<
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

    audio_timebase = this->format_ctx->streams[stream_ind]->time_base;
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
                 " audio_timebase" << audio_timebase.num << "/" << audio_timebase.den <<
                 " sample_rate:" << audio_sample_rate <<
                 " sample_channel:" << audio_channel <<
                 " sample_size:" << audio_sample_size << std::endl;
    return;
}


void Movie::init(std::string path)
{
    movie_name = path;
    assert(avformat_open_input(&format_ctx, path.c_str(), NULL, NULL) == 0);
    assert(avformat_find_stream_info(format_ctx,  NULL) >= 0);
    duration = (double)format_ctx->duration / 1000000; // us -> s
    n_streams = format_ctx->nb_streams;
    std::cout << "Movie: " << movie_name
              << " Streams:" << n_streams
              << " Duration=" << duration << std::endl;

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

    fetch_some_packets();
    audio_frame_ind = video_frame_ind = 0;
    double ms1 = video_pts_to_ms(video_frames[video_frame_ind]->pts);
    double ms2 = audio_pts_to_ms(audio_frames[audio_frame_ind]->pts);
    base_pts_ms = ms1 < ms2 ? ms1 : ms2;
    base_sys_ms = (double)(QDateTime::currentDateTime().toMSecsSinceEpoch());

    return;
}


void Movie::init_rgb_frame(int h, int w){
    rgb_frame->height = h;
    rgb_frame->width = w;
    rgb_frame->format = AV_PIX_FMT_RGB24;
    assert(av_frame_get_buffer(rgb_frame, 0) >= 0);
}


void Movie::decode_one_packet(){
    if((packet->stream_index == video_stream_index) ||
        (packet->stream_index == audio_stream_index)){
        assert(avcodec_send_packet(codec_contexts[packet->stream_index], packet) >= 0);

        AVFrame* tmp_frame = av_frame_alloc();
        int ret = avcodec_receive_frame(codec_contexts[packet->stream_index], tmp_frame);

        // one packet may consist > 1 frame
        while(!(ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)){
            if(packet->stream_index == video_stream_index){
                video_frames.push_back(tmp_frame);
                std::cout << "Read Video Frame, PTS = " << tmp_frame->pts << std::endl;
            }
            else{
                audio_frames.push_back(tmp_frame);
                std::cout << "Read Audio Frame, PTS = " << tmp_frame->pts << std::endl;
            }
            ret = avcodec_receive_frame(codec_contexts[packet->stream_index], tmp_frame);
        }
    }
    return;
}


bool Movie::fetch_some_packets(){
    // each time handle 20 packets
    for(int i = 0; i < 20; i++){
        // read a frame
        av_packet_unref(packet);
        if(av_read_frame(format_ctx, packet) >= 0){
            decode_one_packet();
        }
        else{
            return false;
        }
    }
    return true;
}


void Movie::adjust_frame_vectors(){
    // buffer the frame near 3 second
    double radius = 3000;

    // for video
    if(video_frames.size() > 0){
        double head_video_ms = video_pts_to_ms(video_frames.front()->pts);
        double tail_video_ms = video_pts_to_ms(video_frames.back()->pts);
        double cur_video_ms = video_pts_to_ms(video_frames[video_frame_ind]->pts);
        double diff_head = cur_video_ms - head_video_ms;
        double diff_tail = tail_video_ms - cur_video_ms;

        while(diff_head > radius){
            // delete some elements from the head
            AVFrame* tmp_frame = video_frames.front();
            av_frame_unref(tmp_frame);
            av_frame_free(&tmp_frame);
            video_frames.erase(video_frames.begin());
            head_video_ms = video_pts_to_ms(video_frames.front()->pts);
            video_frame_ind--;
            diff_head = cur_video_ms - head_video_ms;
        }

        if(diff_tail < radius){
            // read some new packets
            fetch_some_packets();
        }
    }

    // for audio
    if(audio_frames.size() > 0){
        double head_audio_ms = audio_pts_to_ms(audio_frames.front()->pts);
        double tail_audio_ms = audio_pts_to_ms(audio_frames.back()->pts);
        double cur_audio_ms = audio_pts_to_ms(audio_frames[audio_frame_ind]->pts);
        double diff_head = cur_audio_ms - head_audio_ms;
        double diff_tail = tail_audio_ms - cur_audio_ms;

        while(diff_head > radius){
            // delete some elements from the head
            AVFrame* tmp_frame = audio_frames.front();
            av_frame_unref(tmp_frame);
            av_frame_free(&tmp_frame);
            audio_frames.erase(audio_frames.begin());
            head_audio_ms = audio_pts_to_ms(audio_frames.front()->pts);
            audio_frame_ind--;
            diff_head = cur_audio_ms - head_audio_ms;
        }

        if(diff_tail < radius){
            // read some new packets
            fetch_some_packets();
        }
    }
    return;
}


bool Movie::next_frame(){
    adjust_frame_vectors();




    av_frame_unref(video_frame);
    av_frame_free(&video_frame);
    av_frame_unref(audio_frame);
    av_frame_free(&audio_frame);
    video_frame = audio_frame = NULL;
    if(video_frames.size() > 0){
        video_frame = video_frames.front();
        video_frame_queues.pop();
        current_pts = video_frame->pts;
        write_rgb_frame(video_frame);
        std::cout << "Fetch Video Frame, PTS = " << video_frame->pts << std::endl;
    }
    if(audio_frame_queues.size() > 0){
        audio_frame = audio_frame_queues.front();
        audio_frame_queues.pop();
        write_audio_frame(audio_frame);
        std::cout << "Fetch Audio Frame, PTS = " << audio_frame->pts << std::endl;
    }
    if(video_frame) return true;
    else return false;
}


void Movie::seek_frame(int64_t target_frame){

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

    }
    else{
        clear_frame_vectors(video_frames);
        clear_frame_vectors(audio_frames);
        // frame_index = timebase * pts * fps, av_q2d: avrational to double
        // second = pts * timebase = frame_index / fps
        int64_t seek_time_pts = (int64_t)(target_frame * pts_per_frame);
        av_seek_frame(format_ctx, video_stream_index, seek_time_pts, AVSEEK_FLAG_BACKWARD);

        //std::cout << "seek frame:" << target_frame << "/" << get_frame_count()
        //          << " seek pts:" << seek_time_pts << "/" << get_max_pts() << std::endl;

        avcodec_flush_buffers(codec_contexts[video_stream_index]);
        avcodec_flush_buffers(codec_contexts[audio_stream_index]);


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
    std::cout << "Init Swscale" << std::endl;
    // perform conversion
    sws_scale(sws_context, video_frame->data, video_frame->linesize, 0,
              video_frame->height, rgb_frame->data, rgb_frame->linesize);

    std::cout << "After conversion: linesize: " << rgb_frame->linesize[0] <<
            " width=" << rgb_frame->width <<
            " height=" << rgb_frame->height <<
            " format=" << rgb_frame->format << std::endl;
    return;
}


void Movie::write_audio_frame(AVFrame * audio_frame){
    // audio resample to the format same to the qt play format
    if(swr_context == NULL){
        swr_context = swr_alloc();
        swr_alloc_set_opts(swr_context,
            codec_contexts[audio_stream_index]->channel_layout,
            AV_SAMPLE_FMT_S16,
            audio_sample_rate,
            audio_channel,
            codec_contexts[audio_stream_index]->sample_fmt,
            audio_sample_rate, 0, 0);
        swr_init(swr_context);
    }

    pcm_len = av_samples_get_buffer_size(NULL,
        audio_channel,
        audio_frame->nb_samples,
        AV_SAMPLE_FMT_S16, 0);

    uint8_t *pcm_out[2] = {0};
    pcm_out[0] = (uint8_t *)pcm_buf;
    int read_len = swr_convert(swr_context,
                    pcm_out,
                    audio_frame->nb_samples,
                    (const uint8_t **)audio_frame->data,
                    audio_frame->nb_samples
               );
    if(read_len <= 0) pcm_len = 0;
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

void Movie::write_qaudio(QIODevice * audio_io){
    if((audio_io != NULL) && (pcm_len > 0)){
        audio_io->write(pcm_buf, pcm_len);
    }
}

int Movie::get_width(){
    return width;
}

int Movie::get_height(){
    return height;
}

std::string Movie::get_movie_name(){
    return movie_name;
}

double Movie::get_duration(){
    return duration; // second
}

double Movie::get_video_fps(){
    return (double)video_fps.num / (double)video_fps.den;
}

int Movie::get_audio_sample_channel(){
    return audio_channel;
}

int Movie::get_audio_sample_size(){
    return audio_sample_size;
}

int Movie::get_audio_sample_rate(){
    return audio_sample_rate;
}

double Movie::audio_pts_to_ms(int64_t pts){
    return (double)pts * av_q2d(audio_timebase) * 1000;
}

double Movie::video_pts_to_ms(int64_t pts){
    return (double)pts * av_q2d(video_timebase) * 1000;
}
