#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

//
// pass2.vert
//

// 頂点属性
layout (location = 0) in vec4 pv;                                     // ローカル座標系の頂点位置

// ラスタライザに送る頂点属性
out vec2 tc;                                                          // テクスチャ座標

void main(void)
{
  tc = pv.xy * 0.5 + 0.5;
  gl_Position = pv;
}
