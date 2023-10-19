attribute vec4 vertexIn;
attribute vec2 textureIn;
varying vec2 textureCoordinate;

void main(void)
{
    gl_Position = vertexIn;
    textureCoordinate = textureIn;
}
