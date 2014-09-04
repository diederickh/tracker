cmake_minimum_required(VERSION 2.8)

option(OPT_BUILD_DEMO "Build demo" OFF)
option(OPT_BUILD_LIB "Build lib" OFF)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
  project(tracking_debug)
else()
  project(tracking)
endif()

include(Triplet.cmake)

set(bd ${CMAKE_CURRENT_LIST_DIR}/../)

include_directories(
  ${bd}/src/
  ${bd}/include/
  ${extern_include_dir}
  ${bd}/shared/tinylib/src/
)

find_library(fr_corefoundation CoreFoundation)
find_library(fr_avfoundation AVFoundation)
find_library(fr_cocoa Cocoa)
find_library(fr_opengl OpenGL)
find_library(fr_opencl OpenCL)
find_library(fr_iokit IOKit)
find_library(fr_corevideo CoreVideo)
find_library(fr_coremedia CoreMedia)

set(tracker_libs
  ${extern_lib_dir}libglfw3.a
  ${extern_lib_dir}libpng.a
  ${extern_lib_dir}libjpeg.a
  ${extern_lib_dir}libvideocapture.a
  ${extern_lib_dir}libopencv_core.a
  ${extern_lib_dir}libopencv_imgproc.a
  ${extern_lib_dir}libopencv_video.a
  ${extern_lib_dir}libopencv_features2d.a
  ${fr_corefoundation}
  ${fr_avfoundation}
  ${fr_cocoa}
  ${fr_opengl}
  ${fr_iokit}  
  ${fr_corevideo}
  ${fr_coremedia}
  ${fr_opencl}
  -lz
)

set(tracker_source_files
  ${bd}/src/tracker/Tracker.cpp
  ${bd}/src/tracker/BackgroundBuffer.cpp
  ${bd}/src/tracker/ErodeDilateThreshold.cpp
  ${bd}/src/tracker/Blur.cpp
  ${bd}/src/tracker/BlobTracker.cpp
)

set(tracker_include_files
  ${bd}/include/tracker/BackgroundBuffer.h
  ${bd}/include/tracker/Blur.h
  ${bd}/include/tracker/BlobTracker.h
  ${bd}/include/tracker/ErodeDilateThreshold.h
  ${bd}/include/tracker/Tracker.h
)

if (OPT_BUILD_LIB)
  add_library(
    ${CMAKE_PROJECT_NAME}
    ${tracker_source_files}
    )
  install(TARGETS ${CMAKE_PROJECT_NAME} ARCHIVE DESTINATION lib)
  install(FILES ${tracker_include_files} DESTINATION include/tracker/)
endif()

if (OPT_BUILD_DEMO)
  add_executable(demo ${bd}/src/main.cpp)
  target_link_libraries(demo ${tracker_libs} ${CMAKE_PROJECT_NAME})
  install(TARGETS demo DESTINATION bin)
endif()
