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

  
  BlobTracker
  -----------
  The BlobTracker class is does all the hard work of detecting and 
  matching new blobs with previously detected blobs. We match new
  blobs based on their similarity with older blobs. At this moment we
  focussed on tracking and matching pedestrians filmed from above.
  Though this works very well for thresholded depth images, infrared
  tracking and pedestrians filmed from above. 

  You need to fill the input image and then call track(). Because this
  class was created for the openGL based tracking library we have a 
  function `getInputImagePtr()` that we fill directly with the pixels
  that we download from the GPU (See Tracker::apply()).

 */
#ifndef TRACKER_BLOB_TRACKER_H
#define TRACKER_BLOB_TRACKER_H

#include <stdint.h>
#include <vector>
#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>

/* ---------------------------------------------------*/

class Similarity {                                                    /* the Similarity class is used to compte the new detected blobs with the already found ones. */
  public:
   Similarity();
   void update();                                                     /* calculates the similiarity value */
  public:
   size_t new_dx;                                                     /* index of a blob detected in the new frame */
   size_t old_dx;                                                     /* index of a blob detected earlier */
   int64_t area_dist_sq;                                              /* area different (squared) */
   int64_t pos_dist_sq;                                               /* position distance squared */
   int64_t value;                                                     /* the actuall similarity value between blob new_dx and old_dx */
};

struct SimilaritySorter {
  bool operator()(const Similarity& a, const Similarity& b) {
    return a.value < b.value;
  }
};

/* ---------------------------------------------------*/

class Blob {
public:
  Blob();
  void update();                                                      /* updates some of te members; e.g. direction */

public:
  int age;                                                            /* the number of frames we detected this same blob */
  int id;                                                             /* @todo - assign a unique ID to each detected blob */
  int area;                                                           /* the area of the blob */
  bool matched;                                                       /* used in BlobTracker::track(), set to true when we found this blob in the last frame */
  cv::Point2f direction;                                              /* the averaged direction the blob is heading towards */
  cv::Point position;                                                 /* the center position */
  std::vector<cv::Point> trail;                                       /* last N-positions */
};

/* ---------------------------------------------------*/

class BlobTracker {
 public:
  BlobTracker(int w, int h);
  void track();                                                       /* once you've filled the input_image with some pixel data, call track() to perform the blob tracking. */
  int getInputImageRowLength();                                       /* returns the row length for the input image; this is used for e.g. GL_PACK_ROW_LENGTH when reading back pixels from the GPU */
  unsigned char* getInputImagePtr();                                  /* returns a pointer to the image buffer that we can fill */

 private:
  void updateContours();                                              /* uses openCV to find contours that are used in updateBlobs()/updateClusters(). */
  void updateBlobs();                                                 /* find the contour centers and create new blobs */
  void updateClusters();                                              /* this does the actual work. it finds and matches blobs based on similarty. */

 public:
  int w;                                                               /* the width of the image buffer on which we perform tracking. */
  int h;                                                               /* the height of the image buffer on which we perform tracking */
  cv::Mat input_image;                                                 /* the input image on which we perform the tracking. you need to copy pixel data into this one. */
  std::vector<Blob> new_blobs;                                         /* blobs detected in the last frame */
  std::vector<Blob> blobs;                                             /* the blobs we found and that we are tracking */
  std::vector<std::vector<cv::Point> > contours;                       /* the found contours */
  std::map<size_t, std::vector<Similarity> > similarities;             /* similarities between the new and old blobs; is updated every time you call track() */
};

/* ---------------------------------------------------*/

inline int BlobTracker::getInputImageRowLength() {
  int row_len = (int) input_image.step / (int) input_image.elemSize();
  return row_len;
}

inline unsigned char* BlobTracker::getInputImagePtr() {
  return (unsigned char*)input_image.data;
}

#endif
