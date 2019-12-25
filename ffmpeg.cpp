// in case of 无法解析的外部符号 "struct AVFormatContext * __cdecl avformat_alloc_context(void)"
// add the shared dll to debug dir in case of crashed exit
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
#include "ffmpeg.h"

static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, unsigned char * data){
    // Supply raw packet data as input to a decoder
    int response = avcodec_send_packet(pCodecContext, pPacket);
    if(response < 0){
        printf("Error while sending a packet to the decoder: %d\n", response);
        return response;
    }

    while(response >= 0){
        // Return decoded output data (into a frame) from a decoder
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF){
            printf("Reutrn here\n");
            break;
        }
        else if(response < 0){
            printf("Error while receiving a frame from the decoder: %d\n", response);
            return response;
        }

        if (response >= 0){
            printf("Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]\n",
                pCodecContext->frame_number,
                av_get_picture_type_char(pFrame->pict_type),
                pFrame->pkt_size,
                pFrame->pts,
                pFrame->key_frame,
                pFrame->coded_picture_number);

            int linesize = pFrame->linesize[0];
            data = pFrame->data[0];
            printf("DenotePacket: linesize=%d width=%d height=%d format=%s\n", linesize, pFrame->width, pFrame->height, pFrame->format);

            return 0;


            //char frame_filename[1024];
            //snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
            // save a grayscale frame into a .pgm file
            //save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
        }
    }
    return 0;
}


void read_one_frame(char* movie_name, unsigned char* data, int* channel, int* height, int* width){

    AVFormatContext *pFormatContext = avformat_alloc_context();
    if(!pFormatContext){
        printf("ERROR could not allocate memory for Format Context\n");
        return;
    }

    if(avformat_open_input(&pFormatContext, movie_name, NULL, NULL) != 0){
        printf("ERROR could not open the file %s\n", movie_name);
        return;
    }

    if(avformat_find_stream_info(pFormatContext,  NULL) < 0){
        printf("ERROR could not get the stream info\n");
        return;
    }

    // this component describes the properties of a codec used by the stream i
    AVCodec *pCodec = NULL;
    AVCodecParameters *pCodecParameters =  NULL;
    int video_stream_index = -1;

    for(int i = 0; i < pFormatContext->nb_streams; i++){
        AVCodecParameters *pLocalCodecParameters =  NULL;
        pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        printf("%d AVStream->time_base before open coded %d/%d\n", i, pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
        printf("%d AVStream->r_frame_rate before open coded %d/%d\n", i, pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
        printf("%d AVStream->start_time %" PRId64, i, pFormatContext->streams[i]->start_time);
        printf("%d AVStream->duration %" PRId64, i, pFormatContext->streams[i]->duration);
        printf("\n");

        // finds the registered decoder for a codec ID
        AVCodec *pLocalCodec = NULL;
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
        if(pLocalCodec==NULL){
          printf("ERROR unsupported codec!\n");
          return;
        }

        // when the stream is a video we store its index, codec parameters and codec
        if(pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO){
          if (video_stream_index == -1){
            video_stream_index = i;
            pCodec = pLocalCodec;
            pCodecParameters = pLocalCodecParameters;
          }
          printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
        }
        else if(pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO){
          printf("Audio Codec: %d channels, sample rate %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
        }
        printf("\tCodec %s ID %d bit_rate %lld \n", pLocalCodec->name, pLocalCodec->id, pCodecParameters->bit_rate);
    }

    *height = pCodecParameters->height;
    *width = pCodecParameters->width;

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if(!pCodecContext){
        printf("failed to allocated memory for AVCodecContext\n");
        return;
    }

    // Fill the codec context based on the values from the supplied codec parameters
    if(avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0){
        printf("failed to copy codec params to codec context\n");
        return;
    }

    // Initialize the AVCodecContext to use the given AVCodec.
    if(avcodec_open2(pCodecContext, pCodec, NULL) < 0){
        printf("failed to open codec through avcodec_open2\n");
        return;
    }

    AVFrame *pFrame = av_frame_alloc();
    if(!pFrame){
        printf("failed to allocated memory for AVFrame\n");
        return;
    }

    AVPacket *pPacket = av_packet_alloc();
    if(!pPacket){
        printf("failed to allocated memory for AVPacket\n");
        return;
    }

    int response = 0;
    int how_many_packets_to_process = 8;

    // fill the Packet with data from the Stream
    while(av_read_frame(pFormatContext, pPacket) >= 0){
        // if it's the video stream
        if (pPacket->stream_index == video_stream_index){
            printf("AVPacket->pts %" PRId64, pPacket->pts);
            printf("\n");
            response = decode_packet(pPacket, pCodecContext, pFrame, data);
            //break;
            if (response < 0) break;
            // stop it, otherwise we'll be saving hundreds of frames
            if (--how_many_packets_to_process <= 0) break;
        }
        av_packet_unref(pPacket);
    }

    printf("releasing all the resources\n");
    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);
    return;
}
