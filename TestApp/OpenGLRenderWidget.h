#ifndef OPENGLRENDERWIDGET_H
#define OPENGLRENDERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <atomic>
#include <condition_variable>
#include <mutex>

class OpenGLRenderWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    explicit OpenGLRenderWidget(QWidget *parent=nullptr);
    virtual ~OpenGLRenderWidget();

    void frameRender(unsigned char *data, int w, int h, int len);

    // QOpenGLWidget interface
protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

private:
    QOpenGLShaderProgram    m_glProgram;
    QOpenGLBuffer           m_glBuff;
    GLuint                  m_txtY;
    GLuint                  m_txtU;
    GLuint                  m_txtV;

    unsigned char*          m_frameData{nullptr};
    int                     m_frameWidth{0};
    int                     m_frameHeight{0};
    std::condition_variable m_renderCv;
    std::mutex              m_renderMtx;
};

#endif // OPENGLRENDERWIDGET_H
