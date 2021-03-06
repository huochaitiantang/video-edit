#include<assert.h>
#include<iostream>
#include<algorithm>
#include "movie.h"
#include<sys/timeb.h>
#include<windows.h>

Movie::Movie()
{
    mutex.lock();
    format_ctx = avformat_alloc_context();
    rgb_frame = av_frame_alloc();
    packet = av_packet_alloc();
    mutex.unlock();
}

Movie::~Movie()
{
    mutex.lock();
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);

    av_frame_unref(rgb_frame);
    av_frame_free(&rgb_frame);

    if(sws_context) sws_freeContext(sws_context);
    if(swr_context) swr_free(&swr_context);

    for(auto codec_ctx : codec_contexts){
        avcodec_free_context(&codec_ctx);
    }

    clear_frame_vectors(video_frames);
    clear_frame_vectors(audio_frames);

    av_packet_unref(packet);
    av_packet_free(&packet);
    av_frame_unref(rgb_frame);
    av_frame_free(&rgb_frame);

    free(pcm_buf);
    mutex.unlock();
}

void Movie::clear_frame_vectors(std::vector<AVFrame*>& vs){
    AVFrame* tmp_frame;
    while(vs.size() > 0){
        tmp_frame = vs.back();
        av_frame_unref(tmp_frame);
        av_frame_free(&tmp_frame);
        vs.pop_back();
    }
}

void Movie::set_video_codec(int stream_ind){
    assert(stream_types[stream_ind] == AVMEDIA_TYPE_VIDEO);
    // set video infomation
    video_stream_index = stream_ind;
    height = format_ctx->streams[stream_ind]->codecpar->height;
    width = format_ctx->streams[stream_ind]->codecpar->width;
    video_fps = av_q2d(format_ctx->streams[stream_ind]->avg_frame_rate);
    video_timebase = format_ctx->streams[stream_ind]->time_base;
    video_buf_len = min(300, max(0, (int)(radius_ms * video_fps / 1000)));

    std::cout << "Video Codec Set: fps=" << video_fps << " video_buf:" << video_buf_len
              << " timebase=" << video_timebase.num << "/" << video_timebase.den
              << " height=" << height << " width=" << width << std::endl;
    return;
}

