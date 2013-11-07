#ifndef __GG_OBJ_H__
#define __GG_OBJ_H__

#include "GgObjShader.h"

namespace gg
{
  //
  // Alias OBJ 形式のファイル
  //
  class GgObj
    : public GgShape
  {
    GLuint ng, (*group)[2];
    GLfloat (*amb)[4], (*diff)[4], (*spec)[4], *shi;
    GgObjShader *shader;

  public:

    // デストラクタ
    virtual ~GgObj(void);

    // コンストラクタ
    GgObj(const char *name, bool normalize = false);

    // シェーダの割り当て
    void attachShader(GgObjShader &shader)
    {
      this->shader = &shader;
    }
    void attachShader(GgObjShader *shader)
    {
      this->shader = shader;
    }

    // 図形の描画
    void draw(void) const;
  };

}

#endif
