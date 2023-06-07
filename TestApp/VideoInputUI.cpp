#include "VideoInputUI.h"
#include "ui_VideoInputUI.h"
#include "VideoInput.h"
#include <thread>

VideoInputUI::VideoInputUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoInputUI)
{
    ui->setupUi(this);

    m_onLogCallbackFunc = std::bind(&VideoInputUI::onLogCallback, this, std::placeholders::_1);
    m_viPtr = new VideoInput(m_onLogCallbackFunc);

    connect(this, &VideoInputUI::render, this, &VideoInputUI::onRender, Qt::QueuedConnection);
}

VideoInputUI::~VideoInputUI()
{
    delete m_viPtr;
    m_viPtr = nullptr;
    delete ui;
}

void VideoInputUI::on_btnOpen_clicked()
{
    if(m_viPtr->open("HP True Vision 5MP Camera", "640*480"))
    {
        m_isOpened = true;
        std::thread renderThread(readFrameThread, this);
        renderThread.detach();
    }
    else
    {
        printLog("open fail!");
    }
}

void VideoInputUI::onRender(QImage img)
{
    //因为QImage采用的是浅拷贝，所以在停止出流时要保证最后一帧渲染完成；
    std::lock_guard<std::mutex> lck(m_waitRenderCompletedMtx);
    if(!m_isOpened)
        return;

    ui->labelImg->setPixmap(QPixmap::fromImage(img));
    update();
    m_renderDone.store(true);
}

void VideoInputUI::printLog(QString log)
{
    emit printMsg(log);
}

void VideoInputUI::onLogCallback(const std::string &log)
{
    printMsg(QString::fromStdString(log));
}

void VideoInputUI::readFrameThread(VideoInputUI *viPtr)
{
    std::lock_guard<std::mutex> lck(viPtr->m_waitReadCompletedMtx);

    viPtr->m_renderDone.store(true);
    while(viPtr->m_isOpened)
    {
        //渲染完一帧再取下一帧；因为渲染是在主线程进行，所以需要进行同步处理；
        if(!viPtr->m_renderDone.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        int width = 0;
        int height = 0;
        auto framePtr = viPtr->m_viPtr->readSpecFormatData(width, height, VideoInput::PixelFormatBGRA);
        if(framePtr == nullptr)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        QImage img(framePtr, width, height, QImage::Format_RGB32);
        viPtr->m_renderDone.store(false);
        emit viPtr->render(img);
    }
}

void VideoInputUI::on_btnClose_clicked()
{
    m_isOpened = false;
    std::lock_guard<std::mutex> lck(m_waitReadCompletedMtx);        //保证读线程已经退出；
    std::lock_guard<std::mutex> lck2(m_waitRenderCompletedMtx);     //保证最后一帧已经渲染完成；
    m_viPtr->close();   //close会释放所有的资源，因此前面需要保证所有使用到资源的地方已经结束；

    ui->labelImg->setPixmap(QPixmap());
    update();
}

