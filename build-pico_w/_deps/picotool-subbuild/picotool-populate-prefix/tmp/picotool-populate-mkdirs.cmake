# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/tmp/PicoNIX/build-pico_w/_deps/picotool-src"
  "/tmp/PicoNIX/build-pico_w/_deps/picotool-build"
  "/tmp/PicoNIX/build-pico_w/_deps/picotool-subbuild/picotool-populate-prefix"
  "/tmp/PicoNIX/build-pico_w/_deps/picotool-subbuild/picotool-populate-prefix/tmp"
  "/tmp/PicoNIX/build-pico_w/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp"
  "/tmp/PicoNIX/build-pico_w/_deps/picotool-subbuild/picotool-populate-prefix/src"
  "/tmp/PicoNIX/build-pico_w/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/tmp/PicoNIX/build-pico_w/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/tmp/PicoNIX/build-pico_w/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