void Movie::set_audio_codec(int stream_ind){
    assert(stream_types[stream_ind] == AVMEDIA_TYPE_AUDIO);
    //set audio infomation
    audio_stream_index = stream_ind;
    audio_timebase = this->format_ctx->streams[stream_ind]->time_base;
    audio_sample_rate = codec_contexts[stream_ind]->sample_rate;
    audio_channel = codec_contexts[stream_ind]->channels;
    audio_layout_channel = codec_contexts[stream_ind]->channel_layout;
    audio_fps = codec_contexts[stream_ind]->frame_size > 0 ?
                audio_sample_rate / (double)(codec_contexts[stream_ind]->frame_size) : 100;

    switch(codec_contexts[stream_ind]->sample_fmt){
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
    audio_buf_len = min(300, max(0, (int)(radius_ms * audio_fps / 1000)));

    std::cout << "Audio Codec Set: audio_fps:" << audio_fps << " audio_buf:" << audio_buf_len
              << " audio_timebase" << audio_timebase.num << "/" << audio_timebase.den
              << " sample_rate:" << audio_sample_rate << " sample_channel:" << audio_channel
              << " layout_channel:" << audio_layout_channel << " sample_size:" << audio_sample_size << std::endl;
    return;
}

void Movie::parse_codec(int stream_ind, AVCodecParameters* codec_parameters){
    AVMediaType media_type = codec_parameters->codec_type;
    assert(stream_types.size() == stream_ind);
    stream_types.push_back(media_type);
    std::cout << "Parse Stream [" << stream_ind << "] :"
              << av_get_media_type_string(media_type) << std::endl;

    if(media_type == AVMEDIA_TYPE_VIDEO){
        video_stream_indexs.push_back(stream_ind);
    }
    else if(media_type == AVMEDIA_TYPE_AUDIO){
        audio_stream_indexs.push_back(stream_ind);
    }
    else if(media_type == AVMEDIA_TYPE_SUBTITLE){
        subtitle_stream_indexs.push_back(stream_ind);
    }

    // finds the registered decoder for a codec ID
    AVCodec* codec = avcodec_find_decoder(codec_parameters->codec_id);

    // find the codec ctx
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    assert(codec_ctx);

    // Fill the codec context based on the values from the supplied codec parameters
    assert(avcodec_parameters_to_context(codec_ctx, codec_parameters) >= 0);

    // Initialize the AVCodecContext to use the given AVCodec.
    assert(avcodec_open2(codec_ctx, codec, NULL) >= 0);

    assert(codec_contexts.size() == stream_ind);
    codec_contexts.push_back(codec_ctx);
}

void Movie::init(std::string path){
    mutex.lock();
    movie_name = path;

    assert(avformat_open_input(&format_ctx, path.c_str(), NULL, NULL) == 0);
    assert(avformat_find_stream_info(format_ctx,  NULL) >= 0);

    video_stream_index = -1;
    audio_stream_index = -1;
    subtitle_stream_index = -1;

    duration = (double)format_ctx->duration / 1000000; // us -> s
    n_streams = format_ctx->nb_streams;
    std::cout << "Movie: " << movie_name << " Streams:" << n_streams
              << " Duration:" << duration << std::endl;

    // find the video codec parameters and audio codec parameters
    for(int i = 0; i < format_ctx->nb_streams; i++){
        parse_codec(i, format_ctx->streams[i]->codecpar);
    }

    if(video_stream_indexs.size() > 0){
        set_video_codec(video_stream_indexs[0]);
    }
    if(audio_stream_indexs.size() > 0){
        set_audio_codec(audio_stream_indexs[0]);
    }
    mutex.unlock();
    return;
}

void Movie::init_rgb_frame(int h, int w){
    mutex.lock();
    rgb_frame->height = h;
    rgb_frame->width = w;
    rgb_frame->format = AV_PIX_FMT_RGB24;
    assert(av_frame_get_buffer(rgb_frame, 0) >= 0);

    // first image display
    audio_frame_ind = video_frame_ind = 0;
    fetch_some_packets(50);
    last_audio_pts = last_video_pts = -1;
    last_audio_sys = last_video_sys = -1;
    write_rgb_frame();
    mutex.unlock();
}

void Movie::decode_one_packet(){
    if((packet->stream_index == video_stream_index) ||
        (packet->stream_index == audio_stream_index)){
        assert(avcodec_send_packet(codec_contexts[packet->stream_index], packet) >= 0);

        // one packet may consist > 1 frame
        while(true){
            AVFrame* tmp_frame = av_frame_alloc();
            int ret = avcodec_receive_frame(codec_contexts[packet->stream_index], tmp_frame);
            if(ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                av_frame_unref(tmp_frame);
                av_frame_free(&tmp_frame);
                break;
            }
            if(packet->stream_index == video_stream_index){
                video_frames.push_back(tmp_frame);
            }
            else{
                audio_frames.push_back(tmp_frame);
            }
        }
    }
    return;
}

bool Movie::fetch_some_packets(int packet_num){
    // each time handle some packets
    double st = current_sys_ms();
    int i = 0;
    // 5 more frames for further play
    while((i < packet_num) ||
          (video_frames.size() < video_frame_ind + 5) ||
          (audio_frames.size() < audio_frame_ind + 5)){
        // read a frame
        av_packet_unref(packet);
        if(av_read_frame(format_ctx, packet) >= 0){
            decode_one_packet();
        }
        else{
            return false;
        }
        i++;
    }
    double et = current_sys_ms();
    /*
    std::cout << "Fetch-cost: " << et - st << " ms"
              << ", video-frame-len:" << video_frames.size()
              << ", audio-frame-len:" << audio_frames.size()
              << ", video-frame-ind:" << video_frame_ind
              << ", audio-fraem-ind:" << audio_frame_ind << std::endl;
    */
    return true;
}

void Movie::fetch_frames(){
    mutex.lock();
    if((video_frame_ind + video_buf_len > video_frames.size()) ||
        audio_frame_ind + audio_buf_len > audio_frames.size()){
        fetch_some_packets(1);
    }
    mutex.unlock();
}

bool Movie::play_video_frame(){
    mutex.lock();
    if((video_frame_ind < 0) || (video_frame_ind >= video_frames.size())){
        mutex.unlock();
        return false;
    }

    // show current frame
    if(last_video_sys < 0){
         write_rgb_frame();
        last_video_sys = current_sys_ms();
        last_video_pts = pts_to_ms(video_frames[video_frame_ind]->pts, video_timebase);
        mutex.unlock();
        return true;
    }

    // sync with the audio pts
    double delta = 1500 / video_fps;
    if((last_video_pts > 0) && (last_audio_pts > 0) &&
       (abs(last_audio_pts - last_video_pts) > delta)){
        // find the pts nearest th last_audio_pts
        int ind = binary_search(last_audio_pts, video_frames, video_timebase);
        if(ind >= 0){
            video_frame_ind = ind;
            write_rgb_frame();
            last_video_sys = current_sys_ms();
            last_video_pts = pts_to_ms(video_frames[video_frame_ind]->pts, video_timebase);
            mutex.unlock();
            return true;
        }
        else{
            // video forward too much, need to wait, when hard seek
            double head_video_pts = pts_to_ms(video_frames.front()->pts, video_timebase);
            if(last_audio_pts <= head_video_pts){
                std::cout << "Warning: Sync Bad Video Forward Too Much, Wait" << std::endl;
                video_frame_ind = 0;
                last_video_pts = last_video_sys = -1;
                mutex.unlock();
                Sleep(head_video_pts - last_audio_pts);
                return false;
            }
            // video behind too much, need to clear the buf
            double tail_video_pts = pts_to_ms(video_frames.back()->pts, video_timebase);
            if(last_audio_pts >= tail_video_pts){
                std::cout << "Warning: Sync Bad Video Backward Too Much, Clear" << std::endl;
                clear_frame_vectors(video_frames);
                video_frame_ind = 0;
                last_video_pts = last_video_sys = -1;
                mutex.unlock();
                return false;
            }
            assert(false);
        }
    }

    // need to refer the last frame
    int next_ind = video_frame_ind + 1;
    if(next_ind >= video_frames.size()){
        mutex.unlock();
        return false;
    }

    // clear some front frames
    while(next_ind > video_buf_len){
        AVFrame* tmp_frame = video_frames.front();
        av_frame_unref(tmp_frame);
        av_frame_free(&tmp_frame);
        video_frames.erase(video_frames.begin());
        next_ind--;
    }

    double cur_pts = pts_to_ms(video_frames[next_ind]->pts, video_timebase);
    double cur_sys = current_sys_ms();
    double diff_sys = (cur_sys - last_video_sys);
    double diff_pts = (cur_pts - last_video_pts) / play_times;
    double sleep_dt = (diff_pts - diff_sys);

    video_frame_ind = next_ind;
    write_rgb_frame();
    if(sleep_dt > play_window){
        mutex.unlock();
        Sleep(sleep_dt);
        mutex.lock();
    }
    last_video_sys = current_sys_ms();
    last_video_pts = cur_pts;
    mutex.unlock();
    return true;
}

bool Movie::play_audio_frame(){
    mutex.lock();
    if((audio_frame_ind < 0) || (audio_frame_ind >= audio_frames.size())){
        mutex.unlock();
        return false;
    }

    // show current frame
    if(last_audio_sys < 0){
        last_audio_sys = current_sys_ms();
        last_audio_pts = pts_to_ms(audio_frames[audio_frame_ind]->pts, audio_timebase);
        write_audio_frame();
        mutex.unlock();
        return true;
    }

    // need to refer the last frame
    int next_ind = audio_frame_ind + 1;
    if(next_ind >= audio_frames.size()){
        mutex.unlock();
        return false;
    }

    // clear some front frames
    while(next_ind > audio_buf_len){
        AVFrame* tmp_frame = audio_frames.front();
        av_frame_unref(tmp_frame);
        av_frame_free(&tmp_frame);
        audio_frames.erase(audio_frames.begin());
        next_ind--;
    }

    double cur_pts = pts_to_ms(audio_frames[next_ind]->pts, audio_timebase);
    double cur_sys = current_sys_ms();
    double diff_sys = (cur_sys - last_audio_sys);
    double diff_pts = (cur_pts - last_audio_pts) / play_times;
    double sleep_dt = (diff_pts - diff_sys);

    audio_frame_ind = next_ind;
    write_audio_frame();
    if(sleep_dt > play_window){
        mutex.unlock();
        Sleep(sleep_dt);
        mutex.lock();
    }
    last_audio_sys = current_sys_ms();
    last_audio_pts = cur_pts;
    mutex.unlock();
    return true;
}

int Movie::binary_search(double millsecs, std::vector<AVFrame*> frames, AVRational timebase){
    if((frames.size() <= 0) ||
        (millsecs < pts_to_ms(frames.front()->pts, timebase)) ||
        (millsecs > pts_to_ms(frames.back()->pts, timebase))){
        return -1;
    }
    int cnt = frames.size();
    int low = 0, high = cnt - 1, mid;
    double cur, prev, next;
    while(low < high){
        mid = (low + high) / 2;
        cur = pts_to_ms(frames[mid]->pts, timebase);
        prev = mid <= 0     ? cur: pts_to_ms(frames[mid - 1]->pts, timebase);
        next = mid >= cnt-1 ? cur: pts_to_ms(frames[mid + 1]->pts, timebase);
        if((millsecs >= prev) && (millsecs < next)){
            if(millsecs < cur) return max(0, mid - 1);
            else return min(cnt - 1, mid);
        }
        if(cur > millsecs) high = mid + 1;
        else low = mid - 1;
    }
    assert(false);
    return -1;
}

bool Movie::seek(double millsecs){
    mutex.lock();
    if(video_frames.size() <= 0 || audio_frames.size() <= 0){
        bool res = hard_seek(millsecs);
        mutex.unlock();
        return res;
    }
    int video_ind = binary_search(millsecs, video_frames, video_timebase);
    int audio_ind = binary_search(millsecs, audio_frames, audio_timebase);

    if(video_ind < 0 || audio_ind < 0){
        bool res = hard_seek(millsecs);
        mutex.unlock();
        return res;
    }
    // target frame is inside the vectors
    video_frame_ind = video_ind;
    audio_frame_ind = audio_ind;
    last_audio_pts = last_video_pts = -1;
    last_audio_sys = last_video_sys = -1;
    write_rgb_frame();
    mutex.unlock();
    return true;
}

bool Movie::hard_seek(double millsecs){
    clear_frame_vectors(video_frames);
    clear_frame_vectors(audio_frames);

    // frame_index = timebase * pts * fps, av_q2d: avrational to double
    // second = pts * timebase = frame_index / fps
    int64_t seek_time_pts = (int64_t)(millsecs / 1000. / av_q2d(video_timebase));

    //std::cout << "seek ms:" << millsecs << "/" << get_duration() * 1000
    //          << " seek pts:" << seek_time_pts << std::endl;

    av_seek_frame(format_ctx, video_stream_index, seek_time_pts, AVSEEK_FLAG_BACKWARD);

    avcodec_flush_buffers(codec_contexts[video_stream_index]);
    avcodec_flush_buffers(codec_contexts[audio_stream_index]);

    audio_frame_ind = video_frame_ind = 0;
    fetch_some_packets(50);
    last_audio_pts = last_video_pts = -1;
    last_audio_sys = last_video_sys = -1;
    write_rgb_frame();
    return true;
}

void Movie::write_rgb_frame(){
    if((video_frame_ind < 0) || (video_frame_ind >= video_frames.size())){
        return;
    }
    AVFrame* video_frame = video_frames[video_frame_ind];
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
    //std::cout << "Init Swscale" << std::endl;
    // perform conversion
    sws_scale(sws_context, video_frame->data, video_frame->linesize, 0,
              video_frame->height, rgb_frame->data, rgb_frame->linesize);

    //std::cout << "After conversion: linesize: " << rgb_frame->linesize[0] <<
    //        " width=" << rgb_frame->width <<
    //        " height=" << rgb_frame->height <<
    //        " format=" << rgb_frame->format << std::endl;
    return;
}

void Movie::write_audio_frame(){
    if((audio_frame_ind < 0) || (audio_frame_ind >= audio_frames.size())){
        return;
    }
    AVCodecContext * audio_ctx = codec_contexts[audio_stream_index];
    AVFrame* audio_frame = audio_frames[audio_frame_ind];
    // audio resample to the format same to the qt play format
    if(swr_context == NULL){
        swr_context = swr_alloc();
        swr_alloc_set_opts(swr_context,
                        audio_ctx->channel_layout, AV_SAMPLE_FMT_S16, audio_ctx->sample_rate,
                        audio_ctx->channel_layout, audio_ctx->sample_fmt, audio_ctx->sample_rate,
                        0, 0);
        swr_init(swr_context);
    }

    uint8_t *pcm_out[2] = {0};
    pcm_out[0] = (uint8_t *)pcm_buf;
    int ret = swr_convert(swr_context, pcm_out, 4096,
                    (const uint8_t **)(audio_frame->data),
                    audio_frame->nb_samples);
    int outsize = av_samples_get_buffer_size(NULL, audio_ctx->channels, audio_frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
    pcm_len = outsize;

    /*
    std::cout << "read_len:" << ret << std::endl;
    std::cout << "pcm_len:" << pcm_len << std::endl;
    std::cout << "nb samples:" << audio_frame->nb_samples << std::endl;
    */
}

// write the rgb frame to qimage
void Movie::write_qimage(QImage * img, int top_h, int top_w){
    if(video_buf_len <= 0) return;
    mutex.lock();
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
    mutex.unlock();
    return;
}

void Movie::write_qaudio(QIODevice * audio_io){
    if(audio_buf_len == 0) return;
    mutex.lock();
    if((audio_io != NULL) && (pcm_len > 0)){
        audio_io->write(pcm_buf, pcm_len);
    }
    mutex.unlock();
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
    return video_fps;
}

int Movie::get_audio_sample_channel(){
    return audio_channel;
}

int Movie::get_audio_layout_channel(){
    return audio_layout_channel;
}

int Movie::get_audio_sample_size(){
    return audio_sample_size;
}

int Movie::get_audio_sample_rate(){
    return audio_sample_rate;
}

double Movie::pts_to_ms(int64_t pts, AVRational timebase){
    return (double)pts * av_q2d(timebase) * 1000;
}

double Movie::get_video_frame_ms(){
    mutex.lock();
    if(video_frames.size() <= 0){
        mutex.unlock();
        return 0;
    }
    double ans = pts_to_ms(video_frames[video_frame_ind]->pts, video_timebase);
    mutex.unlock();
    return ans;
}

double Movie::get_audio_frame_ms(){
    mutex.lock();
    if(audio_frames.size() <= 0){
        mutex.unlock();
        return 0;
    }
    double ans = pts_to_ms(audio_frames[audio_frame_ind]->pts, audio_timebase);
    mutex.unlock();
    return ans;
}

void Movie::restart(){
    mutex.lock();
    last_video_sys = last_audio_sys = -1;
    last_video_pts = last_audio_pts = -1;
    mutex.unlock();
}

void Movie::set_play_times(double x){
    mutex.lock();
    if(x >= 0.1 && x <= 5){
        play_times = x;
    }
    mutex.unlock();
}

double Movie::current_sys_ms(){
    return (double)(QDateTime::currentDateTime().toMSecsSinceEpoch());
}
