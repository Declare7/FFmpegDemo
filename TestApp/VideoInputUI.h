#ifndef VIDEOINPUTUI_H
#define VIDEOINPUTUI_H

#include <QWidget>
#include <string>
#include <mutex>
#include <atomic>
#include <functional>
#include "OpenGLRenderWidget.h"

class VideoInput;

namespace Ui {
class VideoInputUI;
}

class VideoInputUI : public QWidget
{
    Q_OBJECT

public:
    explicit VideoInputUI(QWidget *parent = nullptr);
    ~VideoInputUI();

signals:
    void printMsg(QString msg);

    void renderFrame(QImage img);

    void resizeOpenGLWid(int w, int h);

private slots:
    void on_btnOpen_clicked();

    void onRenderFrame(QImage img);

    void on_btnClose_clicked();

    void on_checkBoxGPU_clicked();

    void onResizeOpenGLWid(int w, int h);

private:
    void printLog(QString log);
    void onLogCallback(const std::string &log);

    static void readFrameThread(VideoInputUI *viPtr);
    void renderFrameGPU(unsigned char* data, int w, int h, int size);

private:
    Ui::VideoInputUI *ui;

    VideoInput *m_viPtr{nullptr};
    std::function<void (const std::string&)>    m_onLogCallbackFunc{nullptr};

    bool            m_isOpened{false};
    std::mutex      m_waitReadCompletedMtx;
    std::mutex      m_waitRenderCompletedMtx;
    std::atomic_bool m_renderDone{false};

    OpenGLRenderWidget* m_openglWid{nullptr};
};

#endif // VIDEOINPUTUI_H
