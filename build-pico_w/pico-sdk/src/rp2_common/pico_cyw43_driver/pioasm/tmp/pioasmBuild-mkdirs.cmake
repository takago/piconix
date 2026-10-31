# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/takago/.pico-sdk/sdk/2.1.1/tools/pioasm"
  "/tmp/PicoNIX/build-pico_w/pioasm"
  "/tmp/PicoNIX/build-pico_w/pioasm-install"
  "/tmp/PicoNIX/build-pico_w/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/tmp"
  "/tmp/PicoNIX/build-pico_w/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp"
  "/tmp/PicoNIX/build-pico_w/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src"
  "/tmp/PicoNIX/build-pico_w/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/tmp/PicoNIX/build-pico_w/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/tmp/PicoNIX/build-pico_w/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
