#ifndef VIDEOINPUTUI_H
#define VIDEOINPUTUI_H

#include <QWidget>
#include <string>
#include <mutex>
#include <atomic>
#include <functional>

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

    void render(QImage img);

private slots:
    void on_btnOpen_clicked();

    void onRender(QImage img);

    void on_btnClose_clicked();

private:
    void printLog(QString log);
    void onLogCallback(const std::string &log);

    static void readFrameThread(VideoInputUI *viPtr);

private:
    Ui::VideoInputUI *ui;

    VideoInput *m_viPtr{nullptr};
    std::function<void (const std::string&)>    m_onLogCallbackFunc{nullptr};

    bool            m_isOpened{false};
    std::mutex      m_waitReadCompletedMtx;
    std::mutex      m_waitRenderCompletedMtx;
    std::atomic_bool m_renderDone{false};
};

#endif // VIDEOINPUTUI_H
