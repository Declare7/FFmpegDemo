varying vec2 textureCoordinate;
uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;

void main(void)
{
    highp float y = texture2D(textureY, textureCoordinate).r;
    highp float u = texture2D(textureU, textureCoordinate).r - 0.5;
    highp float v = texture2D(textureV, textureCoordinate).r - 0.5;

    highp float r = y + 1.370705 * v;
    highp float g = y - 0.337633 * u - 0.698001 * v;
    highp float b = y + 1.732446 * u;

    gl_FragColor = vec4(r, g, b, 1.0);
}
