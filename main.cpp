#include <cmath>
#include <cstdlib>
#include <iostream>

// 時刻の計測に glfwGetTime() も使うなら 1
#define USE_GLFWGETTIME 0

#include <opencv2/highgui/highgui.hpp>

#ifdef _WIN32
#  include <Windows.h>
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  ifdef _DEBUG
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_core" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_highgui" CV_VERSION_STR CV_EXT_STR)
#else
#  include <pthread.h>
#  include <unistd.h>
#endif

// 補助プログラム
#include "gg.h"
using namespace gg;

// Alias OBJ 形式の形状データ
#include "GgObj.h"

// Alias OBJ 形式の形状データ用シェーダ
#include "GgObjShader.h"

// 形状データ
//static const char objfile[] = "box.obj";
//static const char objfile[] = "ball.obj";
static const char objfile[] = "bunny.obj";
//static const char objfile[] = "dragon2.obj";
//static const char objfile[] = "budda.obj";
//static const char objfile[] = "HondaS2000.obj";
//static const char objfile[] = "AC 1038.obj";

// キャプチャする画像サイズとフレームレート
#define CAPTURE_WIDTH 640
#define CAPTURE_HEIGHT 480
#define CAPTURE_FPS 30

// テクスチャ座標の変換係数
#define TEXTURE_WIDTH 1024
#define TEXTURE_HEIGHT 512
static GLuint texname;
static GLfloat mapping[] =
{
  static_cast<GLfloat>(CAPTURE_HEIGHT) / static_cast<GLfloat>(TEXTURE_WIDTH),
  -static_cast<GLfloat>(CAPTURE_HEIGHT) / static_cast<GLfloat>(TEXTURE_HEIGHT),
  static_cast<GLfloat>(CAPTURE_WIDTH - CAPTURE_HEIGHT) * 0.5 / static_cast<GLfloat>(TEXTURE_WIDTH),
  static_cast<GLfloat>(CAPTURE_HEIGHT) / static_cast<GLfloat>(TEXTURE_HEIGHT)
};

// フレームバッファオブジェクトのサイズ
#define FBOWIDTH 1024
#define FBOHEIGHT 1024

// 深度のサンプリングに使う点群数
#define MAXSAMPLES 256

// サンプル点の数
static GLint samples = 64;

// サンプル点の散布半径
static GLint radius = 20;

// 半透明度
static GLint translucency = 50;

// 環境光強度
static GLfloat ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };

// ウィンドウサイズ
static int width = 800, height = 800;

// 透視投影変換行列
static GgMatrix mp;

// トラックボール
static GgTrackball tbl, tbr;

// ベンチマーク
static bool benchmark = false;

//
// ウィンドウのサイズ変更時の処理
//
static void GLFWCALL resize(int w, int h)
{
  // ウィンドウサイズを保存する
  width = w;
  height = h;

  // ウィンドウ全体をビューポートにする
  glViewport(0, 0, w, h);

  // 透視投影変換行列を求める（アスペクト比 w / h）
  mp.loadPerspective(0.6f, (GLfloat)w / (GLfloat)h, 2.5f, 5.5f);
  //mp.loadPerspective(0.6f, (GLfloat)w / (GLfloat)h, 2.5f, 5.0f);

  // トラックボール処理の範囲を設定する
  tbl.region(w, h);
  tbr.region(w, h);
}

//
// マウスボタン操作時の処理
//
static void GLFWCALL mouse(int button, int action)
{
  // マウスの現在位置を取得する
  int x, y;
  glfwGetMousePos(&x, &y);

  switch (button)
  {
  case GLFW_MOUSE_BUTTON_LEFT:
    if (action != GLFW_RELEASE)
    {
      // 左ボタン押下
      tbl.start(x, y);
    }
    else
    {
      // 左ボタン開放
      tbl.stop(x, y);
    }
    break;
  case GLFW_MOUSE_BUTTON_MIDDLE:
    break;
  case GLFW_MOUSE_BUTTON_RIGHT:
    if (action != GLFW_RELEASE)
    {
      // 右ボタン押下
      tbr.start(x, y);
    }
    else
    {
      // 右ボタン開放
      tbr.stop(x, y);
    }
    break;
  default:
    break;
  }
}

