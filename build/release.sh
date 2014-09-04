#!/bin/sh

if [ ! -d build.release ] ; then 
    mkdir build.release
fi

cd build.release
cmake -DCMAKE_BUILD_TYPE=Release -DOPT_BUILD_DEMO=On -DOPT_BUILD_LIB=On ../
cmake --build . --target install
cd ./../../install/mac-clang-x86_64/bin/
./tracking
