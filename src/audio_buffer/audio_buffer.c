#include "audio_buffer/audio_buffer.h"
#include "libavutil/error.h"
#include "libavutil/samplefmt.h"
#include <assert.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_par.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <string.h>
typedef struct rdx_audio_buffer {
    void* (*alloc)(size_t);
    void (*dealloc)(void*);
    size_t buffer_size_in_bytes;
    char* data_buffers[3];
    char current_buffer;
    int read_interleaved;
    int sample_rate;
    int num_channels;
    //---
    int audio_stream_index;
    AVCodec const * opened_codec;
    AVCodecContext* codec_ctx;
    AVFormatContext* format_ctx;
    AVPacket* packet;
    AVFrame* frame;
    int bytes_per_sample;
    //---
} rdx_audio_buffer;

rdx_audio_buffer* rdx_create_audio_stream(size_t buffer_size_in_bytes,bool read_interleaved, void* (*alloc)(size_t), void (*dealloc)(void*)) {
    rdx_audio_buffer* new_buffer = alloc(sizeof *new_buffer );
    new_buffer->alloc = alloc;
    new_buffer->dealloc = dealloc;
    new_buffer->buffer_size_in_bytes = buffer_size_in_bytes;
    new_buffer->data_buffers[0] = alloc(buffer_size_in_bytes);
    new_buffer->data_buffers[1] = alloc(buffer_size_in_bytes);
    new_buffer->data_buffers[2] = alloc(buffer_size_in_bytes);
    new_buffer->current_buffer = 0;
    new_buffer->read_interleaved = read_interleaved;
    new_buffer->sample_rate = 0;
    return new_buffer;
}

rdx_audio_buffer_status rdx_open(rdx_audio_buffer* buf, const char* file, void** out_data, size_t* out_data_size) {
    AVFormatContext* ctx = avformat_alloc_context();
    if(!ctx) {
        return AB_ERR_NO_MEM;
    }
    if(avformat_open_input(&ctx, file, NULL, NULL) != 0) {
        return AB_ERR_FILE_ACCESS;
    }
    if(avformat_find_stream_info(ctx, NULL) < 0) {
        return AB_ERR_UNKNWN_FILE_FMT;
    }
    AVCodec const * codec = NULL;
    AVCodecParameters* codec_params = NULL;
    int audio_stream_index = -1;
    for(unsigned int i =0; i < ctx->nb_streams; ++i) {
        AVCodecParameters* local_params = ctx->streams[i]->codecpar;
        if(local_params->codec_type != AVMEDIA_TYPE_AUDIO) {
            continue;
        }
        AVCodec const * local_codec = avcodec_find_decoder(local_params->codec_id);
        if(!local_codec) {
            return AB_ERR_UNKNWN_FILE_FMT;
        }
        codec = local_codec;
        codec_params = local_params;
        audio_stream_index = i;
        break;
    }
    buf->audio_stream_index = audio_stream_index;
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if(!codec_ctx) {
        avformat_free_context(ctx);
        return AB_ERR_NO_MEM;
    }
    if(avcodec_parameters_to_context(codec_ctx,codec_params) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_free_context(ctx);
        return AB_UKNWN_ERR;
    }
    if(avcodec_open2(codec_ctx, codec,NULL) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_free_context(ctx);
        return AB_ERR_UNKNWN_FILE_FMT;
    }
    AVFrame* frame = av_frame_alloc();
    if(!frame) {
        avcodec_free_context(&codec_ctx);
        avformat_free_context(ctx);
        return AB_ERR_NO_MEM;
    }
    AVPacket* packet = av_packet_alloc();
    if(!packet) {
        avcodec_free_context(&codec_ctx);
        avformat_free_context(ctx);
        av_frame_free(&frame);
        return AB_ERR_NO_MEM;
    }
    buf->bytes_per_sample = av_get_bytes_per_sample(buf->codec_ctx->sample_fmt);
    if(buf->bytes_per_sample < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_free_context(ctx);
        av_frame_free(&frame);
        return AB_UKNWN_ERR;
    }
    buf->codec_ctx = codec_ctx;
    buf->codec_ctx->max_samples = buf->buffer_size_in_bytes / (av_get_bytes_per_sample(codec_ctx->sample_fmt) / codec_ctx->ch_layout.nb_channels);
    buf->num_channels = codec_ctx->ch_layout.nb_channels;
    buf->packet = packet;
    buf->frame = frame;
    buf->format_ctx = ctx;
    buf->sample_rate = codec_ctx->sample_rate;
    buf->opened_codec = codec;
    return rdx_fill_next_buffer(buf, out_data, out_data_size);
}

rdx_audio_buffer_status rdx_close(rdx_audio_buffer* buf) {
    avcodec_close(buf->codec_ctx);
    avformat_close_input(&buf->format_ctx);
    av_frame_free(&buf->frame);
    av_packet_free(&buf->packet);
    avcodec_free_context(&buf->codec_ctx);
    avformat_free_context(buf->format_ctx);
    buf->sample_rate = 0;
    buf->bytes_per_sample = 0;
    buf->audio_stream_index = 0;
    buf->num_channels = 0;
    return AB_OK;
}

rdx_audio_buffer_status rdx_fill_next_buffer(rdx_audio_buffer* buf, void** out_data, size_t* out_data_size) {
    size_t available_bytes = buf->buffer_size_in_bytes;
    size_t bytes_written = 0;
    if(av_read_frame(buf->format_ctx,buf->packet) >= 0) {
        int ret;
        do {
            ret = avcodec_send_packet(buf->codec_ctx, buf->packet);
            ret = avcodec_receive_frame(buf->codec_ctx,buf->frame);
        } while (ret == AVERROR(EAGAIN));
        void* data_to_write_to = buf->data_buffers[(int)buf->current_buffer];
        if(buf->read_interleaved) {
            size_t bytes_to_write = buf->bytes_per_sample;
            for(int s =0; s < buf->frame->nb_samples; ++s) {
                for(int ch = 0; ch < buf->num_channels; ++ch) {
                    memcpy(((char*)data_to_write_to)+bytes_written, buf->frame->data[ch], bytes_to_write);
                    bytes_written += bytes_to_write;
                }
            }
        }else {
            size_t bytes_to_write = buf->frame->nb_samples * buf->bytes_per_sample;
            for(int ch = 0; ch < buf->num_channels; ++ch) {
                assert(bytes_written + bytes_to_write < available_bytes);
                memcpy(((char*)data_to_write_to) + bytes_written, buf->frame->data[ch], bytes_to_write);
                bytes_written += bytes_to_write;
            }
        }
        av_packet_unref(buf->packet);
        *out_data = buf->data_buffers[(int)buf->current_buffer];
        *out_data_size = (unsigned int)bytes_written;
    }
    return AB_OK;
}

void rdx_destroy_audio_stream(rdx_audio_buffer* buf) {
    void (*dealloc)(void*) = buf->dealloc;
    dealloc(buf->data_buffers[0]);
    dealloc(buf->data_buffers[1]);
    dealloc(buf->data_buffers[2]);
    dealloc(buf);
}