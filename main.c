//
// Created by wiggers on 2020/11/14.
//

#include <libavutil/log.h>
#include <libavutil/rational.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>

#include <stdio.h>
#include <assert.h>

static void simple_capture_audio()
{
    /*
     * 输入设备列表
     * libavdevice/indev_list.c
     * static const AVInputFormat * const indev_list[] = {
     *      &ff_avfoundation_demuxer,
     *      &ff_lavfi_demuxer,
     *      NULL };
     *
     * 输出设备列表
     * libavdevice/outdev_list.c
     * static const AVOutputFormat * const outdev_list[] = {
     *      &ff_audiotoolbox_muxer,
     *      &ff_opengl_muxer,
     *      &ff_sdl2_muxer,
     *      NULL };
     *
     * 封装器列表
     * libavformat/muxer_list.c
     * static const AVOutputFormat * const muxer_list[] = {
     *      &ff_a64_muxer,
     *      &ff_ac3_muxer,
     *      &ff_adts_muxer,
     *      &ff_adx_muxer,
     *      ...,
     *      NULL };
     *
     * 解封装器列表
     * libavformat/demuxer_list.c
     * static const AVInputFormat * const demuxer_list[] = {
     *      &ff_aa_demuxer,
     *      &ff_aac_demuxer,
     *      &ff_aax_demuxer,
     *      &ff_ac3_demuxer,
     *      ...,
     *      NULL };
     *
     * avdevice_register_all函数就是将以上数据进行注册
     * */
    avdevice_register_all();

    // 从avdevice_register_all函数注册的输入设备和解封装器列表中进行搜索，并返回有参数short_name指定的数据
    // short_name参数的取值可以查阅上述indev_list和demuxer_list数组中每个成员的name属性的值
    AVInputFormat *input_format = av_find_input_format("avfoundation");
    if (!input_format) {
        av_log(NULL, AV_LOG_ERROR, "av_find_input_format failed.");
        return;
    }

    // 分配AVFormatContext
    AVFormatContext *format_context = avformat_alloc_context();
    if (!format_context) {
        av_log(NULL, AV_LOG_ERROR, "avformat_alloc_context failed.");
        return;
    }

    AVDictionary *options = NULL;

    // avformat_open_input会执行如下判断
    // if (format_context == NULL) format_context = avformat_alloc_context();
    // url 参数指定输入数据，可以是本地文件、rtmp流、rtsp流、rtp流等
    // fmt 参数是有效值，则直接使用该fmt进行输入数据的解析，否则会自动解析url参数指定的数据，来判断fmt应该是什么数据
    // options 参数可以设置采样率、采样大小、分辨率等信息
    //      具体内容可通过fmt结构中的priv_class字段中的options参数查看

    int ret = avformat_open_input(&format_context, "0", input_format, NULL);
    if (ret) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed, error:%s", av_err2str(ret));
        return;
    }

    AVPacket *pkt = av_packet_alloc();
    pkt->data = NULL;
    pkt->size = 0;

    FILE *output_fd = fopen("out.pcm", "wb+");
    assert(output_fd);

    while (1) {
        ret = av_read_frame(format_context, pkt);
        if (ret < 0) {
            if (ret == -35) {
                continue;
            }

            break;
        }
        fwrite(pkt->data, pkt->size, 1, output_fd);
        fflush(output_fd);
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    avformat_close_input(&format_context);
}

static void open_input_media_file(const char *filename)
{
    AVFormatContext *format_context = NULL;
    int ret = 0;

    ret = avformat_open_input(&format_context, filename, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed, error:%s", av_err2str(ret));
        return;
    }

    av_dump_format(format_context, 0, filename, 0);
    avformat_close_input(&format_context);
}

static void open_input_media_from_net(const char *url)
{
    AVFormatContext *format_context = NULL;
    int ret = 0;

    ret = avformat_open_input(&format_context, url, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed, error:%s", av_err2str(ret));
        return;
    }

    av_dump_format(format_context, 0, url, 0);
    avformat_close_input(&format_context);

}

