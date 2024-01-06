#include "VideoInputUI.h"
#include "ui_VideoInputUI.h"
#include "VideoInput.h"
#include <thread>
#include <QCameraInfo>

VideoInputUI::VideoInputUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoInputUI)
{
    ui->setupUi(this);
    auto openglWidLayout = new QHBoxLayout();
    openglWidLayout->setContentsMargins(0, 0, 0, 0);
    openglWidLayout->setSpacing(0);
    ui->labelImg->setLayout(openglWidLayout);

    m_openglWid = new OpenGLRenderWidget(ui->labelImg);
    openglWidLayout->addWidget(m_openglWid);
    on_checkBoxGPU_clicked();

    m_onLogCallbackFunc = std::bind(&VideoInputUI::onLogCallback, this, std::placeholders::_1);
    m_viPtr = new VideoInput(m_onLogCallbackFunc);

    connect(this, &VideoInputUI::renderFrame, this, &VideoInputUI::onRenderFrame, Qt::QueuedConnection);
    connect(this, &VideoInputUI::resizeOpenGLWid, this, &VideoInputUI::onResizeOpenGLWid, Qt::QueuedConnection);

    auto cameras = QCameraInfo::availableCameras();
    for(int i = 0; i< cameras.size(); ++i)
    {
        ui->comboBoxInput->addItem(cameras.at(i).description());
    }
    ui->comboBoxInput->addItem("C:/Users/15306/Desktop/ffmpeg/2023-07-21_18-14-54.mp4");
}

VideoInputUI::~VideoInputUI()
{
    delete m_viPtr;
    m_viPtr = nullptr;
    delete ui;
}

void VideoInputUI::on_btnOpen_clicked()
{
    if(m_viPtr->open(ui->comboBoxInput->currentText().toStdString(),
                     ui->comboBoxFormat->currentText().toStdString(),
                     ui->comboBoxResolution->currentText().toStdString()))
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

void VideoInputUI::onRenderFrame(QImage img)
{
    //因为QImage采用的是浅拷贝，所以在停止出流时要保证最后一帧渲染完成；
    std::lock_guard<std::mutex> lck(m_waitRenderCompletedMtx);
    if(!m_isOpened)
        return;

    ui->labelImg->setPixmap(QPixmap::fromImage(img.scaled(ui->labelImg->width(), ui->labelImg->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
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
        unsigned char* framePtr = nullptr;
        VideoInput::PixelFormatType format = VideoInput::PixelFormatYUVJ422P;
        if(viPtr->ui->checkBoxGPU->isChecked())
            framePtr = viPtr->m_viPtr->readSpecFormatData(format, width, height);
        else
            framePtr = viPtr->m_viPtr->readSpecFormatData(VideoInput::PixelFormatBGRA, width, height);

        if(framePtr == nullptr)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if(viPtr->ui->checkBoxGPU->isChecked())
        {
            int size = 0;
            if(format == VideoInput::PixelFormatYUVJ422P)
                size = width * height * 2;
            viPtr->renderFrameGPU(framePtr, width, height, size);
        }
        else
        {
            QImage img(framePtr, width, height, QImage::Format_RGB32);
            viPtr->m_renderDone.store(false);
            emit viPtr->renderFrame(img);
        }
    }
}

void VideoInputUI::renderFrameGPU(unsigned char *data, int w, int h, int size)
{
    emit resizeOpenGLWid(ui->labelImg->width(), ui->labelImg->width()* ((float)h/w));
    m_openglWid->renderFrame(data, w, h, size);
}

void VideoInputUI::on_btnClose_clicked()
{
    m_isOpened = false;
    std::lock_guard<std::mutex> lck(m_waitReadCompletedMtx);        //保证读线程已经退出；
    std::lock_guard<std::mutex> lck2(m_waitRenderCompletedMtx);     //保证最后一帧已经渲染完成；
    m_viPtr->close();   //close会释放所有的资源，因此前面需要保证所有使用到资源的地方已经结束；

    if(ui->checkBoxGPU->isChecked())
    {
        m_openglWid->reset();
    }
    else
    {
        ui->labelImg->setPixmap(QPixmap());
        update();
    }
}

void VideoInputUI::on_checkBoxGPU_clicked()
{
    m_openglWid->setHidden(!ui->checkBoxGPU->isChecked());
}

void VideoInputUI::onResizeOpenGLWid(int w, int h)
{
    m_openglWid->setFixedHeight(h);
}

