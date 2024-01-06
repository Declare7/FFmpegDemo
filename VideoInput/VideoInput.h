/** @brief 使用ffmpeg获取指定地址的流，并提供解码后的帧数据；
        *  @author: Fly Li
        *  @note: 流可以来自文件，网络，也可以来自设备，对于ffmpeg来说统一为输入源；
        *  @since: 2023-05-29
        */
#ifndef VIDEOINPUT_H
#define VIDEOINPUT_H

#include "VideoInput_global.h"
#include <string>
#include <functional>
#include <mutex>

#define LOG_BUFF_SIZE 1024

struct AVFormatContext;
struct AVInputFormat;
struct AVRational;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct SwsContext;
struct AVBufferRef;

class VIDEOINPUT_EXPORT VideoInput
{
public:
    //参考FFmpeg的pixfmt.h下的AVPixelFormat；
    enum PixelFormatType
    {
        PixelFormatYUVJ422P = 13,
        PixelFormatBGRA = 28,
    };

public:
    explicit VideoInput(std::function<void (const std::string&)> logCallback=nullptr);
    ~VideoInput();

    /**
    * @brief 打开视频流；
    * @param [in] url: 视频流地址；inputFormat: 输入格式；videoSize: 视频流分辨率；
    * @param
    * @return 成功返回true；失败返回false；
    */
    bool open(const std::string url, std::string inputFormat="", std::string videoSize="");

    /**
    * @brief 关闭视频流，并释放相关资源；
    * @param
    * @param
    * @return
    */
    void close();

    /**  读取指定像素格式的帧数据；
    *  @param[in]   pixelFormat: 指定像素格式；
    *  @param[out]  width: 返回帧的宽；height: 返回帧的高；
    *  @return  帧数据的指针，如果失败则返回nullptr；
    *  @note    调用方不能对return 的指针进行释放；
    */
    unsigned char *readSpecFormatData(const PixelFormatType &pixelFormat, int &width, int &height);

    /**  读取原始像素格式的帧数据；
    *  @param[in]
    *  @param[out]  width: 返回帧的宽；height: 返回帧的高；pixelFormat: 返回原始的像素格式；
    *  @return  帧数据的指针，如果失败则返回nullptr；
    *  @note    调用方不能对return 的指针进行释放；
    */
    unsigned char *readRawData(PixelFormatType &pixelFormat, int &width, int &height);

private:
    void printErrorInfo(int errorCode);
    void printLog(const std::string &log);

    int getHWDecoderPixelFmt(void *avcodec);
    float rationalToFloat(const AVRational &rational);
    AVFrame *readAVFrame();
    void release();

private:
    std::function<void (const std::string&)>   m_logCallBack{nullptr};
    char m_logBuff[LOG_BUFF_SIZE];

    AVFormatContext         *m_avformatCtx{nullptr};
    const AVInputFormat     *m_inputFormat{nullptr};
    AVCodecContext          *m_codecCtx{nullptr};
    AVPacket                *m_packet{nullptr};
    AVFrame                 *m_frame{nullptr};
    AVFrame                 *m_converFrame{nullptr};
    SwsContext              *m_swsCtx{nullptr};

    int                     m_videoStreamIdx{0};
    int                     m_width{0};
    int                     m_height{0};
    float                   m_fps{0};
    unsigned char           *m_converBuff{nullptr};
    int                     m_orgPixelFormat{-1};  //参考pixfmt.h的AVPixelFormat;
    unsigned char           *m_orgBuff{nullptr};

    int         m_specHwPixelFormat{-1};//用户指定硬解码器格式；
    AVBufferRef *m_hwDevContext{nullptr};

    std::mutex              m_closeMtx;
};


#endif // VIDEOINPUT_H
