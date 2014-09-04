#include <assert.h>
#include <math.h>
#include <sstream>
#include <string.h>
#include <tracker/Blur.h>
 
Blur::Blur(int w, int h)
  :w(w)
  ,h(h)
  ,fbo_x(0)
  ,fbo_y(0)
  ,vao(0)
  ,prog_x(0)
  ,prog_y(0)
  ,vert(0)
  ,frag_x(0)
  ,frag_y(0)
  ,tex_x(0)
  ,tex_y(0)
  ,depth(0)
  ,win_w(0)
  ,win_h(0)
  ,blur_amount(0)
  ,num_fetches(0)
  ,sample_size(0)
{
}
 
Blur::~Blur() {
  shutdown();
}
 
bool Blur::setup(float blurAmount, int texFetches, int sampleSize) {

  assert(blurAmount);
  assert(texFetches);
  assert(sampleSize);
 
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  
  win_w = viewport[2];
  win_h = viewport[3];
  blur_amount = blurAmount;
  num_fetches = texFetches;
  sample_size = sampleSize;

  glGenVertexArrays(1, &vao);
 
  if(!setupFBO()) {
    shutdown();
    return false;
  }
 
  if(!setupShader()) {
    shutdown();
    return false;
  }
 
  return true;
}
 
bool Blur::setupFBO() { 

  // FBO - scene capture
  // ----------------------------------------------------------
  glGenFramebuffers(1, &fbo_scene);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_scene);

  glGenRenderbuffers(1, &depth);
  glBindRenderbuffer(GL_RENDERBUFFER, depth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

  glGenTextures(1, &tex_scene);
  glBindTexture(GL_TEXTURE_2D, tex_scene);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_scene, 0);

  {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
      printf("Framebuffer not complete");
      shutdown();
      return false;
    }

    GLenum drawbufs[] = { GL_COLOR_ATTACHMENT0 } ;
    glDrawBuffers(1, drawbufs);
  }

  // FBO x-blur
  // ----------------------------------------------------------
  glGenFramebuffers(1, &fbo_x);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_x);
  
  glGenTextures(1, &tex_x);
  glBindTexture(GL_TEXTURE_2D, tex_x);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_x, 0);

  {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
      printf("Framebuffer not complete");
      shutdown();
      return false;
    }

    GLenum drawbufs[] = { GL_COLOR_ATTACHMENT0 } ;
    glDrawBuffers(1, drawbufs);
  }

  // FBO y-blur
  // ----------------------------------------------------------
  glGenFramebuffers(1, &fbo_y);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_y);

  glGenTextures(1, &tex_y);
  glBindTexture(GL_TEXTURE_2D, tex_y);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_y, 0);

  {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
      printf("Framebuffer not complete");
      shutdown();
      return false;
    }

    GLenum drawbufs[] = { GL_COLOR_ATTACHMENT0 } ;
    glDrawBuffers(1, drawbufs);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return true;
}
 
bool Blur::setupShader() {
 
  // CREATE SHADER SOURCES
  // -----------------------------------------------------------------------
  int num_els = num_fetches;  /* kernel size */
  float* weights = new float[num_els]; 
  float sum = 0.0;
  float sigma2 = blur_amount;
 
  weights[0] = gauss(0,sigma2);
  sum = weights[0];
 
  for(int i = 1; i < num_els; i++) {
    weights[i] = gauss(i, sigma2);
    sum += 2 * weights[i];
  }

  for(int i = 0; i < num_els; ++i) {
    weights[i] = (weights[i] / sum);
  }

  float dy = (1.0 / h) * sample_size;
  float dx = (1.0 / w) * sample_size;
 
  std::stringstream xblur;
  std::stringstream yblur;
  std::stringstream base_frag;
 
  base_frag << "#version 150\n"
            << "uniform sampler2D u_scene_tex;\n"
            << "in vec2 v_tex;\n"
            << "out vec4 fragcolor;\n";
 
  xblur << base_frag.str()
        << "void main() {\n"
        << "  fragcolor = texture(u_scene_tex, v_tex) * " << weights[0] << ";\n";
 
  yblur << base_frag.str()
        << "void main() {\n"
        << "  fragcolor = texture(u_scene_tex, v_tex) * " << weights[0] << ";\n";
 
  for(int i = 1 ; i < num_els; ++i) {
    yblur << "  fragcolor += texture(u_scene_tex, v_tex + vec2(0.0, " << float(i) * dy << ")) * " << weights[i] << ";\n" ;
    yblur << "  fragcolor += texture(u_scene_tex, v_tex - vec2(0.0, " << float(i) * dy << ")) * " << weights[i] << ";\n" ;
    xblur << "  fragcolor += texture(u_scene_tex, v_tex + vec2(" << float(i) * dx << ", 0.0)) * " << weights[i] << ";\n" ;
    xblur << "  fragcolor += texture(u_scene_tex, v_tex - vec2(" << float(i) * dx << ", 0.0)) * " << weights[i] << ";\n" ;
  }

  yblur << "}";
  xblur << "}";

  delete weights;
  weights = NULL;
 
  // -----------------------------------------------------------------------
  std::string yblur_s = yblur.str();
  std::string xblur_s = xblur.str();
  const char* yblur_vss = yblur_s.c_str();
  const char* xblur_vss = xblur_s.c_str();

  // x-blur
  vert = rx_create_shader(GL_VERTEX_SHADER, B_VS);
  frag_x = rx_create_shader(GL_FRAGMENT_SHADER, xblur_vss);
  prog_x = rx_create_program(vert, frag_x);
  glLinkProgram(prog_x);
  rx_print_shader_link_info(prog_x);
 
  // y-blur
  frag_y = rx_create_shader(GL_FRAGMENT_SHADER, yblur_vss);
  prog_y = rx_create_program(vert, frag_y);
  glLinkProgram(prog_y);
  rx_print_shader_link_info(prog_y);
 
  GLint u_scene_tex = 0;
 
  // set scene tex unit for 1st pass
  glUseProgram(prog_x);
  u_scene_tex = glGetUniformLocation(prog_x, "u_scene_tex");
  if(u_scene_tex < 0) {
    printf("Cannot find u_scene_tex (1)");
    shutdown();
    return false;
  }
  glUniform1i(u_scene_tex, 0);
 
  // set scene tex unit for 2nd pass
  glUseProgram(prog_y);
  u_scene_tex = glGetUniformLocation(prog_y, "u_scene_tex");
  if(u_scene_tex < 0) {
    printf("Cannot find u_scene_tex (2)");
    shutdown();
    return false;
  }
  glUniform1i(u_scene_tex, 0);

  return true;
}
 
