varying vec2 textureOut;
uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;

void main(void)
{
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture2D(textureY, textureOut).r;
    yuv.y = texture2D(textureU, textureOut).r - 0.5;
    yuv.z = texture2D(textureV, textureOut).r - 0.5;

    rgb = mat3(1.0, 1.0, 1.0,
               0.0, -0.39465, 2.03211,
               1.13983, -0.58060, 0.0) * yuv;

    gl_FragColor = vec4(rgb, 1.0);
}
