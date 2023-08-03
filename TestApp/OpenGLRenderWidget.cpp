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

    if(!m_renderDone.load())
        return;

    if(w != m_frameWidth || h != m_frameHeight || len != m_frameDataSize)
    {
        if(m_frameData != nullptr)
            delete[] m_frameData;

        m_frameWidth = w;
        m_frameHeight = h;
        m_frameDataSize = len;
        m_frameData = new unsigned char[len];
    }

    memcpy(m_frameData, data, len);
    m_renderDone.store(false);
    update();
}

void OpenGLRenderWidget::initializeGL()
{
    initializeOpenGLFunctions();

    //装载OpenGL 着色器代码；
    m_glProgram.addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/VideoRender.frag");
    m_glProgram.addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/VideoRender.vert");
    m_glProgram.link();
    m_glProgram.bind();

    //创建顶点坐标和纹理坐标；
    GLfloat points[]{
        -1.0f, 1.0f,    //左上角
        1.0f, 1.0f,     //右上角
        1.0f, -1.0f,    //右下角
        -1.0f, -1.0f,   //左下角

        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
        0.0f,1.0f
    };

    //把顶点坐标和纹理坐标注入到OpenGL中；
    m_glBuff.create();
    m_glBuff.bind();
    m_glBuff.allocate(points, sizeof(points));

    //创建Y,U,V纹理；
    glGenTextures(1, &m_txtY);
    glGenTextures(1, &m_txtU);
    glGenTextures(1, &m_txtV);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderWidget::resizeGL(int w, int h)
{
    QOpenGLWidget::resizeGL(w, h);
}

void OpenGLRenderWidget::paintGL()
{
    if(m_frameData == nullptr)
        return;

    //由YUV420p的数据结构决定；
    //YUV420p byte stream: /Y--------width*height----------/U---(width/2)*(heigh/2)---/V---(width/2)*(heigh/2)---/
    unsigned char* dataY = m_frameData;
    unsigned char* dataU = dataY + m_frameWidth* m_frameHeight;
    unsigned char* dataV = dataU + (m_frameWidth/2)* (m_frameHeight/2);

//    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glDisable(GL_DEPTH_TEST);

    m_glProgram.bind();
    m_glBuff.bind();
    m_glProgram.enableAttributeArray("vertexIn");
    m_glProgram.enableAttributeArray("textureIn");
    m_glProgram.setAttributeBuffer("vertexIn", GL_FLOAT, 0, 2, sizeof(GLfloat) * 2);
    m_glProgram.setAttributeBuffer("textureIn", GL_FLOAT, sizeof(GLfloat) * 2 * 4, 2, sizeof(GLfloat) * 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_txtY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_frameWidth, m_frameHeight, 0, GL_RED, GL_UNSIGNED_BYTE, dataY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_txtU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_frameWidth>> 1, m_frameHeight>> 1, 0, GL_RED, GL_UNSIGNED_BYTE, dataU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_txtV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_frameWidth>> 1, m_frameHeight>> 1, 0, GL_RED, GL_UNSIGNED_BYTE, dataV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_glProgram.setUniformValue("textureY", 0); //GL_TEXTURE0
    m_glProgram.setUniformValue("textureU", 1); //GL_TEXTURE1
    m_glProgram.setUniformValue("textureV", 2); //GL_TEXTURE2

    //绘制，从顶点0开始，绘制四个顶点；
    glDrawArrays(GL_QUADS, 0, 4);

    m_glProgram.disableAttributeArray("vertexIn");
    m_glProgram.disableAttributeArray("textureIn");
    m_glProgram.release();

    m_renderDone.store(true);
}