void Blur::begin() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_scene);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,w, h);
}

void Blur::end() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, win_w, win_h);
}

void Blur::blit() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_scene);
  glViewport(0,0, w, h);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBlitFramebuffer(0, 0, win_w, win_h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR); // not tested with these win_w/win_h, w/h 
}
 
GLuint Blur::blur() {
 return blur(0);
}

// returns the blurred tex
GLuint Blur::blur(int tex) {
  assert(fbo_x);
  assert(fbo_y);

  glViewport(0, 0, w, h); 
  glBindVertexArray(vao);


  // x-blur 
  {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_x);
    glActiveTexture(GL_TEXTURE0);

    if(tex >= 0) {
      glBindTexture(GL_TEXTURE_2D, tex);
    }
    else {
      glBindTexture(GL_TEXTURE_2D, tex_scene);
    }
 
    glUseProgram(prog_x);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  // y-blur
  {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_y);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_x);
 
    glUseProgram(prog_y);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, win_w, win_h);

  return tex_y;
}
 
void Blur::setAsReadBuffer() {
  assert(fbo_x);
 
  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_y);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
}
 
float Blur::gauss(const float x, const float sigma2) {
  double coeff = 1.0 / (2.0 * PI * sigma2);
  double expon = -(x*x) / (2.0 * sigma2);
  return (float) (coeff*exp(expon));
}
 
void Blur::shutdown() {

  if(fbo_x) {
    glDeleteFramebuffers(1, &fbo_x);
  }
  fbo_x = 0;

  if(fbo_y) {
    glDeleteFramebuffers(1, &fbo_y);
  }
  fbo_y = 0;

  if(fbo_scene) {
    glDeleteFramebuffers(1, &fbo_scene);
  }
  fbo_scene = 0;
 
  if(vao) {
    glDeleteVertexArrays(1, &vao);
  }
  vao = 0;
 
  if(prog_x) {
    glDeleteProgram(prog_x);
  }
  prog_x = 0;
 
  if(prog_y) {
    glDeleteProgram(prog_y);
  }
  prog_y = 0;
 
  if(vert) {
    glDeleteShader(vert);
  }
  vert = 0;
 
  if(frag_x) {
    glDeleteShader(frag_x);
  }
  frag_x = 0;
 
  if(frag_y) {
    glDeleteShader(frag_y);
  }
  frag_y = 0;
 
  if(tex_x) {
    glDeleteTextures(1, &tex_x);
  }
  tex_x = 0;
 
  if(tex_y) {
    glDeleteTextures(1, &tex_y);
  }
  tex_y = 0;
 
  win_w = 0;
  win_h = 0;
  blur_amount = 0.0f;
  num_fetches = 0;
}

void Blur::print() {

  printf("blur.tex_scene: %d\n", tex_scene);
  printf("blur.tex_x: %d\n", tex_x);
  printf("blur.tex_y: %d\n", tex_y);
  printf("blur.fbo_x: %d\n", fbo_x);
  printf("blur.fbo_y: %d\n", fbo_y);
  printf("-\n");

}
