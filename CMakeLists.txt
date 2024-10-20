cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 17)

# Set module path
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})
project(libcamera-demo)

# Find libcamera
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAMERA REQUIRED libcamera)

find_library(LIBCAMERA_LIBRARY libcamera.so REQUIRED)
find_library(LIBCAMERA_BASE_LIBRARY libcamera-base.so REQUIRED)
message(STATUS "Camera include dirs: ${CAMERA_INCLUDE_DIRS}")
message(STATUS "Libcamera library: ${LIBCAMERA_LIBRARY}")
message(STATUS "Libcamera base library: ${LIBCAMERA_BASE_LIBRARY}")

# Find OpenCV
find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
    message("Found OpenCV")
    message("Includes: ${OpenCV_INCLUDE_DIRS}")
endif(OpenCV_FOUND)

# Find libjpeg
find_package(JPEG REQUIRED)
if (JPEG_FOUND)
    message("Found libjpeg")
    message("Includes: ${JPEG_INCLUDE_DIRS}")
    message("Libjpeg library: ${JPEG_LIBRARIES}")
endif(JPEG_FOUND)

# Include directories
include_directories(. "${CAMERA_INCLUDE_DIRS}" "${JPEG_INCLUDE_DIRS}")

# Set libraries
set(LIBCAMERA_LIBRARIES "${LIBCAMERA_LIBRARY}" "${LIBCAMERA_BASE_LIBRARY}")

# Add executable
add_executable(libcamera-demo main.cpp LibCamera.cpp)

# Link libraries
target_link_libraries(libcamera-demo "${LIBCAMERA_LIBRARIES}" ${OpenCV_LIBS} ${JPEG_LIBRARIES})
