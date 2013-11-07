/*
** Alias OBJ 形式の形状データの陰影付け
*/
#include "GgObjShader.h"

// コンストラクタ
gg::GgObjShader::GgObjShader(const char *vert, const char *frag,
  const char *geom, GLint nvarying, const char **varyings)
  : GgPointShader(vert, frag, geom, nvarying, varyings)
{
  // プログラム名
  GLuint program = get();

  // 光源のパラメータの uniform 変数の場所
  loc.pl = glGetUniformLocation(program, "pl");
  loc.lamb = glGetUniformLocation(program, "lamb");
  loc.ldiff = glGetUniformLocation(program, "ldiff");
  loc.lspec = glGetUniformLocation(program, "lspec");

  // 材質のパラメータの uniform 変数の場所
  loc.kamb = glGetUniformLocation(program, "kamb");
  loc.kdiff = glGetUniformLocation(program, "kdiff");
  loc.kspec = glGetUniformLocation(program, "kspec");
  loc.kshi = glGetUniformLocation(program, "kshi");

  // 変換行列の uniform 変数の場所
  loc.mg = glGetUniformLocation(program, "mg");
}

// シェーダの使用開始
void gg::GgObjShader::use(void) const
{
  // 基底クラスのシェーダの設定を呼び出す
  GgPointShader::use();

  // 光源
  glUniform4fv(loc.pl, 1, l.pos);
  glUniform4fv(loc.lamb, 1, l.amb);
  glUniform4fv(loc.ldiff, 1, l.diff);
  glUniform4fv(loc.lspec, 1, l.spec);

  // 変換
  glUniformMatrix4fv(loc.mg, 1, GL_FALSE, m.g);
}

// シェーダの使用終了
void gg::GgObjShader::unuse(void) const
{
  // attribute 変数 pv をバッファオブジェクトから得ることを無効にする
  glDisableVertexAttribArray(1);

  // 基底クラスのシェーダの設定を呼び出す
  GgShader::unuse();
}

// 変換行列の設定
void gg::GgObjShader::loadMatrix(const GgMatrix &mp, const GgMatrix &mw)
{
  GgPointShader::loadMatrix(mp, mw);
  m.loadNormalMatrix(mw.normal());
}
void gg::GgObjShader::loadMatrix(const GLfloat *mp, const GLfloat *mw)
{
  GgMatrix tmp(mp), tmw(mw);
  loadMatrix(tmp, tmw);
}

// 環境光に対する反射係数
void gg::GgObjShader::setMaterial(const GLfloat *amb, const GLfloat *diff, const GLfloat *spec, GLfloat shi)
{
  glUniform4fv(loc.kamb, 1, amb);
  glUniform4fv(loc.kdiff, 1, diff);
  glUniform4fv(loc.kspec, 1, spec);
  glUniform1f(loc.kshi, shi);
}

