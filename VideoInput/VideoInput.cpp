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

AVPixelFormat   gHwPixelFormat = AV_PIX_FMT_NONE;       //使用的硬解码器格式；
AVHWDeviceType  gHwDeviceType = AV_HWDEVICE_TYPE_NONE;  //硬解码设备类型；
enum AVPixelFormat get_hw_format(struct AVCodecContext *s, const enum AVPixelFormat * fmt)
{
    (void)s;
    for (const enum AVPixelFormat *p = fmt; *p != -1; p++)
    {
        if (*p == gHwPixelFormat)
            return *p;
    }
    return AV_PIX_FMT_NONE;
}


VideoInput::VideoInput(std::function<void (const std::string &)> logCallback)
{
    m_logCallBack = logCallback;
    avdevice_register_all();
}

VideoInput::~VideoInput()
{

}

bool VideoInput::open(const std::string url, std::string inputFormat, std::string videoSize)
{
    std::string totalUrl = "video="+ url;

    AVDictionary *dict = nullptr;
    if(!videoSize.empty())
        av_dict_set(&dict, "video_size", videoSize.c_str(), 0);
    if(!inputFormat.empty())
        m_inputFormat = av_find_input_format(inputFormat.c_str());

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

        //获取硬解码格式；
        gHwPixelFormat = (AVPixelFormat)getHWDecoderPixelFmt((void*)avcodec);

        //创建解码器上下文；
        m_codecCtx =  avcodec_alloc_context3(avcodec);
        if(m_codecCtx == nullptr)
        {
            rtn = -1;
            printLog("can not create codec context!");
            break;
        }

        //将视频流的编码器信息赋值给解码器上下文；
        rtn = avcodec_parameters_to_context(m_codecCtx, videoStream->codecpar);
        if(rtn< 0)
            break;

        m_codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;  //允许不合规范的加速技巧；
        m_codecCtx->thread_count = 2;               //使用多个线程加速解码；

        //支持硬解码；
        if(gHwPixelFormat != AV_PIX_FMT_NONE)
        {
            printLog("Hw device type: " + std::to_string(gHwDeviceType));
            rtn = av_hwdevice_ctx_create(&m_hwDevContext, gHwDeviceType, "", nullptr, 0);
            if(rtn == 0)
            {
                //设置硬解码格式；
                m_codecCtx->get_format = get_hw_format;
                m_codecCtx->hw_device_ctx = av_buffer_ref(m_hwDevContext);
            }
            else
            {
                printErrorInfo(rtn);
                gHwPixelFormat = AV_PIX_FMT_NONE;
            }
        }

        //初始化解码器上下文，如果在调用avcodec_alloc_context3时已传入解码器，则参数2可以设置为nullptr；
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
        if(m_frame == nullptr)
        {
            printLog("alloc frame fail!");
            break;
        }

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

unsigned char *VideoInput::readSpecFormatData(const PixelFormatType &pixelFormat, int &width, int &height)
{
    auto frame = readAVFrame();
    if(frame == nullptr)
        return nullptr;

    AVPixelFormat pixFmt = (AVPixelFormat)pixelFormat;

    //已创建的sws context指定的像素格式发生变化;
    if(m_swsCtx != nullptr && m_orgPixelFormat != frame->format)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;

        if(m_converBuff != nullptr)
        {
            delete[] m_converBuff;
            m_converBuff = nullptr;
        }

        av_frame_free(&m_converFrame);
    }

    int rtn = -1;
    do
    {
        //初始化转换器上下文，用作转换输出不同格式的帧数据；
        if(m_swsCtx == nullptr)
        {
            m_orgPixelFormat = frame->format;
            printLog("pixel format: "+ std::to_string(m_orgPixelFormat));
            m_swsCtx = sws_getContext(m_width,
                                      m_height,
                                      (AVPixelFormat)m_orgPixelFormat,
                                      m_width,
                                      m_height,
                                      pixFmt,
                                      SWS_BICUBIC, nullptr, nullptr, nullptr);
            if(m_swsCtx == nullptr)
                break;
        }

        if(m_converFrame == nullptr)
        {
            m_converFrame = av_frame_alloc();
            if(m_converFrame == nullptr)
            {
                printLog("alloc frame fail!");
                break;
            }
        }

        if(m_converBuff == nullptr)
        {
            rtn = av_image_get_buffer_size(pixFmt, m_width, m_height, 1);
            if(rtn< 0)
                break;
            m_converBuff = new unsigned char[rtn];
        }

        //把AVFrame的图像数据绑定到指定的buffer；
        rtn = av_image_fill_arrays(m_converFrame->data, m_converFrame->linesize, m_converBuff, pixFmt, frame->width, frame->height, 1);
        if(rtn< 0)
            break;

        //YUV-->RGB;
        sws_scale(m_swsCtx,
                  (uint8_t const *const*)frame->data,
                  frame->linesize,
                  0,
                  frame->height,
                  m_converFrame->data,
                  m_converFrame->linesize);

    }while(0);

    if(rtn< 0)
    {
        printErrorInfo(rtn);
        return nullptr;
    }
    else
    {
        width = frame->width;
        height = frame->height;
        return m_converBuff;
    }
}

