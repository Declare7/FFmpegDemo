
#include "VideoInput.h"

extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
}


VideoInput::VideoInput(std::function<void (const std::string &)> logCallback)
{
    m_logCallBack = logCallback;
    avdevice_register_all();

#ifdef _WIN32
    m_inputFormat = av_find_input_format("dshow");
#endif
}

VideoInput::~VideoInput()
{

}

bool VideoInput::open(const std::string url, std::string videoSize, std::string inputFmt)
{
    std::string totalUrl = "video="+ url;

    AVDictionary *dict = nullptr;
    if(!videoSize.empty())
        av_dict_set(&dict, "video_size", videoSize.c_str(), 0);
    if(!inputFmt.empty())
        av_dict_set(&dict, "input_format", inputFmt.c_str(), 0);

    //ffmpeg的接口返回0是成功，负数是失败；
    int rtn = 1;
    do
    {
        //打开输入的流地址，得到avformatCtx；
        rtn = avformat_open_input(&m_avformatCtx, totalUrl.c_str(), m_inputFormat, &dict);
        if(rtn< 0)
            break;

        //获取视频流的通道；
        rtn = av_find_best_stream(m_avformatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        m_videoStreamIdx = rtn;
        if(rtn< 0)
            break;

        //提取视频流基本信息；
        AVStream *videoStream = m_avformatCtx->streams[m_videoStreamIdx];
        m_width = videoStream->codecpar->width;
        m_height = videoStream->codecpar->height;
        m_fps = rationalToFloat(videoStream->avg_frame_rate);
        printLog("width: "+ std::to_string(m_width));
        printLog("height: "+ std::to_string(m_height));
        printLog("fps: "+ std::to_string(m_fps));

        //获取解码器；
        const AVCodec *avcodec =  avcodec_find_decoder(videoStream->codecpar->codec_id);
        std::string codecName = avcodec->name;
        printLog("codec: "+ codecName);


        //创建编码器上下文；
        m_codecCtx =  avcodec_alloc_context3(avcodec);
        if(m_codecCtx == nullptr)
        {
            rtn = -1;
            printLog("can not create codec context!");
            break;
        }

        //将视频流的编码器信息赋值给编码器上下文；
        rtn = avcodec_parameters_to_context(m_codecCtx, videoStream->codecpar);
        if(rtn< 0)
            break;

        m_codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;  //允许不合规范的加速技巧；
        m_codecCtx->thread_count = 2;               //使用多个线程加速解码；

        //初始化编码器上下文，如果在调用av_alloc_context3时已传入编码器，则参数2可以设置为nullptr；
        rtn = avcodec_open2(m_codecCtx, nullptr, nullptr);
        if(rtn< 0)
            break;

        m_packet = av_packet_alloc();
        if(m_packet == nullptr)
        {
            printLog("alloc avpacket fail!");
            break;
        }

        m_frame = av_frame_alloc();
        m_rgbFrame = av_frame_alloc();
        if(m_frame == nullptr || m_rgbFrame == nullptr)
        {
            printLog("alloc frame fail!");
            break;
        }

        //初始化以RGB32为标准的数据空间；
        int rtn = av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_width, m_height, 1);
        if(rtn< 0)
            break;
        m_rgbBuffer = new unsigned char[rtn];

    }while(0);

    if(dict)
        av_dict_free(&dict);
    if(rtn< 0)
    {
        printErrorInfo(rtn);
        release();
    }

    return rtn >= 0;
}

void VideoInput::close()
{
    std::lock_guard<std::mutex> lck(m_closeMtx);
    release();
}

unsigned char *VideoInput::readRgbData(int &width, int &height)
{
    auto frame = readAVFrame();
    if(frame == nullptr)
        return nullptr;

    //初始化转换器上下文，用作转换输出不同格式的帧数据；
    if(m_swsCtx == nullptr)
    {
        m_swsCtx = sws_getContext(m_width,
                                  m_height,
                                  (AVPixelFormat)frame->format,
                                  m_width,
                                  m_height,
                                  AV_PIX_FMT_RGB32,
                                  SWS_BICUBIC, nullptr, nullptr, nullptr);
    }

    //把AVFrame的图像数据绑定到指定的buffer；
    int rtn = av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_rgbBuffer, AV_PIX_FMT_RGB32, frame->width, frame->height, 1);
    if(rtn< 0)
        return nullptr;

    //YUV-->RGB;
    sws_scale(m_swsCtx,
              (uint8_t const *const*)frame->data,
              frame->linesize,
              0,
              frame->height,
              m_rgbFrame->data,
              m_rgbFrame->linesize);

    width = frame->width;
    height = frame->height;
    return m_rgbBuffer;
}

unsigned char *VideoInput::readYuvData(int &width, int &height, std::string &yuvType)
{
    return nullptr;
}

void VideoInput::printErrorInfo(int errorCode)
{
    if(m_logCallBack == nullptr)
        return;

    memset(m_logBuff, 0, LOG_BUFF_SIZE);
    av_strerror(errorCode, m_logBuff, LOG_BUFF_SIZE);
    std::string errorInfo = m_logBuff;
    printLog(errorInfo);
}

void VideoInput::printLog(const std::string &log)
{
    if(m_logCallBack == nullptr)
        return;

    m_logCallBack(log);
}

float VideoInput::rationalToFloat(const AVRational &rational)
{
    return rational.den == 0? 0 : (float)(rational.num)/rational.den;
}

AVFrame *VideoInput::readAVFrame()
{
    std::lock_guard<std::mutex> lck(m_closeMtx);

    if(m_avformatCtx == nullptr)
        return nullptr;

    int rtn = 0;
    do
    {
        //重置；
        av_packet_unref(m_packet);
        //读取帧数据，存在packet；
        rtn = av_read_frame(m_avformatCtx, m_packet);
        if(rtn< 0)
        {
//            avcodec_send_packet(m_codecCtx, m_packet); // 读取完成后向解码器中传如空AVPacket，否则无法读取出最后几帧;
            break;
        }

        if(m_packet->stream_index != m_videoStreamIdx)
            break;

        m_packet->dts = round(m_packet->dts * (1000 * rationalToFloat(m_avformatCtx->streams[m_videoStreamIdx]->time_base)));
        m_packet->pts = round(m_packet->pts * (1000 * rationalToFloat(m_avformatCtx->streams[m_videoStreamIdx]->time_base)));

        //将packet传给解码器；
        rtn = avcodec_send_packet(m_codecCtx, m_packet);
    }while(0);

    if(rtn< 0)
    {
        printErrorInfo(rtn);
        return nullptr;
    }

    av_frame_unref(m_frame);
    //获取解码后的AVFrame;
    rtn = avcodec_receive_frame(m_codecCtx, m_frame);
    if(rtn< 0)
    {
        return nullptr;
    }

    return m_frame;
}

void VideoInput::release()
{
    if(m_avformatCtx != nullptr)
        avformat_close_input(&m_avformatCtx);

    if(m_packet != nullptr)
        av_packet_free(&m_packet);

    if(m_frame != nullptr)
        av_frame_free(&m_frame);

    if(m_rgbFrame != nullptr)
        av_frame_free(&m_rgbFrame);

    if(m_codecCtx != nullptr)
        avcodec_free_context(&m_codecCtx);

    if(m_rgbBuffer != nullptr)
    {
        delete[] m_rgbBuffer;
        m_rgbBuffer = nullptr;
    }
}

