precision mediump float;
varying vec2 v_texPosition;  // 从顶点程序传过来的值
uniform sampler2D sampler_y;
uniform sampler2D sampler_u;
uniform sampler2D sampler_v;

void main() {
    float y,u,v;

    // 返回值是rgb
    y = texture2D(sampler_y,v_texPosition).r;
    u = texture2D(sampler_u,v_texPosition).r;
    v = texture2D(sampler_v,v_texPosition).r;

    vec3 rgb;
    // yuv转为rgb的固定代码
    rgb.r = y + 1.403 * (v-0.5);
    rgb.g = y - 0.344 * (u-0.5) - 0.714 * (v-0.5);
    rgb.b = y + 1.770 * (u-0.5);

    gl_FragColor = vec4(rgb,1);
}
