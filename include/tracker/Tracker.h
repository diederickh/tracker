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

  Tracker
  -------

  This tracker has been created and tested to track pedestrians using a RGB webcam from above. 

  This class is the main API around the other classes of this  Tracker library. 
  We grab a frame (beginFrame()/endFrame()) and  then apply a couple of computer 
  vision tasks like dilate, erode, blur and threshold. Once we have a good segmented 
  background/foreground image, we find contours and track the centers of them using 
  the BlobTracker.

  ````c++
  Tracker tracker(320,240, 10);
  
  tracker.beginFrame();
  {
#if 0
       draw here the thing you want to track. 
       this is the raw input for the tracking.
       for example, you can draw the depth frames 
       from a kinect here. Make sure that it will 
       fill the complete viewport.
#endif
  }
  tracker.endFrame();
  
  // apply tracking
  tracker.apply();

  // draw some info 
  tracker.draw();
  ````
 */
#ifndef TRACKER_H
#define TRACKER_H 

/* We use the glad GL wrapper, see: https://github.com/Dav1dde/glad */
#include <glad/glad.h>

#define ROXLU_USE_FONT
#define ROXLU_USE_OPENGL
#include <tinylib.h>

#include <tracker/BackgroundBuffer.h>
#include <tracker/ErodeDilateThreshold.h>
#include <tracker/Blur.h>
#include <tracker/BlobTracker.h>
#include <iostream>

class Tracker {
 public:
  Tracker(int w, int h, int bgBufferSize = 10);                     /* Create the tracker using the w/h dimensions to perform the computer vision algos on */
  void beginFrame();                                                /* Begin drawing the frame on which you want to perform tracking */
  void endFrame();                                                  /* End drawing the frame on which you want to perform tracking */
  void apply();                                                     /* Apply the tracking */
  void draw();                                                      /* Draw some tracking info */

 private:
  void drawContours(int x, int y);                                  /* Draw the found contours (gets called by draw()) */
  void drawBlobs(int x, int y);                                     /* Draw the detected and tracked blobs */
  
 public:
  int w;
  int h;
  BackgroundBuffer bg_buffer;                                       /* The BackgroundBuffer which will be used to create a background model */
  ErodeDilateThreshold edt;                                         /* Erode, Dilate and Threshold operations. */
  Blur blur;                                                        /* After the erode and dilate steps we blur and threshold the output */
  BlobTracker blobs;                                                /* The BlobTracker instance does the blog matching and following */
  Painter shape_painter;                                            /* Simply GL painting class */
  Painter tex_painter;                                              /* Used to draw textures */
  PixelFont font;                                                   /* Used to write some text */
  int erode_steps;                                                  /* Number of erode iterations */
  int dilate_steps;                                                 /* Number of dilate iterations */
  GLuint pbos[2];                                                   /* GL_PIXEL_PACK_PACK bufers to optimize the GPU > CPU transfers */
  int pbo_toggle;                                                   /* Toggles between PBOs */
};
#endif
