/*
** 点を球として表示する
*/
#include "GgPointShader.h"

// コンストラクタ
gg::GgPointShader::GgPointShader(const char *vert, const char *frag,
  const char *geom, GLint nvarying, const char **varyings)
  : GgShader(vert, frag, geom, nvarying, varyings)
{
  // プログラム名
  GLuint program = get();

  // 変換行列の uniform 変数の場所
  loc.mc = glGetUniformLocation(program, "mc");
  loc.mw = glGetUniformLocation(program, "mw");
}

// シェーダの使用開始
void gg::GgPointShader::use(void) const
{
  // 基底クラスのシェーダの設定を呼び出す
  GgShader::use();

  // 変換
  glUniformMatrix4fv(loc.mc, 1, GL_FALSE, m.c);
  glUniformMatrix4fv(loc.mw, 1, GL_FALSE, m.w);
}

// シェーダの使用終了
void gg::GgPointShader::unuse(void) const
{
  // 基底クラスのシェーダの設定を呼び出す
  GgShader::unuse();
}

// 変換行列の設定
void gg::GgPointShader::loadMatrix(const GgMatrix &mp, const GgMatrix &mw)
{
  m.loadModelViewProjectionMatrix(mp * mw);
  m.loadModelViewMatrix(mw);
}
void gg::GgPointShader::loadMatrix(const GLfloat *mp, const GLfloat *mw)
{
  GgMatrix tmp(mp), tmw(mw);
  loadMatrix(tmp, tmw);
}
