
#include "DeviceInput.h"
#include "libavdevice/avdevice.h"

DeviceInput::DeviceInput()
{
    avdevice_register_all();
}

DeviceInput::~DeviceInput()
{

}

bool DeviceInput::openDevice(std::string deviceName, int width, int height)
{

    m_isOpened = true;
    return true;
}

