#ifndef OPENGLRENDERWIDGET_H
#define OPENGLRENDERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <atomic>

class OpenGLRenderWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    explicit OpenGLRenderWidget(QWidget *parent=nullptr);
    virtual ~OpenGLRenderWidget();

    void renderFrame(unsigned char *data, int w, int h, int len);

    void reset();

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
    int                     m_frameDataSize{0};
    int                     m_frameWidth{0};
    int                     m_frameHeight{0};

    std::atomic_bool        m_renderDone{true};
};

#endif // OPENGLRENDERWIDGET_H
