#include <tracker/ErodeDilateThreshold.h>

ErodeDilateThreshold::ErodeDilateThreshold(int w, int h) 
  :w(w)
  ,h(h)
  ,win_w(0)
  ,win_h(0)
  ,fullscreen_vao(0)
  ,fullscreen_vert(0)
  ,erode_frag(0)
  ,erode_prog(0)
  ,threshold_frag(0)
  ,threshold_prog(0)
  ,threshold_fbo(0)
  ,threshold_tex(0)
{

  GLint viewp[4] = { 0 } ;
  glGetIntegerv(GL_VIEWPORT, viewp);
  win_w = viewp[2];
  win_h = viewp[3];


  glGenVertexArrays(1, &fullscreen_vao);

  // Create ping/pong fbos for erode/dilate
  createFBO(fbo[0], tex[0]);
  createFBO(fbo[1], tex[1]);

  // Shaders
  fullscreen_vert = rx_create_shader(GL_VERTEX_SHADER, ROXLU_OPENGL_FULLSCREEN_VS);
  erode_frag = rx_create_shader(GL_FRAGMENT_SHADER, ERODE_FS);
  erode_prog = rx_create_program(fullscreen_vert, erode_frag, true);

  glUseProgram(erode_prog);
  rx_uniform_1i(erode_prog, "u_tex", 0);

  dilate_frag = rx_create_shader(GL_FRAGMENT_SHADER, DILATE_FS);
  dilate_prog = rx_create_program(fullscreen_vert, dilate_frag, true);

  glUseProgram(dilate_prog);
  rx_uniform_1i(dilate_prog, "u_tex", 0);

  threshold_frag = rx_create_shader(GL_FRAGMENT_SHADER, THRESHOLD_FS);
  threshold_prog = rx_create_program(fullscreen_vert, threshold_frag, true);
  glUseProgram(threshold_prog);
  rx_uniform_1i(threshold_prog, "u_tex", 0);

  createFBO(threshold_fbo, threshold_tex);
}

void ErodeDilateThreshold::setThresholdOutputAsReadBuffer() {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, threshold_fbo);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glViewport(0, 0, w, h);
}

void ErodeDilateThreshold::resetReadBuffer() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK_LEFT);
  glViewport(0, 0, win_w, win_h);
}

bool ErodeDilateThreshold::createFBO(GLuint& fbo, GLuint& tex) {
  
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf("Error: framebuffer is not complete in ErodeDilateThreshold.\n");
    ::exit(EXIT_FAILURE);
  }

  return true;
}

GLuint ErodeDilateThreshold::erode(GLuint intex, int num) {

  GLenum drawbufs[] = { GL_COLOR_ATTACHMENT0 } ;

  glViewport(0, 0, w, h);
  glBindVertexArray(fullscreen_vao);
  glUseProgram(erode_prog);  

  // first step we read from the incoming tex
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, intex);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
  glDrawBuffers(1, drawbufs);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // next we ping/pong num times to apply erode
  int toggle = 1;
  GLuint last_tex = 0;
  num = num - 1; // we performed one erode already

  for(int i = 0; i < num; ++i) {

    int read_index = 1 - toggle;
    int write_index = toggle;
  
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[read_index]);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[write_index]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    last_tex = tex[write_index];
    toggle = 1 - toggle;
  }

  // reset
  glViewport(0, 0, win_w, win_h);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return last_tex;
}

GLuint ErodeDilateThreshold::dilate(GLuint intex, int num) {

  GLenum drawbufs[] = { GL_COLOR_ATTACHMENT0 } ;

  glViewport(0, 0, w, h);
  glBindVertexArray(fullscreen_vao);
  glUseProgram(dilate_prog);  

  // first step we read from the incoming tex
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, intex);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
  glDrawBuffers(1, drawbufs);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // next we ping/pong num times to apply dilate
  int toggle = 1;
  GLuint last_tex = 0;
  num = num - 1; // we performed one dilate already

  for(int i = 0; i < num; ++i) {

    int read_index = 1 - toggle;
    int write_index = toggle;
  
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[read_index]);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[write_index]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    last_tex = tex[write_index];
    toggle = 1 - toggle;
  }

  // reset
  glViewport(0, 0, win_w, win_h);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return last_tex;
}


GLuint ErodeDilateThreshold::threshold(GLuint intex) {

  GLenum drawbufs[] = { GL_COLOR_ATTACHMENT0 } ;

  glViewport(0, 0, w, h);
  glBindVertexArray(fullscreen_vao);
  glUseProgram(threshold_prog);  

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, intex);
  glBindFramebuffer(GL_FRAMEBUFFER, threshold_fbo);
  glDrawBuffers(1, drawbufs);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // reset
  glViewport(0, 0, win_w, win_h);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return threshold_tex;
}
