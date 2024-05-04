# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/sohamc1909/esp/esp-idf-v5-2-1/components/bootloader/subproject"
  "/home/sohamc1909/esp/Projects/esp-drts/esp-drts-proc1/build/bootloader"
  "/home/sohamc1909/esp/Projects/esp-drts/esp-drts-proc1/build/bootloader-prefix"
  "/home/sohamc1909/esp/Projects/esp-drts/esp-drts-proc1/build/bootloader-prefix/tmp"
  "/home/sohamc1909/esp/Projects/esp-drts/esp-drts-proc1/build/bootloader-prefix/src/bootloader-stamp"
  "/home/sohamc1909/esp/Projects/esp-drts/esp-drts-proc1/build/bootloader-prefix/src"
  "/home/sohamc1909/esp/Projects/esp-drts/esp-drts-proc1/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/sohamc1909/esp/Projects/esp-drts/esp-drts-proc1/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/sohamc1909/esp/Projects/esp-drts/esp-drts-proc1/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
