PROJECT(scenecutremover)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

FILE(GLOB SOURCES src/*.c src/*.cpp)
ADD_EXECUTABLE(scenecutremover ${SOURCES})
INSTALL(TARGETS scenecutremover RUNTIME DESTINATION bin)

set(CMAKE_CXX_STANDARD 14) #setting C++ 14 standard
find_package(PkgConfig) #finding pkg-config is a helper tool

#using pkg-config to getting Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(OPENCV REQUIRED opencv4)

#including GStreamer header files directory
include_directories(
        ${GLIB_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
        ${OPENCV_INCLUDE_DIRS}
)

#linking GStreamer library directory
link_directories(
        ${GLIB_LIBRARY_DIRS}
        ${GSTREAMER_LIBRARY_DIRS}
	${OPENCV_LIBRARY_DIRS}
)

#linking Gstreamer library with target executable
target_link_libraries(${PROJECT_NAME} ${OPENCV_LIBRARIES} gstapp-1.0 ${GSTREAMER_LIBRARIES} )