//
// キーボード
//
static void GLFWCALL keyboard(int key, int action)
{
  if (action == GLFW_PRESS)
  {
    switch (key)
    {
    case GLFW_KEY_SPACE:
      if (samples < MAXSAMPLES) ++samples;
      std::cout << "SAMPLES= " << samples << std::endl;
      break;
    case GLFW_KEY_BACKSPACE:
    case GLFW_KEY_DEL:
      if (samples > 0) --samples;
      std::cout << "SAMPLES= " << samples << std::endl;
      break;
    case GLFW_KEY_UP:
      std::cout << "TRANSLUCENCY= " << ++translucency * 0.01f << std::endl;
      break;
    case GLFW_KEY_DOWN:
      std::cout << "TRANSLUCENCY= " << --translucency * 0.01f << std::endl;
      break;
    case GLFW_KEY_RIGHT:
      std::cout << "RADIUS= " << ++radius * 0.01f << std::endl;
      break;
    case GLFW_KEY_LEFT:
      std::cout << "RADIUS= " << --radius * 0.01f << std::endl;
      break;
    case 'b':
    case 'B':
      benchmark = true;
      break;
    case GLFW_KEY_ESC:
    case 'Q':
    case 'q':
      exit(0);
    default:
      break;
    }
  }
}

// キャプチャ用スレッド
class CaptureWorker
{
  // キャプチャ
  CvCapture *capture;

  // テクスチャ
  GLenum format;
  GLsizei width, height;
  GLubyte *texture;

  // 実行状態
  bool status;

  // スレッドとミューテックス
#ifdef _WIN32
  HANDLE thread;
  HANDLE mutex;
#else
  pthread_t thread;
  pthread_mutex_t mutex;
#endif

public:

  // コンストラクタ
  CaptureWorker(int index, int width, int height, int fps)
  {
    // カメラを初期化する
    capture = cvCreateCameraCapture(index);
    if (capture == 0)
    {
      std::cerr << "cannot capture image" << std::endl;
      exit(1);
    }
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
    cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, static_cast<double>(fps));

    // テクスチャ用のメモリを確保する
    texture = new GLubyte[width * height * 4];

    // パラメータに初期値を与えておく
    this->width = width;
    this->height = height;
    this->format = GL_BGRA;

    // スレッドとミューテックスを生成する
#ifdef _WIN32
    mutex = CreateMutex(NULL, TRUE, NULL);
    thread = CreateThread(NULL, 0, start, (LPVOID)this, 0, NULL);
#else
    pthread_mutex_init(&mutex, 0);
    pthread_create(&thread, 0, start, this);
#endif

    // スレッドが実行状態であることを記録する
    status = true;
  }

  // デストラクタ
  ~CaptureWorker()
  {
    // スレッドを停止する
    stop();

#ifdef _WIN32
    // スレッドの停止を待つ
    WaitForSingleObject(thread, 0); 
    CloseHandle(thread);

    // ミューテックスを破棄する
    CloseHandle(mutex);
#else
    // スレッドの停止を待つ
    pthread_join(thread, 0);

    // ミューテックスを破棄する
    pthread_mutex_destroy(&mutex);
#endif

    // image の release
    cvReleaseCapture(&capture);

    // メモリの解放
    delete[] texture;
  }

  // mutex のロック
  void lock(void)
  {
#ifdef _WIN32
    WaitForSingleObject(mutex, 0); 
#else
    pthread_mutex_lock(&mutex);
#endif
  }

  // mutex のリリース
  void unlock(void)
  {
#ifdef _WIN32
    ReleaseMutex(mutex);
#else
    pthread_mutex_unlock(&mutex);
#endif
  }

  // スレッドの開始
#ifdef _WIN32
  static DWORD WINAPI start(LPVOID arg)
#else
  static void *start(void *arg)
#endif
  {
    return reinterpret_cast<CaptureWorker *>(arg)->getTexture();
  }

  // スレッドの停止
  void stop(void)
  {
    lock();
    status = false;
    unlock();
  }

  // スレッドの停止判定
  bool check(void)
  {
    bool ret;

    lock();
    ret = status;
    unlock();

    return ret;
  }

  // テクスチャ作成
#ifdef _WIN32
  DWORD WINAPI getTexture(void)
#else
  void *getTexture(void)
