# Install script for directory: /home/boogie/vlc/contrib/native/aom

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/boogie/vlc/contrib/x86_64-linux-gnu")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aom.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aom_codec.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aom_frame_buffer.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aom_image.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aom_integer.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aom.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aom_decoder.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aomdx.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aomcx.h;/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom/aom_encoder.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/home/boogie/vlc/contrib/x86_64-linux-gnu/include/aom" TYPE FILE FILES
    "/home/boogie/vlc/contrib/native/aom/aom/aom.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aom_codec.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aom_frame_buffer.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aom_image.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aom_integer.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aom.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aom_decoder.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aomdx.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aomcx.h"
    "/home/boogie/vlc/contrib/native/aom/aom/aom_encoder.h"
    )
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/boogie/vlc/contrib/x86_64-linux-gnu/lib/pkgconfig/aom.pc")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/home/boogie/vlc/contrib/x86_64-linux-gnu/lib/pkgconfig" TYPE FILE FILES "/home/boogie/vlc/contrib/native/aom/aom_build/aom.pc")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/boogie/vlc/contrib/x86_64-linux-gnu/lib/libaom.a")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/home/boogie/vlc/contrib/x86_64-linux-gnu/lib" TYPE STATIC_LIBRARY FILES "/home/boogie/vlc/contrib/native/aom/aom_build/libaom.a")
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/boogie/vlc/contrib/native/aom/aom_build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
