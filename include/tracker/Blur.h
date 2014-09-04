/*

---------------------------------------------------------------------------------
 
                                               oooo
                                               `888
                oooo d8b  .ooooo.  oooo    ooo  888  oooo  oooo
                `888""8P d88' `88b  `88b..8P'   888  `888  `888
                 888     888   888    Y888'     888   888   888
                 888     888   888  .o8"'88b    888   888   888
                d888b    `Y8bod8P' o88'   888o o888o  `V88V"V8P'
 
                                                  www.roxlu.com
                                             www.apollomedia.nl
                                          www.twitter.com/roxlu
 
---------------------------------------------------------------------------------

 
  Blur
  -----
 
  This blur class can be used to blur the current GL_READ_BUFFER or a texture
  that you pass into the blur() function. 

  In case of using the GL_READ_BUFFER We read the current GL_READ_FRAMEBUFFER into our 
  intermediate scene texture and then perfom 2 passes. This means we might have one extra 
  blit which might be considered  a performance issue; thouh on modern gpus you won't 
  notice anything.  To use the blurred result, call setAsReadBuffer() and either blit it 
  to the  default framebuffer or do something else with it.

  When you pass a texture into the `blur(texid)` function we will use that texture
  as input and blur it. The returned GLuint from `blur(texid)` is the texture that contains
  the blurred image.

  <example>

    Blur blur;
    blur.setup(15,10,3);

    // somewhere in draw
    blur.blur();

  </example>
 
*/
 
#ifndef ROXLU_BLUR_H
#define ROXLU_BLUR_H

#define ROXLU_USE_MATH
#define ROXLU_USE_OPENGL
#include <tinylib.h>
 
static const char* B_VS = ""
  "#version 150\n"
 
  "const float w = 1.0;"
  "const float h = 1.0;"
 
  "const vec2 vertices[4] = vec2[]("
  "     vec2(-w, h), "
  "     vec2(-w, -h), "
  "     vec2( w,  h), "
  "     vec2( w, -h) "
  ");"
 
  "const vec2 texcoords[4] = vec2[] ("
  "     vec2(0.0, 0.0), "
  "     vec2(0.0, 1.0), "
  "     vec2(1.0, 0.0), "
  "     vec2(1.0, 1.0) "
  ");"
 
  "out vec2 v_tex;"
 
  "void main() {"
  "  gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);"
  "  v_tex = texcoords[gl_VertexID]; "
  "}"
  "";
 
class Blur {
 public:
  Blur(int w, int h);                                                   
  ~Blur();
  bool setup(float blurAmount = 8.0f, int texFetches = 5, int sampleSize = 3);           /* setups and apply the blur amount. the more texel fetches the better the quality, the bitter the sampleSize the bigger the blur too. values like: setup(15,10,3)  */
  void begin();
  void end();
  GLuint blur(int tex= -1);
  GLuint blur();                                                                         /* applies the blur */
  void blit();                                                                           /* will blit the current read buffer into the scene texture; you can use this instead of capturing a scene with begin()/end() */
  void setAsReadBuffer();                                                                /* sets the result to the current read buffer */
  void print();                                                                          /* print some debug info */
                                                                                         
 private:                                                                                
  bool setupFBO();                                                                       /* sets up the FBOs and textures */
  bool setupShader();                                                                    /* generates the shaders */
  float gauss(const float x, const float sigma2);                                        /* 1d gaussian function */
  void shutdown();                                                                       /* resets this class; destroys all allocated objects */
                                                                                         
 public:                                                                                 
  int w;
  int h;
  GLuint vao;                                                                            /* we use attribute-less rendering; but we need a vao as GL core 3 does not allow drawing with the default VAO */
  GLuint vert;                                                                           /* the above vertex shader */
  GLuint fbo_scene;                                                                      /* used to capture the scene; we blit the current read buffer into our scene texture, so you don't need "begin()" .. "end()" */
  GLuint tex_scene;                                                                      /* will hold the scene texture */
  GLuint fbo_x;                                                                          /* the framebuffer for rtt */
  GLuint frag_x;                                                                         /* vertical fragment shader */
  GLuint prog_x;                                                                         /* program for the vertical blur */
  GLuint tex_x;                                                                          /* first pass (scene capture) + combined blur */
  GLuint fbo_y;                                                                          /* fbo for the y texture */
  GLuint frag_y;                                                                         /* horizontal fragment shader */
  GLuint prog_y;                                                                         /* program for the horizontal blur */
  GLuint tex_y;                                                                          /* intermedia texture */
  GLuint depth;
  int win_w;                                                                             /* width of the window/fbo */
  int win_h;                                                                             /* height of the window/fbo */
  float blur_amount;                                                                     /* the blur amount, 5-8 normal, 8+ heavy */
  int num_fetches;                                                                       /* how many texel fetches (half), the more the heavier for the gpu but more blur  */
  int sample_size;
};
 
#endif