#endif
  {
    for (;;)
    {
      // 終了条件のテスト
      if (!check()) break;

      if (cvGrabFrame(capture))
      {
        // キャプチャ映像から画像を切り出す
        IplImage *image = cvRetrieveFrame(capture);

        if (image)
        {
          // 切り出した画像の種類の判別
          width = image->width;
          height = image->height;
          if (image->nChannels == 3)
            format = GL_BGR;
          else if (image->nChannels == 4)
            format = GL_BGRA;
          else
            format = GL_RED;

          // テクスチャメモリへの転送
          GLsizei size = image->width * image->nChannels;
          lock();
          for (int y = 0; y < image->height; ++y)
            memcpy(texture + size * y, image->imageData + image->widthStep * y, size);
          unlock();
        }
        else
        {
          // １フレーム分待つ
#ifdef _WIN32
          Sleep(1000 / CAPTURE_FPS);
#else
          usleep(1000000 / CAPTURE_FPS);
#endif
        }
      }
    }

    return 0;
  }

  // テクスチャ転送
  void sendTexture(void)
  {
    lock();
    glBindTexture(GL_TEXTURE_2D, texname);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, texture);
    unlock();
  }
};
static CaptureWorker *worker = 0;

//
// キャプチャ終了時の処理
//
static void endCapture(void)
{
  delete worker;
}

//
// OpenCV の初期化
//
static void cvInit(void)
{
  // スレッドを生成する
  worker = new CaptureWorker(CV_CAP_ANY, CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_FPS);

  // キャプチャ終了時の処理を予約する
  atexit(endCapture);
}

//
// 初期設定
//
static int init(const char *title)
{
  // GLFW を初期化する
  if (glfwInit() == GL_FALSE)
  {
    // 初期化に失敗した
    std::cerr << "Error: Failed to initialize GLFW." << std::endl;
    return false;
  }

  // OpenGL Version 3.2 Core Profile を選択する
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
  glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  //glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);

  // ウィンドウを開く
  if (glfwOpenWindow(width, height, 8, 8, 8, 8, 24, 8, GLFW_WINDOW) == GL_FALSE)
  {
    // ウィンドウが開けなかった
    std::cerr << "Error: Failed to open GLFW window." << std::endl;
    return false;
  }

  // 開いたウィンドウに対する設定
  glfwEnable(GLFW_KEY_REPEAT);
  glfwSwapInterval(1);
  glfwSetWindowTitle(title);

  // ウィンドウのサイズ変更時に呼び出す処理の設定
  glfwSetWindowSizeCallback(resize);

  // マウスのボタン操作時に呼び出す処理の設定
  glfwSetMouseButtonCallback(mouse);

  // キーボード操作時に呼び出す処理の設定
  glfwSetKeyCallback(keyboard);

  // 補助プログラムによる初期化
  ggInit();

  // 初期設定
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glEnable(GL_BLEND);
  //glEnable(GL_MULTISAMPLE);
  //glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  //glEnable(GL_CULL_FACE);

  // フレームバッファオブジェクトの初期値
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // テクスチャの準備
  glGenTextures(1, &texname);
  glBindTexture(GL_TEXTURE_2D, texname);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_BGR, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return true;
}

//
// フレームバッファオブジェクトのアタッチメントの作成
//
static GLuint attachTexture(GLint internal, GLint format, GLsizei width, GLsizei height, GLenum attachment)
{
  // テクスチャオブジェクトの作成
  GLuint texture;
  glGenTextures(1, &texture);

  // テクスチャの結合
  glBindTexture(GL_TEXTURE_2D, texture);

  // テクスチャメモリの確保
  ggLoadTexture(width, height, internal, format);

  // フレームバッファオブジェクトに結合
  glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);

  return texture;
}

//
// フレームバッファオブジェクトの準備
//
static GLuint prepareFBO(const GLenum *buf, GLuint bufnum, GLuint *tex)
{
  // フレームバッファオブジェクトの作成
  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // 拡散反射係数 D
  tex[0] = attachTexture(GL_RGBA, GL_RGBA, FBOWIDTH, FBOHEIGHT, buf[0]);

  // 鏡面反射係数 S
  tex[1] = attachTexture(GL_RGBA, GL_RGBA, FBOWIDTH, FBOHEIGHT, buf[1]);

  // 位置 P
  tex[2] = attachTexture(GL_RGB16F, GL_RGB, FBOWIDTH, FBOHEIGHT, buf[2]);

  // 法線 N
  tex[3] = attachTexture(GL_RGB16F, GL_RGB, FBOWIDTH, FBOHEIGHT, buf[3]);

  // 深度
  tex[4] = attachTexture(GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, FBOWIDTH, FBOHEIGHT, GL_DEPTH_ATTACHMENT);

  return fbo;
}

