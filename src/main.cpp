/*

  BASIC GLFW + GLXW WINDOW AND OPENGL SETUP 
  ------------------------------------------
  See https://gist.github.com/roxlu/6698180 for the latest version of the example.
 
*/
#include <stdlib.h>
#include <stdio.h>

#if defined(__linux) || defined(_WIN32)
#  include <GLXW/glxw.h>
#endif
 
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#define ROXLU_USE_JPG
#define ROXLU_USE_MATH
#define ROXLU_USE_PNG
#define ROXLU_USE_OPENGL
#define ROXLU_USE_FONT
#define ROXLU_IMPLEMENTATION
#include <tinylib.h>

#define VIDEO_CAPTURE_IMPLEMENTATION
#include <videocapture/CaptureGL.h>

#include <tracker/Tracker.h>

void button_callback(GLFWwindow* win, int bt, int action, int mods);
void cursor_callback(GLFWwindow* win, double x, double y);
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* win, unsigned int key);
void error_callback(int err, const char* desc);
void resize_callback(GLFWwindow* window, int width, int height);

int main() {

  glfwSetErrorCallback(error_callback);
 
  if(!glfwInit()) {
    printf("Error: cannot setup glfw.\n");
    return false;
  }
 
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  GLFWwindow* win = NULL;
  int w = 640;
  int h = 270;

  win = glfwCreateWindow(w, h, "GLFW", NULL, NULL);
  if(!win) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetFramebufferSizeCallback(win, resize_callback);
  glfwSetKeyCallback(win, key_callback);
  glfwSetCharCallback(win, char_callback);
  glfwSetCursorPosCallback(win, cursor_callback);
  glfwSetMouseButtonCallback(win, button_callback);
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);
 
#if defined(__linux) || defined(_WIN32)
  if(glxwInit() != 0) {
    printf("Error: cannot initialize glxw.\n");
    ::exit(EXIT_FAILURE);
  }
#endif
 
  // ----------------------------------------------------------------
  // THIS IS WHERE YOU START CALLING OPENGL FUNCTIONS, NOT EARLIER!!
  // ----------------------------------------------------------------
  
  ca::CaptureGL capture;
  Tracker tracker(320, 240, 10);
  capture.cap.listDevices();
  if(capture.open(0, tracker.w, tracker.h) < 0) {
    printf("Erorr: cannot open the tracker.\n");
    ::exit(EXIT_FAILURE);
  }

  if(capture.start() < 0) {
    printf("Error: cannot start the capture..\n");
    ::exit(EXIT_FAILURE);
  }

  while(!glfwWindowShouldClose(win)) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    tracker.beginFrame();
    {
      capture.update();
      capture.draw();
    }
    tracker.endFrame();
    
    tracker.apply();
    tracker.draw();
    
    glfwSwapBuffers(win);
    glfwPollEvents();
  }
 
  capture.stop();
  capture.close();

  glfwTerminate();
 
  return EXIT_SUCCESS;
}
 
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {
  
  if(action != GLFW_PRESS) {
    return;
  }

  switch(key) {
    case GLFW_KEY_ESCAPE: {
      glfwSetWindowShouldClose(win, GL_TRUE);
      break;
    }
    case GLFW_KEY_SPACE: {
      break;
    }
  };
}

void error_callback(int err, const char* desc) {
  printf("GLFW error: %s (%d)\n", desc, err);
}

void resize_callback(GLFWwindow* window, int width, int height) { 

}

void button_callback(GLFWwindow* win, int bt, int action, int mods) {
  double x,y;
  if(action == GLFW_PRESS || action == GLFW_REPEAT) { 
    glfwGetCursorPos(win, &x, &y);
  }
}

void cursor_callback(GLFWwindow* win, double x, double y) { }

void char_callback(GLFWwindow* win, unsigned int key) { }
