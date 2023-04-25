attribute vec4 av_Position;  // 世界坐标
attribute vec2 af_Position;  // 纹理坐标
varying vec2 v_texPosition;  // 声明传过去的值   数据流动

void main() {
    v_texPosition = af_Position;
    gl_Position = av_Position;
}
