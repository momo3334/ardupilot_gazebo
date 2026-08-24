cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(ardupilot_gazebo)

# --------------------------------------------------------------------------- #
# Options.

option(BUILD_GST_CAMERA_PLUGIN
  "Build GstCameraPlugin. Requires GStreamer and OpenCV." ON)

# --------------------------------------------------------------------------- #
# If ament_cmake is found build as an ament package, otherwise ignore.
# This is so the system may be built for Gazebo only, if ROS is not available.
find_package(ament_cmake QUIET)
if(${ament_cmake_FOUND})
  message("Building ${PROJECT_NAME} as an `ament_cmake` project.")
endif()

# --------------------------------------------------------------------------- #
# Compile as C++17, the minimum required by all supported Gazebo releases.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------------- #
# Find gz-sim and dependencies.

if("$ENV{GZ_VERSION}" STREQUAL "harmonic" OR NOT DEFINED "ENV{GZ_VERSION}")
  # Harmonic (default)
  find_package(gz-cmake3 REQUIRED)
  set(GZ_CMAKE_VER ${gz-cmake3_VERSION_MAJOR})

  gz_find_package(gz-common5 REQUIRED)
  set(GZ_COMMON_VER ${gz-common5_VERSION_MAJOR})

  gz_find_package(gz-rendering8 REQUIRED)
  set(GZ_RENDERING_VER ${gz-rendering8_VERSION_MAJOR})

  gz_find_package(gz-sim8 REQUIRED)
  set(GZ_SIM_VER ${gz-sim8_VERSION_MAJOR})

  message(STATUS "Compiling against Gazebo Harmonic")
elseif("$ENV{GZ_VERSION}" STREQUAL "ionic")
  # Ionic
  find_package(gz-cmake4 REQUIRED)
  set(GZ_CMAKE_VER ${gz-cmake4_VERSION_MAJOR})

  gz_find_package(gz-common6 REQUIRED)
  set(GZ_COMMON_VER ${gz-common6_VERSION_MAJOR})

  gz_find_package(gz-rendering9 REQUIRED)
  set(GZ_RENDERING_VER ${gz-rendering9_VERSION_MAJOR})
  
  gz_find_package(gz-sim9 REQUIRED)
  set(GZ_SIM_VER ${gz-sim9_VERSION_MAJOR})

  message(STATUS "Compiling against Gazebo Ionic")
elseif("$ENV{GZ_VERSION}" STREQUAL "jetty")
  # Jetty
  find_package(gz-cmake REQUIRED)
  set(GZ_CMAKE_VER "")

  gz_find_package(gz-common REQUIRED)
  set(GZ_COMMON_VER "")

  gz_find_package(gz-rendering REQUIRED)
  set(GZ_RENDERING_VER "")
  
  gz_find_package(gz-sim REQUIRED)
  set(GZ_SIM_VER "")

  message(STATUS "Compiling against Gazebo Jetty")
elseif("$ENV{GZ_VERSION}" STREQUAL "garden")
  # Garden
  find_package(gz-cmake3 REQUIRED)
  set(GZ_CMAKE_VER ${gz-cmake3_VERSION_MAJOR})

  gz_find_package(gz-common5 REQUIRED)
  set(GZ_COMMON_VER ${gz-common5_VERSION_MAJOR})

  gz_find_package(gz-rendering7 REQUIRED)
  set(GZ_RENDERING_VER ${gz-rendering7_VERSION_MAJOR})
  
  gz_find_package(gz-sim7 REQUIRED)
  set(GZ_SIM_VER ${gz-sim7_VERSION_MAJOR})

  message(STATUS "Compiling against Gazebo Garden")
else()  
  message(FATAL_ERROR "Unsupported GZ_VERSION: $ENV{GZ_VERSION}")
endif()

# --------------------------------------------------------------------------- #
find_package(RapidJSON REQUIRED)
find_package(Threads REQUIRED)

if(BUILD_GST_CAMERA_PLUGIN)
  find_package(PkgConfig REQUIRED)
  find_package(OpenCV REQUIRED)
  pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)
endif()

# --------------------------------------------------------------------------- #
# Settings shared by every plugin.

