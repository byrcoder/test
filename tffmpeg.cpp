// testffmpeg.cpp : Defines the entry point for the console application.
//

// tutorial01.c
// Code based on a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
// With updates from https://github.com/chelyaev/ffmpeg-tutorial
// Updates tested on:
// LAVC 54.59.100, LAVF 54.29.104, LSWS 2.1.101
// on GCC 4.7.2 in Debian February 2015

// A small sample program that shows how to use libavformat and libavcodec to
// read video from a file.
//
// Use
//
// gcc -o tutorial01 tutorial01.c -lavformat -lavcodec -lswscale -lz
//
// to build (assuming libavformat and libavcodec are correctly installed
// your system).
//
// Run using
//
// tutorial01 myvideofile.mpg
//
// to write the first five frames from "myvideofile.mpg" to disk in PPM
// format.
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <stdio.h>
}

#include <iostream>

using namespace std;

namespace Ffmpeg {

#if 0
    void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
        FILE *pFile;
        char szFilename[32];
        int y;

        // Open file
        sprintf(szFilename, "frame%d.ppm", iFrame);
        pFile = fopen(szFilename, "wb");
        if (pFile == NULL)
            return;

        // Write header
        fprintf(pFile, "P6\n%d %d\n255\n", width, height);

        // Write pixel data
        for (y = 0; y < height; y++)
            fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

        // Close file
        fclose(pFile);
    }

    int test_codec(int argc, char *argv[]) {
        // Initalizing these to NULL prevents segfaults!
        AVFormatContext *pFormatCtx = NULL;
        int i, videoStream;
        AVCodecParameters *pCodecpar = NULL;
        AVCodecContext *pCodecCtx = NULL;
        AVCodec *pCodec = NULL;
        AVFrame *pFrame = NULL;
        AVFrame *pFrameRGB = NULL;
        AVPacket packet;
        int numBytes;
        uint8_t *buffer = NULL;
        struct SwsContext *sws_ctx = NULL;

        // Register all formats and codecs
        av_register_all();
        //debug程序需要将test.flv放在对应的project目录下，跟引用的ffmpeg的dll库同一目录
        char* filepath = argv[1];
        // Open video file
        if ((i = avformat_open_input(&pFormatCtx, filepath, NULL, NULL) )!= 0) {
            cerr << filepath << " not exits " << i << " " << strerror(errno) << endl;
            return -1; // Couldn't open file
        }
        bool found_stream = true;
        // Retrieve stream information
        if ( found_stream && avformat_find_stream_info(pFormatCtx, NULL) < 0) {
            cerr << filepath << " avformat_find_stream_info failed" << endl;
            return -1; // Couldn't find stream information
        }

        // Dump information about file onto standard error
        av_dump_format(pFormatCtx, 0, filepath, 0);

        // Find the first video stream
        videoStream = -1;
        for (i = 0; i < pFormatCtx->nb_streams; i++)
            if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStream = i;
                break;
            }

        cout << " videostream " << videoStream << endl;

        if (videoStream == -1)
            return -1; // Didn't find a video stream

        // Get a pointer to the codec context for the video stream
        pCodecpar = pFormatCtx->streams[videoStream]->codecpar;

        cout << " codeid " << pFormatCtx->streams[videoStream]->codecpar->codec_id << endl;

        // Find the decoder for the video stream
        pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
        if (pCodec == NULL) {
            cerr << " Unsupported codec! " << endl;
            return -1; // Codec not found
        }

        // Copy context
        pCodecCtx = avcodec_alloc_context3(pCodec);
        if (avcodec_parameters_to_context(pCodecCtx, pCodecpar) != 0) {
            cerr << "Couldn't copy codec context" << endl;
            return -1; // Error copying codec context
        }

        // Open codec
        if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
            cerr << "avcodec_open2 Couldn't open codec context" << endl;
            return -1; // Could not open codec
        }

        // Allocate video frame
        pFrame = av_frame_alloc();

        // Allocate an AVFrame structure
        pFrameRGB = av_frame_alloc();
        if (pFrameRGB == NULL)
            return -1;

        // Determine required buffer size and allocate buffer
        numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
                                            pCodecCtx->height, 1);
        buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

        // Assign appropriate parts of buffer to image planes in pFrameRGB
        // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
        // of AVPicture
        av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24,
                             pCodecCtx->width, pCodecCtx->height, 1);

        // initialize SWS context for software scaling
        sws_ctx = sws_getContext(pCodecCtx->width,
                                 pCodecCtx->height,
                                 pCodecCtx->pix_fmt,
                                 pCodecCtx->width,
                                 pCodecCtx->height,
                                 AV_PIX_FMT_RGB24,
                                 SWS_BILINEAR,
                                 NULL,
                                 NULL,
                                 NULL
        );

        // Read frames and save first five frames to disk
        i = 0;
        while (av_read_frame(pFormatCtx, &packet) >= 0) {

            cout << i << ". av_read_frame success " << packet.stream_index  << endl;

            if (i > 3) {
                break;
            }
            // Is this a packet from the video stream?
            if (packet.stream_index == videoStream) {
                // Decode video frame
                avcodec_send_packet(pCodecCtx, &packet);
                if (avcodec_receive_frame(pCodecCtx, pFrame) != 0)
                    continue;

                // Convert the image from its native format to RGB
                sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
                          pFrameRGB->data, pFrameRGB->linesize);

                // Save the frame to disk
                if (++i <= 10)
                    SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
            }

            // Free the packet that was allocated by av_read_frame
            av_packet_unref(&packet);
        }

        // Free the RGB image
        av_free(buffer);
        av_frame_free(&pFrameRGB);

        // Free the YUV frame
        av_frame_free(&pFrame);

        // Close the codecs
        avcodec_close(pCodecCtx);

        // Close the video file
        avformat_close_input(&pFormatCtx);

        return 0;
    }
