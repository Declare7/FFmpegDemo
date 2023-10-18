varying vec2 textureOut;
uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;

void main(void)
{
    vec3 yuv;
    vec3 rgb;

    highp float y = texture2D(textureY, textureOut).r;
    highp float u = texture2D(textureU, textureOut).r - 0.5;
    highp float v = texture2D(textureV, textureOut).r - 0.5;

    highp float r = y + 1.13983 * v;
    highp float g = y - 0.39465 * u - 0.58060 * v;
    highp float b = y + 2.03211 * u;

    gl_FragColor = vec4(r, g, b, 1.0);
}
