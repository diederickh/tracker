#include <tracker/BlobTracker.h>
#include <stdint.h>

/* ---------------------------------------------------*/

Similarity::Similarity() 
  :new_dx(0)
  ,old_dx(0)
  ,value(0)
  ,area_dist_sq(0)
  ,pos_dist_sq(0)
{
}

void Similarity::update() {
  //value = 0.95 * pos_dist_sq + 0.05 * area_dist_sq;
  value = pos_dist_sq;
}

/* ---------------------------------------------------*/

Blob::Blob() 
  :id(0)
  ,area(0)
  ,age(0)
  ,matched(false)
{
}

// @todo - we can optimize this
void Blob::update() {

  if(trail.size() < 3) {
    return;
  }

  // Update the direction vector by using the last N-directions 
  cv::Point2f last_direction(0.0, 0.0);
  int num = 0;
  for(size_t i = trail.size()-1; i > 0; --i) {
    cv::Point2f a = trail[i-0];
    cv::Point2f b = trail[i-1]; 
    last_direction += (a - b);
    ++num;
    if(num >= 5) {
      break;
    }
  }

  // We use part of the normalized direction of the last N-frames.
  float dist = sqrtf(last_direction.x * last_direction.x + last_direction.y * last_direction.y);
  last_direction.x /= dist;
  last_direction.y /= dist;

  direction.x = direction.x * 0.9 + last_direction.x * 0.1;
  direction.y = direction.y * 0.9 + last_direction.y * 0.1;

  if(direction.x != direction.x) {
    direction.x = 0.0f;
  }
  if(direction.y != direction.y) {
    direction.y = 0.0f;
  }

}

/* ---------------------------------------------------*/

BlobTracker::BlobTracker(int w, int h) 
  :w(w)
  ,h(h)
  ,input_image(h, w, CV_8UC1, NULL, cv::Mat::AUTO_STEP)
{
  input_image.create(h, w, CV_8UC1);
}

void BlobTracker::track() {

  contours.clear();
 
  updateContours();
  updateBlobs();
  updateClusters();
}

void BlobTracker::updateContours() {
 cv::findContours(input_image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
}

void BlobTracker::updateBlobs() {

  float x = 0;
  float y = 0;
  int min_x = INT_MAX;
  int max_x = INT_MIN;
  int min_y = INT_MAX;
  int max_y = INT_MIN;
  int dx = 0;
  int dy = 0;
  int area = 0;

  new_blobs.clear();

  for(size_t i = 0; i < contours.size(); ++i) {

    min_x = INT_MAX;
    max_x = INT_MIN;
    min_y = INT_MAX;
    max_y = INT_MIN;
    x = 0;
    y = 0;

    std::vector<cv::Point>& points = contours[i];
    for(size_t j = 0; j < points.size(); ++j) {
      cv::Point& p = points[j];
      x += p.x;
      y += p.y;


      if(p.x < min_x) {
        min_x = p.x;
      }
      if(p.x > max_x) {
        max_x = p.x;
      }
      if(p.y < min_y) {
        min_y = p.y;
      }
      if(p.y > max_y) {
        max_y = p.y;
      }
    }

    dx = max_x - min_x;
    dy = max_y - min_y;
    area = dx * dy;
    
    if(area > 100) { 
      Blob blob;
      blob.area = area;
      blob.position.x = (x/points.size());
      blob.position.y = (y/points.size());
      new_blobs.push_back(blob);
    }
  }
}

void BlobTracker::updateClusters() {

  similarities.clear();

  // unset all matched flags for this update
  for(size_t i = 0; i < blobs.size(); ++i) {
    blobs[i].matched = false;
  }

  // update similarities
  for(size_t i = 0; i < new_blobs.size(); ++i) {
    Blob& new_blob = new_blobs[i];

    for(size_t j  = 0; j < blobs.size(); ++j) {

      Blob& old_blob = blobs[j];
      int64_t area_diff = old_blob.area - new_blob.area;
      int64_t darea = area_diff * area_diff;
      int64_t dx = old_blob.position.x - new_blob.position.x;
      int64_t dy = old_blob.position.y - new_blob.position.y;
      int64_t dp = (dx * dx) + (dy * dy);

      Similarity sim;
      sim.new_dx = i;
      sim.old_dx = j;
      sim.area_dist_sq = (darea < 0) ? 0 : darea;
      sim.pos_dist_sq = (dp < 0) ? 0 : dp;

      sim.update();
      similarities[i].push_back(sim);
    }
  }

  // Find matches between similarities
  std::vector<size_t> matched_blobs; 
  std::map<size_t, size_t> matched_with_indices;

  std::map<size_t, std::vector<Similarity> >::iterator it = similarities.begin();
  while(it != similarities.end()) {

    // Sort similarities so the first one is the best match.
    std::vector<Similarity>& sims = it->second;
    std::sort(sims.begin(), sims.end(), SimilaritySorter());

    // Find the best unmatched existing blob
    for(size_t i = 0; i < sims.size(); ++i) {
      Similarity& sim = sims[i];

      // Check if the new blob already found a "partner"
      std::vector<size_t>::iterator matched_it = std::find(matched_blobs.begin(), matched_blobs.end(), sim.new_dx);
      if(matched_it != matched_blobs.end()) {
        continue;
      }

      // Check if the old blob was matched.
      Blob& old_blob = blobs[sim.old_dx];
      if(old_blob.matched) {
        continue;
      }

      //printf("Dist between: %ld and %ld is %lld\n", sim.new_dx, sim.old_dx, sim.value);
      if(sim.value < 1000) {
        old_blob.matched = true;
        matched_blobs.push_back(sim.new_dx);
        matched_with_indices[sim.new_dx] = sim.old_dx;
      }
      break;
    }
    ++it;
  }

  // Create new blobs for all unmatched ones.
  std::vector<size_t> unmatched_blobs;
  for(size_t i = 0; i < new_blobs.size(); ++i) {
  
    std::vector<size_t>::iterator matched_it = std::find(matched_blobs.begin(), matched_blobs.end(), i);

    if(matched_it == matched_blobs.end()) {

      // not matched, created a new blob
      blobs.push_back(new_blobs[i]);
    }
    else {

      // matched, increase age.
      size_t old_dx = matched_with_indices[i];
      Blob& new_blob = new_blobs[i];
      Blob& old_blob = blobs[old_dx];
      old_blob.position = new_blob.position;
      old_blob.area = new_blob.area;
      old_blob.age++;
      old_blob.matched = true;

      if(old_blob.age > 10) {
        old_blob.trail.push_back(old_blob.position);
      }

      while(old_blob.trail.size() > 55) {
        old_blob.trail.erase(old_blob.trail.begin());
      }

      // Update some of the members of the blob.
      old_blob.update();
    }
  }
  
  // All unmatched blobs will get younger
  std::vector<Blob>::iterator bit = blobs.begin();
  while(bit != blobs.end()) {

    Blob& b = *bit;
    if(b.matched == false) {
      b.age--;
    }
    
    if(b.age < -50) {
      bit = blobs.erase(bit);
    }
    else {
      ++bit;
    }
  }

  // tmp print some info
#if 0  
  for(size_t i = 0;i < blobs.size(); ++i) {
    Blob& b = blobs[i];
    printf("Blob: %ld, age: %d\n", i, b.age);
  }
  printf("-\n");
#endif

}
