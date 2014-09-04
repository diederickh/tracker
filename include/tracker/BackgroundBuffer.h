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


  BackgroundBuffer
  -----------------

  The BackgroundBuffer class segments moving objects from background objects 
  in a very but effective way. The approach to the background/foreground 
  segmentation is as follows:

    - We keep track of the last N frames (pass as `num` to the constructor)
    - For each frame we sum the a color value by using `1/num * color-channel`
      For example when we have num = 4:
    
          total_red += frame0.r * (1.0/4.0) 
          total_red += frame1.r * (1.0/4.0) 
          total_red += frame2.r * (1.0/4.0) 
          total_red += frame3.r * (1.0/4.0) 
    
    - When we have the total avaraged from the last N-frames we calculate
      the distance for this color with the newest frame. When the distance
      is bigger then K we flag the pixel as foreground, else as background.

  In the future we can apply all kinds of weightings to this background 
  buffer to make it work better with slowly moving objects. E.g. the `num`
  you pass basically represents the history time. Max. value depends on your
  GPU (the number of sampler2D)
  

 */
#ifndef TRACKER_BACKGROUND_BUFFER_H
#define TRACKER_BACKGROUND_BUFFER_H

/* We use the glad GL wrapper, see: https://github.com/Dav1dde/glad */
#include <glad/glad.h>

#define ROXLU_USE_OPENGL
#include <tinylib.h>
#include <vector>

static const char* BG_BUFFER_VS = ""
  "#version 330\n"
  "const vec2 verts[4] = vec2[] ("
  "  vec2(-1.0, 1.0), "
  "  vec2(-1.0, -1.0), "
  "  vec2(1.0, 1.0), "
  "  vec2(1.0, -1.0) "
  ");"
  "const vec2 texcoords[4] = vec2[] ("
  "  vec2(0.0, 0.0), "
  "  vec2(0.0, 1.0), "
  "  vec2(1.0, 0.0), "
  "  vec2(1.0, 1.0) "
  ");"
  "out vec2 v_texcoord;"
  "void main() {"                               
  "  gl_Position = vec4(verts[gl_VertexID], 0.0, 1.0);"
  "  v_texcoord = texcoords[gl_VertexID];"
  "}"
  "";

struct BackgroundFBO {
  GLuint fbo;
  GLuint tex;
};

class BackgroundBuffer {
 public:
  BackgroundBuffer(int w, int h, int num);                         /* Create a background buffer/segmentation with the width/height (w/h) and num numbers of frames */
  BackgroundFBO createBuffer();                                    /* We create `num` fbos with one color attachment */
  void beginFrame();                                               /* Begin grabbing a frame. Between beginFrame() and endFrame() you should draw your raw input */
  void endFrame();                                                 /* End grabbing a frame. "" "" "" */
  void resize(int winW, int winH);                                 /* When your viewport resizes call this. */
  GLuint apply();                                                  /* Returns the texture that contains the current background model. We return the texture that contains the one channel (GL_R8) segmented image  */
  GLuint getLastUpdatedTexture();                                  /* Returns the texture id that contains the latest frame */
 public:
  size_t index;                                                    /* Current index for the texture that we fill between beginFrame()/endFrame() */
  int w;                                                           /* The width of the textures */
  int h;                                                           /* The height of the textures */
  int win_w;                                                       /* The window width (we reset the viewport after endFrame()). */
  int win_h;                                                       /* The window height */
  int num;                                                         /* Number of frames in our buffer */
  std::vector<BackgroundFBO> buffers;                              /* The FBOs + Textures (on GL_COLOR_ATTACHMENT0) */
 
  GLuint fbo;                                                      /* FBO used to store the result (in out_tex) */
  GLuint vert;                                                     /* Vertex shader to grab a frame */
  GLuint frag;                                                     /* The fragment shader that does the background distance calc. generated in the c'tor */
  GLuint prog;                                                     /* Shader program that performs the bg subtraction */
  GLuint vao;                                                      /* VAO to back our attribute less rendering */
  GLuint out_tex;                                                  /* The result texture with foreground pixels being 1 */
  int last_index;                                                  /* Internally used; last index that we wrote frame data into */
};

inline GLuint BackgroundBuffer::getLastUpdatedTexture() {
  return buffers[last_index].tex;
}

inline void BackgroundBuffer::resize(int ww, int wh) {
  win_w = ww;
  win_h = wh;
}

#endif
