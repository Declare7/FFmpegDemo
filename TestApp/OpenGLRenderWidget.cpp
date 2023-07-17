#include "OpenGLRenderWidget.h"

OpenGLRenderWidget::OpenGLRenderWidget(QWidget *parent) : QOpenGLWidget(parent)
{

}

OpenGLRenderWidget::~OpenGLRenderWidget()
{

}

void OpenGLRenderWidget::frameRender(unsigned char *data, int w, int h, int len)
{
    if(data == nullptr)
        return;

    if(w == m_frameWidth || h == m_frameHeight)
        return;

    std::unique_lock<std::mutex> lck(m_renderMtx);
    m_frameData = data;
    m_frameWidth = w;
    m_frameHeight = h;
    update();

    //等待渲染完成；
    while(m_renderCv.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout)
    {
        qDebug()<< "error: render timeout!!!";
    }
}

void OpenGLRenderWidget::initializeGL()
{
    initializeOpenGLFunctions();

    m_glProgram.addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, "VideoRender.frag");
    m_glProgram.addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, "VideoRender.vert");
    m_glProgram.link();

    GLfloat points[]{
        -1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, -1.0f,
        -1.0f, -1.0f,

        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
        0.0f,1.0f
    };

    m_glBuff.create();
    m_glBuff.bind();
    m_glBuff.allocate(points, sizeof(points));

    GLuint textures[3];
    glGenTextures(3, textures);
    m_txtY = textures[0];
    m_txtU = textures[1];
    m_txtV = textures[2];

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderWidget::resizeGL(int w, int h)
{
    QOpenGLWidget::resizeGL(w, h);
}

void OpenGLRenderWidget::paintGL()
{
    do
    {
        if(m_frameData == nullptr)
            break;

        //由YUV420p的数据结构决定；
        unsigned char* dataY = m_frameData;
        unsigned char* dataU = dataY + m_frameWidth* m_frameHeight;
        unsigned char* dataV = dataU + m_frameWidth* m_frameHeight/ 4;

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        m_glProgram.bind();
        m_glBuff.bind();
        m_glProgram.enableAttributeArray("vertexIn");
        m_glProgram.enableAttributeArray("textureOut");
        m_glProgram.setAttributeBuffer("vertexIn", GL_FLOAT, 0, 2, 2* sizeof(GLfloat));
        m_glProgram.setAttributeBuffer("textureOut", GL_FLOAT, 2* 4* sizeof(GLfloat), 2, 2* sizeof(GLfloat));

        glActiveTexture(GL_TEXTURE0+ 2);
        glBindTexture(GL_TEXTURE_2D, m_txtY);\
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_frameWidth, m_frameHeight, 0, GL_RED, GL_UNSIGNED_BYTE, dataY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glActiveTexture(GL_TEXTURE0+ 0);
        glBindTexture(GL_TEXTURE_2D, m_txtU);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, m_frameWidth>> 1, m_frameHeight>> 1, 0, GL_RG, GL_UNSIGNED_BYTE, dataU);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glActiveTexture(GL_TEXTURE0+ 0);
        glBindTexture(GL_TEXTURE_2D, m_txtV);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, m_frameWidth>> 1, m_frameHeight>> 1, 0, GL_RG, GL_UNSIGNED_BYTE, dataV);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        m_glProgram.setUniformValue("textureUV", 0);
        m_glProgram.setUniformValue("textureY", 0);
        glDrawArrays(GL_QUADS, 0, 4);
        m_glProgram.disableAttributeArray("vertexIn");
        m_glProgram.disableAttributeArray("textureIn");
        m_glProgram.release();

    }while(0);

    m_renderCv.notify_one();

}