static void capture_media_data_from_input_device()
{
    AVFormatContext *format_context = NULL;
    AVInputFormat *input_format = NULL;
    int ret = 0;

    input_format = av_find_input_format("avfoundation");
    if (!input_format) {
        av_log(NULL, AV_LOG_ERROR, "av_find_input_format failed\n");
        return;
    }

    ret = avformat_open_input(&format_context, ":0", input_format, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed, error:%s", av_err2str(ret));
        return;
    }

    avformat_close_input(&format_context);
}

static void print_pts_and_dts(const char *filename)
{
    av_log_set_level(AV_LOG_DEBUG);

    AVFormatContext *format_context = NULL;

    int ret = avformat_open_input(&format_context, filename, NULL, NULL);
    assert(ret == 0);

    int audio_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    assert(audio_stream_index >= 0);

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    for (int i = 0; i < format_context->nb_streams; i++) {
        AVStream *stream = format_context->streams[i];
        av_log(NULL, AV_LOG_INFO, "%d %d\n", stream->time_base.den, stream->time_base.num);
    }

    while (1) {
        ret = av_read_frame(format_context, &pkt);
        if (ret < 0) {
            break;
        }

        if (pkt.stream_index == audio_stream_index) {
            AVRational *time_base = &format_context->streams[audio_stream_index]->time_base;
            av_log(NULL, AV_LOG_INFO, "pts: %lld %s, dts: %lld %s\n",
                   pkt.pts, av_ts2timestr(pkt.pts, time_base), pkt.dts, av_ts2timestr(pkt.dts, time_base));
        }
        av_packet_unref(&pkt);
    }

    avformat_close_input(&format_context);
}

static void dump_format(const char *filename)
{
    AVFormatContext *format_context = NULL;

    int ret = avformat_open_input(&format_context, filename, NULL, NULL);
    assert(ret == 0);

    av_dump_format(format_context, 0, filename, 0);
}

static void abstract_h264_from_mediafile(const char *input_filename, const char *output_filename)
{
    AVFormatContext *input_format_context = NULL;
    int ret = avformat_open_input(&input_format_context, input_filename, NULL, NULL);
    assert(ret == 0);

    ret = avformat_find_stream_info(input_format_context, NULL);
    assert(ret >= 0);

    av_dump_format(input_format_context, 0, input_filename, 0);

    AVFormatContext *output_format_context = NULL;
    ret = avformat_alloc_output_context2(&output_format_context, NULL, NULL, output_filename);
    assert(ret == 0);

    AVPacket *pkt = av_packet_alloc();

    int stream_index = 0;
    int stream_mapping_size = input_format_context->nb_streams;
    int *stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    assert(stream_mapping);

    for (int i = 0; i < input_format_context->nb_streams; i++) {
        AVStream *input_stream = input_format_context->streams[i];
        AVCodecParameters *in_codecpar = input_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE
            ) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        AVStream *output_stream = avformat_new_stream(output_format_context, NULL);
        assert(output_stream);

        ret = avcodec_parameters_copy(output_stream->codecpar, in_codecpar);
        assert(ret >= 0);
        output_stream->codecpar->codec_tag = 0;
    }

    av_dump_format(output_format_context, 0, output_filename, 1);

    if (!(output_format_context->flags&AVFMT_NOFILE)) {
        ret = avio_open(&output_format_context->pb, output_filename, AVIO_FLAG_WRITE);
        assert(ret >= 0);
    }

    ret = avformat_write_header(output_format_context, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_write_header failed, error:%s", av_err2str(ret));
        return;
    }
    while (1) {
        ret = av_read_frame(input_format_context, pkt);
        if (ret < 0) {
            break;
        }

        AVStream *in_stream = input_format_context->streams[pkt->stream_index];
        assert(in_stream);

        if (pkt->stream_index >= stream_mapping_size ||
            stream_mapping[pkt->stream_index] < 0) {
            av_packet_unref(pkt);
            continue;
        }

        pkt->stream_index = stream_mapping[pkt->stream_index];
        AVStream *out_stream = output_format_context->streams[pkt->stream_index];

        /* copy packet */
        pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;

        ret = av_interleaved_write_frame(output_format_context, pkt);
        assert(ret >= 0);
        av_packet_unref(pkt);
    }
    av_write_trailer(output_format_context);

    av_packet_free(&pkt);
    avformat_close_input(&input_format_context);
    avformat_free_context(output_format_context);
}

