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


  ErodeDilateThreshold
  --------------------

  This class contains a couple of features that are helpfull when 
  performing basic computer vision algorithms. This class will erode, 
  dilate and threshold on the GPU. 

  - this is experimental code and might not result in what you expect -

  That said, this works great for the Tracker library. This library
  was designed to be used using the following steps and in the following
  order of calls. Every function returns the texture that holds the 
  eroded/thresholded/etc.. result.

  ````c++

      ErodeDilateThreshold edt(320,240)
    
      GLuint eroded_tex = edt.erode(bg_tex, erode_steps);
      GLuint dilated_tex = edt.dilate(eroded_tex, dilate_steps);
      GLuint blurred_tex = blur.blur(dilated_tex);
      GLuint thresholded_tex = edt.threshold(blurred_tex);

  ````      

 */
#ifndef TRACKER_ERODE_DILATE_H
#define TRACKER_ERODE_DILATE_H

#define ROXLU_USE_OPENGL
#include <tinylib.h>

static const char* ERODE_FS = ""
  "#version 330\n"
  "uniform sampler2D u_tex;"
  "in vec2 v_texcoord;"
  "layout( location = 0 ) out vec4 fragcolor;"
  ""
  ""
  "void main() {"
  "  fragcolor = vec4(0.0, 0.0, 0.0, 1.0);"
  "  fragcolor.r = texture(u_tex, v_texcoord).r;"
  "  float v = 0.0;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(-1,-1)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(0,-1)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(1,1)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(1,0)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(-1,0)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(0,1)).r;"
  "  fragcolor.r = (v > 2.0) ? 1.0 : 0.0; "
  "}"
  "";

static const char* DILATE_FS  = ""
  "#version 330\n"
  "uniform sampler2D u_tex;"
  "in vec2 v_texcoord;"
  "layout( location = 0 ) out vec4 fragcolor;"
  ""
  ""
  "void main() {"
  "  fragcolor = vec4(0.0, 0.0, 0.0, 1.0);"
  "  fragcolor.r = texture(u_tex, v_texcoord).r;"
  "  float v = 0.0;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(-1,-1)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(0,-1)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(1,1)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(1,0)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(-1,0)).r;"
  "  v += textureOffset(u_tex, v_texcoord, ivec2(0,1)).r;"
  "  fragcolor.r = (v > 0.0) ? 1.0 : 0.0; "
  "}"
  "";

static const char* THRESHOLD_FS = ""
  "#version 330\n"
  "uniform sampler2D u_tex;"
  "in vec2 v_texcoord;"
  "layout( location = 0 ) out vec4 fragcolor;"
  ""
  "void main() {"
  "  fragcolor = vec4(0.0, 0.0, 0.0, 1.0);"
  "  fragcolor.r = texture(u_tex, v_texcoord).r > 0.5 ? 1.0 : 0.0;"
  "}"
  "";

class ErodeDilateThreshold {

 public:
  ErodeDilateThreshold(int w, int h);
  GLuint erode(GLuint intex, int num);              /* returns a reference to a texture that is the eroded version of the input texture */
  GLuint dilate(GLuint intex, int num);             /* returns a reference to a texture that is the dilated version of the input texture */
  GLuint threshold(GLuint intex);                   /* threshold the given texture */
  bool createFBO(GLuint& fbo, GLuint& tex);         /* creates a FBO with one texture attachment (grayscale) */
  void setThresholdOutputAsReadBuffer();            /* this will make sure that a glReadPixels() wil read from the thresholded buffer */
  void resetReadBuffer();                           /* sets the default framebuffer again */
  GLuint getThresholdedTex();                       /* get the thresholded texture output. */

 public:
  int w; 
  int h;
  int win_w;
  int win_h;
  GLuint fullscreen_vao;
  GLuint fullscreen_vert;
  GLuint erode_frag;
  GLuint erode_prog;
  GLuint dilate_frag;
  GLuint dilate_prog;
  GLuint threshold_frag;
  GLuint threshold_prog;
  GLuint threshold_fbo;
  GLuint threshold_tex;
  GLuint fbo[2];
  GLuint tex[2];
};

inline GLuint ErodeDilateThreshold::getThresholdedTex() {
  return threshold_tex;
}

#endif
