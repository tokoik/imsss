#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

#define REFLECTIVE_OCCLUSION 0

//
// pass2.frag
//

const float PI = 3.141593;                                            // 円周率
const int MAXSAMPLES = 256;                                           // サンプル点の最大数
const float kshi = 60.0;                                              // 輝き係数

uniform int samples;                                                  // サンプル点数
uniform float radius;                                                 // サンプル点の散布半径
uniform float translucency;                                           // 半透明度
uniform vec4 ambient;                                                 // 環境光強度
uniform vec4 mapping;                                                 // テクスチャ座標のスケールとオフセット
uniform sampler2D unit[6];                                            // テクスチャユニット
uniform vec4 point[MAXSAMPLES];                                       // サンプル点の位置

// 変換行列
uniform mat4 mp;                                                      // 投影変換行列
uniform mat4 mr;                                                      // 放物面マッピング用
uniform mat4 mt;                                                      // テクスチャの回転

// ラスタライザから受け取る頂点属性の補間値
in vec2 tc;                                                           // テクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                                    // フラグメントの色

void main(void)
{
  // 反射マップのテスト用
  //fc = texture(unit[5], tc * mapping.xy + mapping.zw);
  //return;

  // unit[0] から albedo を取得
  vec4 albedo = texture(unit[0], tc);

  // albedo のアルファ値が 0 なら背景色
  if (albedo.a == 0.0)
  {
    fc = vec4(0.0);
    return;
  }

  // unit[1] から fresnel を取得
  vec4 fresnel = texture(unit[1], tc);

  // unit[2] から処理対象の画素の視点座標系上の位置を取得
  vec4 position = texture(unit[2], tc);

  // unit[3] から処理対象の画素の視点座標系上の法線ベクトルを取得 (w = 0)
  vec3 normal = texture(unit[3], tc).xyz;

  // 視線ベクトル
  vec3 view = normalize(position.xyz);

  // 反射ベクトル
  vec3 reflection = (vec4(reflect(view, normal), 0.0) * mt).xyz;

  // normal 方向の放射照度
  vec4 diffuse = vec4(0.0);

  // reflection 方向の放射照度
#if REFLECTIVE_OCCLUSION
  vec4 specular = vec4(0.0);
#else
  vec4 specular = mix(ambient, texture(unit[5], (reflection.xz * 0.5 / (1.0 + reflection.y) + 0.5) * mapping.xy + mapping.zw), step(0.0, reflection.y));
#endif

  // 遮蔽されないポイントの数
  float count = 0.0;

  // 個々のサンプル点について
  for (int i = 0; i < samples; ++i)
  {
    // サンプル点の相対位置
    vec4 offset = vec4(radius * point[i].w * point[i].xyz, 0.0);

    // サンプル点の位置を p からの相対位置に平行移動した後，その点のクリッピング座標系上の位置 q を求める
    vec4 q = mp * (position + offset);

    // テクスチャ座標に変換する
    q = q * 0.5 / q.w + 0.5;

    // q の深度が unit[4] の値 (デプスバッファの値) より小さければ可視係数 visibility = 0 大きければ visibility = 1
    float visibility = step(q.z, texture(unit[4], q.xy).x);

    // 可視係数の総和を求める
    count += visibility;

    //if (visibility == 0.0) continue;  // これを有効にすると 15% くらい遅くなる

    // 天空マップのサンプリング方向を求める
    vec4 v = mt * point[i];

    // 放物面マップから天空の放射輝度を得る (v.y < 0 なら放物面マップの外側だから背景色)
    vec4 radiance = mix(ambient, texture(unit[5], (v.xz * 0.5 / (1.0 + v.y) + 0.5) * mapping.xy + mapping.zw), step(0.0, v.y)) * visibility;
  
    // normal 方向の放射照度の重みづけ和を求める
    diffuse += max(dot(v.xyz, normal), 0.0) * radiance;

#if REFLECTIVE_OCCLUSION
    // reflection 方向の放射照度の重みづけ和を求める
    specular += pow(max(dot(v.xyz, reflection), 0.0), kshi) * radiance;
#endif
  }

  // normal 方向の放射照度を正規化する
  diffuse *= 4.0 / float(samples);

#if REFLECTIVE_OCCLUSION
  // reflection 方向の放射照度を正規化する
  specular *= (kshi + 8.0) / float(samples);
#endif

  // 伝達度
  //float transmission = count / samples;
  //float transmission = 1.0 - sin(sqrt(2.0 * (samples - count) / samples) * PI * 0.5);
  float transmission = 2.0 * exp(-sin(sqrt(2.0 * (samples - count) / samples) * PI * 0.5));

  // アルベド
  //fc = albedo;

  // フレネル
  //fc = fresnel;

  // 放射照度
  //fc = diffuse;

  // 映り込み
  //fc = specular;

  // 位置
  //fc = position * 0.5 + 0.5;

  // 法線
  //fc = vec4(normal * 0.5 + 0.5, 1.0);

  // 深度
  //fc = vec4(texture(unit[4], tc).r);

  // 伝達度
  //fc = vec4(transmission);

  // 天空からの放射照度による自己遮蔽を考慮した陰影
  //fc = albedo * diffuse;

  // 陰影に伝達度を積算
  fc = albedo * mix(diffuse, ambient * transmission, translucency);

  // アルベド×放射照度と映り込みを比例配分
  fc = mix(fc, specular, fresnel);
}