static void abstract_h264_from_mediafile2(const char *input_filename, const char *output_filename, enum AVMediaType type)
{
    AVFormatContext *input_format_context = NULL;
    int ret = avformat_open_input(&input_format_context, input_filename, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed, error: [%d]%s\n", ret, av_err2str(ret));
        return;
    }

    ret = avformat_find_stream_info(input_format_context, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_find_stream_info failed, error: [%d]%s\n", ret, av_err2str(ret));
        goto end;
    }

    av_dump_format(input_format_context, 0, input_filename, 0);

    AVFormatContext *output_format_context = NULL;
    ret = avformat_alloc_output_context2(&output_format_context, NULL, NULL, output_filename);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_alloc_output_context2 failed, error: [%d]%s\n", ret, av_err2str(ret));
        goto end;
    }


    int video_stream_index = av_find_best_stream(input_format_context, type, -1, -1, NULL, 0);
    if (video_stream_index < 0) {
        av_log(NULL, AV_LOG_ERROR, "av_find_best_stream failed, error:%s\n", av_err2str(video_stream_index));
        goto end;
    }

    AVStream *input_stream = input_format_context->streams[video_stream_index];
    AVCodecParameters *in_codecpar = input_stream->codecpar;

    AVStream *output_stream = avformat_new_stream(output_format_context, NULL);
    if (!output_stream) {
        av_log(NULL, AV_LOG_ERROR, "avformat_new_stream failed.\n");
        goto end;
    }

    ret = avcodec_parameters_copy(output_stream->codecpar, in_codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_copy failed, error: [%d]%s\n", ret, av_err2str(ret));
        goto end;
    }
    output_stream->codecpar->codec_tag = 0;

    av_dump_format(output_format_context, 0, output_filename, 1);

    if (!(output_format_context->flags&AVFMT_NOFILE)) {
        ret = avio_open(&output_format_context->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "avio_open failed, error: [%d]%s\n", ret, av_err2str(ret));
            goto end;
        }
    }

    ret = avformat_write_header(output_format_context, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_write_header failed, error: [%d]%s\n", ret, av_err2str(ret));
        goto end;
    }

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    while (1) {
        ret = av_read_frame(input_format_context, &pkt);
        if (ret < 0) {
            break;
        }

        if (pkt.stream_index != video_stream_index) {
            av_packet_unref(&pkt);
            continue;
        }

        AVStream *in_stream = input_format_context->streams[pkt.stream_index];
        assert(in_stream);

        pkt.stream_index = 0;
        AVStream *out_stream = output_format_context->streams[pkt.stream_index];

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        ret = av_interleaved_write_frame(output_format_context, &pkt);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_interleaved_write_frame failed, error: [%d]%s\n", ret, av_err2str(ret));
            av_packet_unref(&pkt);
            goto end;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(output_format_context);

    end:
    avformat_close_input(&input_format_context);

    if (output_format_context && !(output_format_context->flags&AVFMT_NOFILE)) {
        avio_close(output_format_context->pb);
    }

    if (output_format_context) {
        avformat_free_context(output_format_context);
    }
}

int main(int argc, char *argv[])
{
    av_log_set_level(AV_LOG_DEBUG);
    // simple_capture_audio();
    // print_pts_and_dts("input.mp4");
    // dump_format("input.mp4");
    abstract_h264_from_mediafile2("input.mp4", "output.aac", AVMEDIA_TYPE_AUDIO);
    return 0;
}