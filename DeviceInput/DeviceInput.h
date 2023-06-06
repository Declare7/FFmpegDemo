/** @brief 使用ffmpeg获取指定设备的流，并提供解码后的帧数据；
        *  @author: Fly Li
        *  @note:
        *  @since: 2023-05-29
        */
#ifndef DEVICEINPUT_H
#define DEVICEINPUT_H

#include "DeviceInput_global.h"
#include <string>


class DEVICEINPUT_EXPORT DeviceInput
{
public:
    explicit DeviceInput();
    ~DeviceInput();

    bool openDevice(std::string deviceName, int width, int height);

private:
    bool m_isOpened{false};
};


#endif // DEVICEINPUT_H