//
// メインプログラム
//
int main(int argc, const char * argv[])
{
  // OpenCV の初期化
  cvInit();

  // 初期設定
  if (!init("Irradiance Map Screen Space Sub-Surface Scattaring")) return 1;

  // 第1パスのシェーダ
  GgObjShader pass1("pass1.vert", "pass1.frag");

  // 第2パスのシェーダ
  GLuint program = ggLoadShader("pass2.vert", "pass2.frag");
  GLint samplesLoc = glGetUniformLocation(program, "samples");
  GLint radiusLoc = glGetUniformLocation(program, "radius");
  GLint translucencyLoc = glGetUniformLocation(program, "translucency");
  GLint ambientLoc = glGetUniformLocation(program, "ambient");
  GLint mappingLoc = glGetUniformLocation(program, "mapping");
  GLint unitLoc = glGetUniformLocation(program, "unit");
  GLint pointLoc = glGetUniformLocation(program, "point");
  GLint mpLoc = glGetUniformLocation(program, "mp");
  GLint mrLoc = glGetUniformLocation(program, "mr");
  GLint mtLoc = glGetUniformLocation(program, "mt");

  // テクスチャユニット
  GLint unit[7];
  for (GLint i = 0; i < 7; ++i) unit[i] = i;

  // サンプル点
  GLfloat point[MAXSAMPLES][4];
  for (int i = 0; i < MAXSAMPLES; ++i)
  {
    float r = pow((float)rand() / (float)RAND_MAX, 1.0f / 3.0f);
    //float r = (float)rand() / (float)RAND_MAX;
    float t = 6.2831853f * (float)rand() / ((float)RAND_MAX + 1.0f);
    float cp = 2.0f * (float)rand() / (float)RAND_MAX - 1.0f;
    float sp = sqrt(1.0f - cp * cp);
    float ct = cos(t), st = sin(t);

    point[i][0] = sp * ct;
    point[i][1] = sp * st;
    point[i][2] = cp;
    point[i][3] = r;
  }

  // 放物面マッピング用のテクスチャ変換行列
  static const GLfloat mr[] =
  {
    -1.0f,  0.0f,  0.0f,  0.0f,
    1.0f,  1.0f,  0.0f,  2.0f,
    0.0f, -1.0f,  0.0f,  0.0f,
    1.0f,  1.0f,  0.0f,  2.0f,
  };

  // OBJ ファイル
  GgObj obj(objfile, true);
  obj.attachShader(pass1);

  // 表示領域を覆う矩形
  static const GLfloat position[4][3] =
  {
    { -1.0f, -1.0f, 0.0f },
    {  1.0f, -1.0f, 0.0f },
    { -1.0f,  1.0f, 0.0f },
    {  1.0f,  1.0f, 0.0f }
  };
  GgPoints rect(4, position, GL_TRIANGLE_STRIP);

  // レンダーターゲット
  static const GLenum buf[] =
  {
    GL_COLOR_ATTACHMENT0, // 拡散反射光
    GL_COLOR_ATTACHMENT1, // 環境光
    GL_COLOR_ATTACHMENT2, // 位置
    GL_COLOR_ATTACHMENT3, // 法線
  };

  // レンダーターゲットのアタッチメントの数
  static const GLsizei bufnum = sizeof buf / sizeof buf[0];

  // 必要になるテクスチャの数（レンダーターゲット＋深度＋環境）
  static const GLsizei texnum = bufnum + 2;

  // テクスチャ
  GLuint tex[texnum];

  // FBO の準備
  GLuint fbo = prepareFBO(buf, bufnum, tex);

  // ビュー変換行列を mv に求める
  GgMatrix mv = ggTranslate(0.0f, 0.0f, -4.0f);
  //GgMatrix mv = ggLookat(2.0f, 1.0f, 2.5f, 0.0f, -0.1f, 0.0f, 0.0f, 1.0f, 0.0f);
  //GgMatrix mv = ggLookat(-3.0f, 1.0f, 2.5f, 0.0f, -0.1f, 0.0f, 0.0f, 1.0f, 0.0f);

  // 時間計測用の Query Object
  GLuint query;
  glGenQueries(1, &query);

  // 経過時間の合計
  double tstep1 = 0.0, tstep2 = 0.0;
#if USE_GLFWGETTIME
  double sstep1 = 0.0, sstep2 = 0.0;
#endif

  // フレーム数
  int frames = 0;

  // ウィンドウが開いている間くり返し描画する
  while (glfwGetWindowParam(GLFW_OPENED))
  {
    // テクスチャ作成
    worker->sendTexture();

#if USE_GLFWGETTIME
    // 現在時刻
    double s0, s1, s2;
#endif

    // 時間の計測
    if (benchmark)
    {
#if USE_GLFWGETTIME
      glFinish();
      s0 = glfwGetTime();
#endif

      glBeginQuery(GL_TIME_ELAPSED, query);
    }

    // フレームバッファオブジェクトに描く
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // レンダーターゲットの指定
    glDrawBuffers(bufnum, buf);

    // ビューポートを FBO のサイズに合わせる
    glViewport(0, 0, FBOWIDTH, FBOHEIGHT);

    // 隠面消去を有効にする
    glEnable(GL_DEPTH_TEST);

    // 画面消去
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // シーンの描画
    pass1.loadMatrix(mp, mv * tbl.get());
    obj.draw();

    // 時間の計測
    if (benchmark)
    {
#if USE_GLFWGETTIME
      glFinish();
      s1 = glfwGetTime();
#endif
      glEndQuery(GL_TIME_ELAPSED);

      GLint done;
      do { glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done); } while (!done);

      GLuint64 elapsed_time;
      glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
      tstep1 += static_cast<double>(elapsed_time) * 0.000001;

      glBeginQuery(GL_TIME_ELAPSED, query);
    }

    // 通常のフレームバッファに描く
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 通常のフレームバッファ（バックバッファ）を指定する
    glDrawBuffer(GL_BACK);

    // ビューポートを FBO のサイズに合わせる
    glViewport(0, 0, width, height);

    // 隠面消去を無効にする
    glDisable(GL_DEPTH_TEST);

    // 環境のテクスチャ
    tex[5] = texname;

    // 遅延レンダリング
    glUseProgram(program);
    glUniform1i(samplesLoc, samples);
    glUniform1f(radiusLoc, radius * 0.01f);
    glUniform1f(translucencyLoc, translucency * 0.01f);
    glUniform4fv(ambientLoc, 1, ambient);
    glUniform4fv(mappingLoc, 1, mapping);
    glUniform1iv(unitLoc, texnum, unit);
    glUniform4fv(pointLoc, MAXSAMPLES, point[0]);
    glUniformMatrix4fv(mpLoc, 1, GL_FALSE, mp.get());
    glUniformMatrix4fv(mrLoc, 1, GL_FALSE, mr);
    glUniformMatrix4fv(mtLoc, 1, GL_FALSE, tbr.get());
    for (GLsizei i = 0; i < texnum; ++i)
    {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, tex[i]);
    }
    rect.draw();

    // 時間の計測
    if (benchmark)
    {
#if USE_GLFWGETTIME
      glFinish();
      s2 = glfwGetTime();
#endif
      glEndQuery(GL_TIME_ELAPSED);

      GLint done;
      do { glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done); } while (!done);

      GLuint64 elapsed_time;
      glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
      tstep2 += static_cast<double>(elapsed_time) * 0.000001;
    }

    // 実行時間の出力
    if (benchmark)
    {
      ++frames;

      std::cout << frames << ": "
        << tstep1 / frames
#if USE_GLFWGETTIME
        << " (" << (sstep1 += static_cast<double>(s1 - s0)) * 1000.0 / static_cast<double>(frames) << ")"
#endif
        << ", "
        << tstep2 / frames
#if USE_GLFWGETTIME
        << " (" << (sstep2 += static_cast<double>(s2 - s1)) * 1000.0 / static_cast<double>(frames) << ")"
#endif
        << std::endl;
    }

    // バッファを入れ替える
    glfwSwapBuffers();

    // マウス操作などのイベント待機
    //glfwWaitEvents();

    // マウスの現在位置を取得する
    int mx, my;
    glfwGetMousePos(&mx, &my);

    // 左マウスボタンドラッグ
    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE) tbl.motion(mx, my);

    // 右マウスボタンドラッグ
    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE) tbr.motion(mx, my);
  }

  return 0;
}
