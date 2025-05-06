#include "raylib.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

#include <stdint.h>
#include <assert.h>

#define WIDTH 1280
#define HEIGHT 720

static uint8_t fbuf_rgb24[WIDTH * HEIGHT * 3];

int main()
{
    int err;

    InitWindow(WIDTH, HEIGHT, "player");
    Image color_image = (Image) {
        .data    = fbuf_rgb24,
        .width   = WIDTH,
        .height  = HEIGHT,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8,
    };
    Texture2D color_tex = LoadTextureFromImage(color_image);

    AVFormatContext *fmt_ctx        = NULL;
    AVCodecContext *codec_ctx       = NULL;
    const AVCodec *codec            = NULL;
    AVCodecParameters *codec_params = NULL;
    struct SwsContext *sws_ctx      = NULL; 
    AVFrame *avframe                = av_frame_alloc();
    AVPacket *avpacket              = av_packet_alloc();

    /* Open video source */
    err = avformat_open_input(&fmt_ctx, "udp://localhost:1337", NULL, NULL);
    if (err != 0) {
        return 1;
    }

    err = avformat_find_stream_info(fmt_ctx, NULL);
    if (err != 0) {
        return 1;
    }

    /* find first video stream */
    int i = 0;
    int video_stream = -1;
    for (i = 0; i < (int) fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            codec_params = fmt_ctx->streams[i]->codecpar;
            codec = avcodec_find_decoder(codec_params->codec_id);
            break;
        }
    }
    if (video_stream == -1) {
        return 1;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        return 1;
    }
    
    err = avcodec_parameters_to_context(codec_ctx, codec_params);
    if (err != 0) {
        return 1;
    }

    err = avcodec_open2(codec_ctx, codec, NULL);
    if (err != 0) {
        return 1;
    }

    sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                             WIDTH, HEIGHT, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR,
                             NULL, NULL, NULL);
    if (sws_ctx == NULL) {
        return 1;
    }

    while (!WindowShouldClose() && !IsKeyPressed(KEY_Q))
    {
        int err;
        err = av_read_frame(fmt_ctx, avpacket);

        /* Error or EOF */
        if (err != 0) {
            break;
        }

        /* Packet does not belong to the video stream -> continue */
        if (avpacket->stream_index != video_stream) {
            av_packet_unref(avpacket);
            continue;
        }

        /* AVERROR(EGAIN) -> skip & try to receive a frame instead */
        err = avcodec_send_packet(codec_ctx, avpacket);
        if ((err != AVERROR(EAGAIN)) && (err != 0)) {
            break;
        }
        av_packet_unref(avpacket);

        /* Receive frame */
        err = avcodec_receive_frame(codec_ctx, avframe);

        /* Output not yet available, try again */
        if (err == AVERROR(EAGAIN)) {
            continue;
        }

        /* End of output */
        if (err == AVERROR_EOF) {
            break;
        }

        /* Other decoding error */
        if (err != 0) {
            break;
        }

        /* convert to RGB24 frame */
        const int stride = WIDTH*3;
        uint8_t *fbuf_rgb24_data[6];
        fbuf_rgb24_data[0] = fbuf_rgb24;
        sws_scale(sws_ctx, (const uint8_t *const *) avframe->data, avframe->linesize, 0, codec_ctx->height, fbuf_rgb24_data, &stride);

        /* draw */
        UpdateTexture(color_tex, fbuf_rgb24);
        BeginDrawing();
            DrawTexture(color_tex, 0, 0, WHITE);
            //DrawFPS(10,10);
        EndDrawing();
    }

    CloseWindow();
}