function(configure_ardupilot_plugin target)
  target_include_directories(${target} PRIVATE include)
  target_link_libraries(${target} PRIVATE Threads::Threads)

  if(WIN32)
    # NOMINMAX stops windows.h defining min/max macros that break std::min and
    # std::max. WIN32_LEAN_AND_MEAN stops it pulling in the Winsock 1.1
    # headers, which clash with the winsock2.h that SocketUDP.hh needs.
    target_compile_definitions(${target} PRIVATE
      NOMINMAX
      WIN32_LEAN_AND_MEAN
      _USE_MATH_DEFINES
      _WINSOCK_DEPRECATED_NO_WARNINGS
      _CRT_SECURE_NO_WARNINGS
    )
  endif()
endfunction()

# --------------------------------------------------------------------------- #
# Build plugin.

add_library(ArduPilotPlugin
    SHARED
    src/ArduPilotPlugin.cc
    src/SocketUDP.cc
    src/Util.cc
)
configure_ardupilot_plugin(ArduPilotPlugin)
target_include_directories(ArduPilotPlugin PRIVATE
  ${RapidJSON_INCLUDE_DIRS}
)
target_link_libraries(ArduPilotPlugin PRIVATE
  gz-sim${GZ_SIM_VER}::gz-sim${GZ_SIM_VER}
)
if(WIN32)
  # Winsock, for SocketUDP.
  target_link_libraries(ArduPilotPlugin PRIVATE ws2_32)
endif()

add_library(ParachutePlugin
  SHARED
  src/ParachutePlugin.cc
)
configure_ardupilot_plugin(ParachutePlugin)
target_link_libraries(ParachutePlugin PRIVATE
  gz-sim${GZ_SIM_VER}::gz-sim${GZ_SIM_VER}
)

add_library(CameraZoomPlugin
  SHARED
  src/CameraZoomPlugin.cc
)
configure_ardupilot_plugin(CameraZoomPlugin)
target_link_libraries(CameraZoomPlugin PRIVATE
  gz-common${GZ_COMMON_VER}::gz-common${GZ_COMMON_VER}
  gz-rendering${GZ_RENDERING_VER}::gz-rendering${GZ_RENDERING_VER}
  gz-sim${GZ_SIM_VER}::gz-sim${GZ_SIM_VER}
)

set(ARDUPILOT_GAZEBO_PLUGINS
  ArduPilotPlugin
  ParachutePlugin
  CameraZoomPlugin
)

if(BUILD_GST_CAMERA_PLUGIN)
  add_library(GstCameraPlugin
    SHARED
    src/GstCameraPlugin.cc
  )
  configure_ardupilot_plugin(GstCameraPlugin)
  target_include_directories(GstCameraPlugin PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${GST_INCLUDE_DIRS}
  )
  target_link_directories(GstCameraPlugin PRIVATE
    ${GST_LIBRARY_DIRS}
  )
  target_link_libraries(GstCameraPlugin PRIVATE
    gz-sim${GZ_SIM_VER}::gz-sim${GZ_SIM_VER}
    ${OpenCV_LIBS}
    ${GST_LINK_LIBRARIES}
  )
  list(APPEND ARDUPILOT_GAZEBO_PLUGINS GstCameraPlugin)
endif()

# --------------------------------------------------------------------------- #
# Install.

install(
  TARGETS
  ${ARDUPILOT_GAZEBO_PLUGINS}
  RUNTIME DESTINATION lib/${PROJECT_NAME}
  LIBRARY DESTINATION lib/${PROJECT_NAME}
  ARCHIVE DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY
  config/
  DESTINATION share/${PROJECT_NAME}/config
)

install(
  DIRECTORY
  models/
  DESTINATION share/${PROJECT_NAME}/models
)

install(
  DIRECTORY
  worlds/
  DESTINATION share/${PROJECT_NAME}/worlds
)

# --------------------------------------------------------------------------- #
# Register as an ament package if ament_cmake is available.
if(${ament_cmake_FOUND})
  ament_environment_hooks(
    "${CMAKE_CURRENT_SOURCE_DIR}/hooks/${PROJECT_NAME}.dsv.in")
  ament_environment_hooks(
    "${CMAKE_CURRENT_SOURCE_DIR}/hooks/${PROJECT_NAME}.sh.in")

  ament_package()
endif()