#endif

    // 封装ffmpeg是怎么用的
    // https://blog.csdn.net/leixiaohua1020/article/details/25422685
    int test_mux(char* in_filename, char* out_filename) {

        int  i             = 0;
        int ret            = 0;
        int frame_index    = 0;

        AVPacket pkt;
        AVFormatContext *ifmt_ctx = NULL;
        AVFormatContext *ofmt_ctx = NULL;
        AVOutputFormat  *ofmt     = NULL;

        //输入（Input）
        if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
            printf("Could not open input file.\n");
            return ret;
        }

        if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
            printf( "Failed to retrieve input stream information");
            goto end;
        }
        av_dump_format(ifmt_ctx, 0, in_filename, 0);

        //输出（Output）
        avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
        if (!ofmt_ctx) {
            printf( "Could not create output context\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ofmt = ofmt_ctx->oformat;

        //
        for (i = 0; i < ifmt_ctx->nb_streams; i++) {
            //根据输入流创建输出流（Create output AVStream according to input AVStream）
            AVStream *in_stream = ifmt_ctx->streams[i];
            AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
            if (!out_stream) {
                printf( "Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                goto end;
            }
            //复制AVCodecContext的设置（Copy the settings of AVCodecContext）
            ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
            if (ret < 0) {
                printf( "Failed to copy context from input to output stream codec context\n");
                goto end;
            }
            out_stream->codec->codec_tag = 0;
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        //输出一下格式------------------
        av_dump_format(ofmt_ctx, 0, out_filename, 1);

        //打开输出文件（Open output file）
        if (!(ofmt->flags & AVFMT_NOFILE)) {
            ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
            if (ret < 0) {
                printf( "Could not open output file '%s'", out_filename);
                goto end;
            }
        }
        //写文件头（Write file header）
        ret = avformat_write_header(ofmt_ctx, NULL);
        if (ret < 0) {
            printf( "Error occurred when opening output file\n");
            goto end;
        }

        while (1) {
            AVStream *in_stream, *out_stream;
            //获取一个AVPacket（Get an AVPacket）
            ret = av_read_frame(ifmt_ctx, &pkt);
            if (ret < 0)
                break;
            in_stream  = ifmt_ctx->streams[pkt.stream_index];
            out_stream = ofmt_ctx->streams[pkt.stream_index];
            /* copy packet */
            //转换PTS/DTS（Convert PTS/DTS）
            pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
            pkt.pos = -1;
            //写入（Write）
            ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
            if (ret < 0) {
                printf( "Error muxing packet\n");
                break;
            }
            printf("Write %8d frames to output file\n",frame_index);
            av_free_packet(&pkt);
            frame_index++;
        }
        //写文件尾（Write file trailer）
        av_write_trailer(ofmt_ctx);

    end:
        avformat_close_input(&ifmt_ctx);
        /* close output */
        if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
            avio_close(ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        if (ret < 0 && ret != AVERROR_EOF) {
            printf( "Error occurred.\n");
            return -1;
        }


        return 0;
    }

    static char* iobuffer      = NULL;
    int          iobuffer_size =  1 * 1024 * 1024;

    FILE         *fp_open      = NULL;
    FILE         *fp_out       = NULL;



    //读取数据的回调函数-------------------------
    //AVIOContext使用的回调函数！
    //注意：返回值是读取的字节数
    //手动初始化AVIOContext只需要两个东西：内容来源的buffer，和读取这个Buffer到FFmpeg中的函数
    //回调函数，功能就是：把buf_size字节数据送入buf即可
    //第一个参数(void *opaque)一般情况下可以不用
    int fill_iobuffer(void * opaque,uint8_t *buf, int bufsize) {
        if (!feof(fp_open)) {
            int true_size = fread(buf, 1, bufsize, fp_open);
            return true_size;
        }

        return -1;
    }

    int write_buffer(void *opaque, uint8_t *buf, int buf_size){
        if (fp_out) {
            printf("write buf_size:%d\n", buf_size);
            return fwrite(buf, 1, buf_size, fp_out);
        }

        printf("write buffer failed");
        return -1;
    }

    // 复制上一段代码，只不过从内存io读取和写入内存
    // https://blog.csdn.net/leixiaohua1020/article/details/12980423
    int test_remux_memory(char* in_filename, char* out_filename) {
        int  i             = 0;
        int ret            = 0;
        int frame_index    = 0;

        AVPacket pkt;
        AVFormatContext *ifmt_ctx = NULL;
        AVFormatContext *ofmt_ctx = NULL;
        AVOutputFormat  *ofmt     = NULL;

        printf("avformat find success1\r\n");

        unsigned char* outbuffer = (unsigned char*)av_malloc(iobuffer_size);
        AVIOContext*   avio_out    = NULL;


            // 输入初始化
        ifmt_ctx                  = avformat_alloc_context();
        unsigned char* iobuffer   = (unsigned char *) av_malloc(iobuffer_size);
        AVIOContext*    avio      = avio_alloc_context(iobuffer, iobuffer_size, 0,NULL, fill_iobuffer, NULL,NULL);

        fp_open = fopen(in_filename,"rb+");
        fp_out  = fopen(out_filename, "wb+");

        if (fp_open == NULL) {
            printf("fp_open in failed\n");
            goto end;
        }

        if (fp_out == NULL) {
            printf("fp_open out failed\n");
            goto end;
        }

        ifmt_ctx->pb = avio;  //自定义io操作

        //输入（Input）
        if ((ret = avformat_open_input(&ifmt_ctx, "nothing", 0, 0)) < 0) {
            printf("Could not open input file.\n");
            return ret;
        }

        printf("avformat find success2\r\n");

        if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
            printf( "Failed to retrieve input stream information");
            goto end;
        }

        printf("avformat find success1 \n");
        av_dump_format(ifmt_ctx, 0, "inxx.flv", 0);

        // 输出初始化
        //输出（Output）
        avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", NULL);
        if (!ofmt_ctx) {
            printf( "Could not create output context\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        avio_out              = avio_alloc_context(outbuffer, iobuffer_size, 1, NULL, NULL, write_buffer, NULL);
        ofmt_ctx->pb          = avio_out;
        ofmt_ctx->flags       = AVFMT_FLAG_CUSTOM_IO;
        ofmt                  = ofmt_ctx->oformat;

        //
        for (i = 0; i < ifmt_ctx->nb_streams; i++) {
            //根据输入流创建输出流（Create output AVStream according to input AVStream）
            AVStream *in_stream = ifmt_ctx->streams[i];
            AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
            if (!out_stream) {
                printf( "Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                goto end;
            }
            //复制AVCodecContext的设置（Copy the settings of AVCodecContext）
            ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
            if (ret < 0) {
                printf( "Failed to copy context from input to output stream codec context\n");
                goto end;
            }
            out_stream->codec->codec_tag = 0;
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        //输出一下格式------------------
        av_dump_format(ofmt_ctx, 0, out_filename, 1);

        //写文件头（Write file header）
        ret = avformat_write_header(ofmt_ctx, NULL);
        if (ret < 0) {
            printf( "Error occurred when opening output file\n");
            goto end;
        }

        while (1) {
            AVStream *in_stream, *out_stream;
            //获取一个AVPacket（Get an AVPacket）
            ret = av_read_frame(ifmt_ctx, &pkt);
            if (ret < 0)
                break;
            in_stream  = ifmt_ctx->streams[pkt.stream_index];
            out_stream = ofmt_ctx->streams[pkt.stream_index];
            /* copy packet */
            //转换PTS/DTS（Convert PTS/DTS）
            pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
            pkt.pos = -1;
            //写入（Write）
            ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
            if (ret < 0) {
                printf( "Error muxing packet\n");
                break;
            }
            printf("Write %8d frames to output file\n",frame_index);
            av_free_packet(&pkt);
            frame_index++;
        }
        //写文件尾（Write file trailer）
        av_write_trailer(ofmt_ctx);

        end:
        avformat_close_input(&ifmt_ctx);
        /* close output */
        if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
            avio_close(ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        if (ret < 0 && ret != AVERROR_EOF) {
            printf( "Error occurred.\n");
            return -1;
        }


        return 0;
    }

    int test(int argc, char *argv[]) {

        printf("ffmpeg test remux in:%s, out:%s\n", argv[1], argv[2]);
        test_remux_memory(argv[1], argv[2]);
        //test_mux(argv[1], argv[2]);

        return 0;
    }
}