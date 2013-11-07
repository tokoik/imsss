#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

//
// pass1.vert
//

// 変換行列
uniform mat4 mw;                                                      // 視点座標系への変換行列
uniform mat4 mc;                                                      // クリッピング座標系への変換行列
uniform mat4 mg;                                                      // 法線ベクトルの変換行列

// 頂点属性
layout (location = 0) in vec4 pv;                                     // ローカル座標系の頂点位置
layout (location = 1) in vec4 nv;                                     // 頂点の法線ベクトル

// ラスタライザに送る頂点属性
out vec4 p;                                                           // 物体表面上の位置 P
out vec3 n;                                                           // 物体表面上の法線ベクトル N

void main(void)
{
  p = mw * pv;                                                        // フラグメントの位置
  n = normalize((mg * nv).xyz);                                       // フラグメントの法線ベクトル N

  gl_Position = mc * pv;                                              // 頂点位置
}
