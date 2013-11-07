#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

//
// pass1.frag
//

// 材質
uniform vec4 kamb;                                                    // 環境光の反射係数
uniform vec4 kdiff;                                                   // 拡散反射係数
uniform vec4 kspec;                                                   // 鏡面反射係数
uniform float kshi;                                                   // 輝き係数

// ラスタライザから受け取る頂点属性の補間値
in vec4 p;                                                            // 位置 P
in vec3 n;                                                            // 法線 N

// フレームバッファに出力するデータ
layout (location = 0) out vec4 albedo;
layout (location = 1) out vec4 fresnel;
layout (location = 2) out vec3 position;
layout (location = 3) out vec3 normal;

void main(void)
{
  albedo = kdiff;
  fresnel = kspec;
  position = p.xyz / p.w;
  normal = normalize(n);
}
