#include <tracker/Tracker.h>

Tracker::Tracker(int w, int h, int bgBuffersize) 
  :w(w)
  ,h(h)
  ,bg_buffer(w, h, bgBuffersize)
  ,edt(w, h)
  ,erode_steps(2)
  ,dilate_steps(3)
  ,blur(w, h)
  ,blobs(w, h)
  ,pbo_toggle(0)
{
#if 1
  pbos[0] = pbos[1] = 0;

  if(!blur.setup(1.0, 10, 1)) {
    printf("Error: cannot setup the blur handler.\n");
    ::exit(EXIT_FAILURE);
  }


  // PBOs for async read back
  int nbytes = w * h;
  glGenBuffers(2, pbos);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[0]);
  glBufferData(GL_PIXEL_PACK_BUFFER, nbytes, NULL, GL_STREAM_COPY);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[1]);
  glBufferData(GL_PIXEL_PACK_BUFFER, nbytes, NULL, GL_STREAM_COPY);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#endif
}

void Tracker::beginFrame() {
  bg_buffer.beginFrame();
}

void Tracker::endFrame() {
  bg_buffer.endFrame();
}

void Tracker::apply() {

  // Perform background subtraction, erode, dilate, blur and thresholding.
  GLuint bg_tex = bg_buffer.apply();
  GLuint eroded_tex = edt.erode(bg_tex, erode_steps);
  GLuint dilated_tex = edt.dilate(eroded_tex, dilate_steps);
  GLuint blurred_tex = blur.blur(dilated_tex);
  GLuint thresholded_tex = edt.threshold(blurred_tex);

  // Read back the input for blob tracking into PBO "A"
  edt.setThresholdOutputAsReadBuffer();
  {
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[pbo_toggle]);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, blobs.getInputImageRowLength());
    glReadPixels(0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, NULL);
  }
  edt.resetReadBuffer();

  // Copy the pixels from the previous glReadPixels call (above) (PBO "B")
  glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[1 - pbo_toggle]);
  unsigned char* ptr = (unsigned char*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
  if(ptr) {
    memcpy(blobs.getInputImagePtr(), ptr,  w * h);
  }
  glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  // Toggle the pbos
  pbo_toggle = 1 - pbo_toggle;

  // Perform tracking.
  blobs.track();

  /* Reset defaults. */
  glPixelStorei(GL_PACK_ROW_LENGTH, 0);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
}

void Tracker::draw() {

  // draw the textures.
  tex_painter.clear();
  {
    tex_painter.texture(bg_buffer.getLastUpdatedTexture(), 0, h, w, -h);
    tex_painter.texture(edt.getThresholdedTex(), w, 0, w, h);
  }
  tex_painter.draw();

  // draw CV info
  shape_painter.clear();
  {
    drawContours(0, 0);
    drawBlobs(0, 0);
  }
  shape_painter.draw();

  // some labels.
  font.clear();
  font.write(5, h + 10, "Input and contours");
  font.write(w, h + 10, "Background segmentation");
  font.draw();
}

void Tracker::drawContours(int x, int y) {

  shape_painter.color(1.0, 0.0, 1.0, 1.0);

  for(size_t i = 0; i < blobs.contours.size(); ++i) {
    shape_painter.begin(GL_LINE_STRIP);
    std::vector<cv::Point> points = blobs.contours[i];
    for(size_t j = 0; j < points.size(); ++j) {
      cv::Point& p = points[j];
      shape_painter.vertex(p.x, y + p.y);
    }
    shape_painter.end();
  }
}

void Tracker::drawBlobs(int x, int y) {

  // draw trails.
  size_t col_dx = 0;
  for(size_t i = 0; i < blobs.blobs.size(); ++i) {
 
    Blob& b = blobs.blobs[i];

    if(b.trail.size() < 10) {
      continue;
    }

    if(b.matched == false) {
      continue;
    }
 
    shape_painter.color(0.0, 1.0, 0.2, 1.0);
    shape_painter.begin(GL_LINE_STRIP);

    for(size_t j = 0; j < b.trail.size(); ++j) {
      shape_painter.vertex(x + b.trail[j].x, y + b.trail[j].y);
    }

    shape_painter.end();
  }
 
  // draw directions
  for(size_t i = 0; i < blobs.blobs.size(); ++i) {

    Blob& b = blobs.blobs[i];
    if(b.matched == false) {
      continue;
    }

    shape_painter.color(1,1,0);
    shape_painter.begin(GL_LINES);
    shape_painter.vertex(x + b.position.x, y + b.position.y);
    shape_painter.vertex(x + b.position.x + (b.direction.x * 25), y + b.position.y + (b.direction.y * 25));
    shape_painter.end();
  }

  // draw contour centers.
  shape_painter.fill();
  shape_painter.color(0.9,0,0.3);
  for(size_t i = 0; i < blobs.new_blobs.size(); ++i) {
    cv::Point& p = blobs.new_blobs[i].position;
    shape_painter.circle(x + p.x, y + p.y, 6);
  }
}
