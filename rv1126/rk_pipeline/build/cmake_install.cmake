# Install script for directory: /home/huangjd/workdir/jiankong/rk_pipeline

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
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

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo/luckfox_pico_rtsp_opencv" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo/luckfox_pico_rtsp_opencv")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo/luckfox_pico_rtsp_opencv"
         RPATH "$ORIGIN/lib")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo/luckfox_pico_rtsp_opencv")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo" TYPE EXECUTABLE FILES "/home/huangjd/workdir/jiankong/rk_pipeline/build/luckfox_pico_rtsp_opencv")
  if(EXISTS "$ENV{DESTDIR}/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo/luckfox_pico_rtsp_opencv" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo/luckfox_pico_rtsp_opencv")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo/luckfox_pico_rtsp_opencv"
         OLD_RPATH "/home/huangjd/workdir/jiankong/rk_pipeline/3rdparty/rknpu2/Linux/armhf-uclibc:/home/huangjd/workdir/jiankong/rk_pipeline/lib:"
         NEW_RPATH "$ORIGIN/lib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/huangjd/workdir/jiankong/rk_pipeline/install/luckfox_pico_rtsp_opencv_demo/luckfox_pico_rtsp_opencv")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/huangjd/workdir/jiankong/rk_pipeline/build/3rdparty.out/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/huangjd/workdir/jiankong/rk_pipeline/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