unsigned char *VideoInput::readRawData(PixelFormatType &pixelFormat, int &width, int &height)
{
    auto frame = readAVFrame();
    if(frame == nullptr)
        return nullptr;

    AVPixelFormat pixFmt = (AVPixelFormat)frame->format;
    int rtn = -1;
    do
    {
        if(m_converFrame == nullptr)
        {
            m_converFrame = av_frame_alloc();
            if(m_converFrame == nullptr)
            {
                printLog("alloc frame fail!");
                break;
            }
        }

        if(m_converBuff == nullptr)
        {
            rtn = av_image_get_buffer_size(pixFmt, m_width, m_height, 1);
            if(rtn< 0)
                break;
            m_converBuff = new unsigned char[rtn];
        }

        //把AVFrame的图像数据绑定到指定的buffer；
        rtn = av_image_fill_arrays(m_converFrame->data, m_converFrame->linesize, m_converBuff, pixFmt, frame->width, frame->height, 1);
        if(rtn< 0)
            break;

    }while(0);

    if(rtn< 0)
    {
        printErrorInfo(rtn);
        return nullptr;
    }

    pixelFormat = (PixelFormatType)frame->format;
    width = frame->width;
    height = frame->height;
    return m_converBuff;
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

int VideoInput::getHWDecoderPixelFmt(void *avcodec)
{
    int i = 0;
    while(1)
    {
        auto hwConfig = avcodec_get_hw_config(static_cast<const AVCodec *>(avcodec), i);
        if(hwConfig == nullptr)
        {
            if(m_specHwPixelFormat != AV_PIX_FMT_NONE)
            {
                printLog("can not found spec decoder pixel format :"+ std::to_string(m_specHwPixelFormat));
            }
            return AV_PIX_FMT_NONE;
        }
        printLog("Hardware decoder pixel format" + std::to_string(i) + " :" + std::to_string(hwConfig->pix_fmt));

        //用户未指定硬解码器则返回第一个；
        if(m_specHwPixelFormat == AV_PIX_FMT_NONE)
        {
            gHwDeviceType = hwConfig->device_type;
            return hwConfig->pix_fmt;
        }
        //用户已指定硬解码器时，找到相应的硬解码器则返回；
        else if(m_specHwPixelFormat == hwConfig->pix_fmt)
        {
            gHwDeviceType = hwConfig->device_type;
            return hwConfig->pix_fmt;
        }

        i++;
    }
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

        m_packet->dts = (int64_t)round(m_packet->dts * (1000 * rationalToFloat(m_avformatCtx->streams[m_videoStreamIdx]->time_base)));
        m_packet->pts = (int64_t)round(m_packet->pts * (1000 * rationalToFloat(m_avformatCtx->streams[m_videoStreamIdx]->time_base)));

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

    //硬解码，帧数据存放在硬件存储上（比如GPU的显存）；
    if(gHwPixelFormat != AV_PIX_FMT_NONE)
    {

    }

    return m_frame;
}

void VideoInput::release()
{
    gHwPixelFormat = AV_PIX_FMT_NONE;
    if(m_avformatCtx != nullptr)
        avformat_close_input(&m_avformatCtx);

    if(m_packet != nullptr)
        av_packet_free(&m_packet);

    if(m_frame != nullptr)
        av_frame_free(&m_frame);

    if(m_converFrame != nullptr)
        av_frame_free(&m_converFrame);

    if(m_codecCtx != nullptr)
        avcodec_free_context(&m_codecCtx);

    if(m_converBuff != nullptr)
    {
        delete[] m_converBuff;
        m_converBuff = nullptr;
    }

    if(m_swsCtx != nullptr)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }

    if(m_hwDevContext != nullptr)
    {
        av_buffer_unref(&m_hwDevContext);
        m_hwDevContext = nullptr;
    }
}

