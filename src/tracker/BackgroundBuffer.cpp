#include <tracker/BackgroundBuffer.h>
#include <sstream>

BackgroundBuffer::BackgroundBuffer(int w, int h, int num) 
  :w(w)
  ,h(h)
  ,win_w(0)
  ,win_h(0)
  ,num(num)
  ,index(0)
  ,vert(0)
  ,frag(0)
  ,prog(0)
  ,vao(0)
  ,last_index(0)
  ,out_tex(0)
{
#if 1
  GLint vp[4] = { 0 };
  glGetIntegerv(GL_VIEWPORT, vp);
  win_w = vp[2];
  win_h = vp[3];

  // Create num fbos;
  for(int i = 0; i < num; ++i) {
    BackgroundFBO buf = createBuffer();
    buffers.push_back(buf);
  }

  BackgroundFBO bg_fbo = createBuffer();
  fbo = bg_fbo.fbo;
  out_tex = bg_fbo.tex;

  std::stringstream ss;
  float div = 1.0 / num;

  ss << "#version 330\n";
  ss << "in vec2 v_texcoord;\n";
  ss << "layout( location = 0 ) out vec4 fragcolor;\n";
  ss << "uniform sampler2D u_last_frame;\n";

  for(int i = 0; i < num; ++i) {
    ss << "uniform sampler2D u_tex" << i << ";\n";
  }

  ss << "void main() { \n"
     << "  fragcolor = vec4(0.0, 0.0, 0.0, 1.0);\n"
     << "  vec3 history = vec3(0.0);"
     << "  vec3 last_col = texture(u_last_frame, v_texcoord).rgb;\n";
  
  for(int i = 0; i < num; ++i) {
    ss <<  "  history += texture(u_tex" << i << ", v_texcoord).rgb * " << div << ";\n";
  }

  ss << " float dr = last_col.r - history.r; \n"
     << " float dg = last_col.g - history.g; \n"
     << " float db = last_col.b - history.b; \n"
     << " float d = sqrt(dr * dr + dg * dg + db * db); \n"
     << " if(d > 0.1) { \n"   // only keep changes which are big enough
     <<      "fragcolor.rgb = vec3(1.0);  \n"
     << "  } \n"
     << " } \n"
     << "";

  std::string frag_src = ss.str();
  const char* frag_src_ptr = frag_src.c_str();

  vert = rx_create_shader(GL_VERTEX_SHADER, BG_BUFFER_VS);
  frag = rx_create_shader(GL_FRAGMENT_SHADER, frag_src_ptr);
  prog = rx_create_program(vert, frag, true);
  glUseProgram(prog);

  for(int i = 0; i < num; ++i) {
    char texname[512] = { 0 } ;
    sprintf(texname, "u_tex%d", i);
    rx_uniform_1i(prog, texname, i);
  }
  rx_uniform_1i(prog, "u_last_frame", num);

  glGenVertexArrays(1, &vao);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

BackgroundFBO BackgroundBuffer::createBuffer() {

  BackgroundFBO bf;

  glGenFramebuffers(1, &bf.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, bf.fbo);
  
  glGenTextures(1, &bf.tex);
  glBindTexture(GL_TEXTURE_2D, bf.tex);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bf.tex, 0);

  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf("Error: framebuffer not complete.\n");
    ::exit(EXIT_FAILURE);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return bf;
}

void BackgroundBuffer::beginFrame() {
  GLenum drawbuffers[] = { GL_COLOR_ATTACHMENT0 } ;
  glBindFramebuffer(GL_FRAMEBUFFER, buffers[index].fbo);
  glDrawBuffers(1, drawbuffers);
  glViewport(0,0,w,h);
  glClear(GL_COLOR_BUFFER_BIT); /* not 100% necessary */
  last_index = index;
}

void BackgroundBuffer::endFrame() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, win_w, win_h);
  ++index %= buffers.size();
}

GLuint BackgroundBuffer::apply() {

  GLenum drawbuffers[] = { GL_COLOR_ATTACHMENT0 } ;
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glDrawBuffers(1, drawbuffers);
  glViewport(0, 0, w, h);
  glBindVertexArray(vao);
  glUseProgram(prog);

  for(int i = 0; i < (int)buffers.size(); ++i) {
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, buffers[i].tex);
  }

  glActiveTexture(GL_TEXTURE0 + buffers.size());
  glBindTexture(GL_TEXTURE_2D, buffers[last_index].tex);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, win_w, win_h);

  return out_tex;
}
