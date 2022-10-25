cmake_minimum_required(VERSION 3.14)

project(QtCurl LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(USE_QT_5)
  find_package(QT NAMES Qt5 COMPONENTS Core REQUIRED)
else()
  find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)
find_package(CURL REQUIRED)

add_library(QtCurl SHARED
  qcurl.cpp
  qcurl.h
)

target_link_libraries(QtCurl PRIVATE curl Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network curl)

target_compile_definitions(QtCurl PRIVATE METARECORDQT_LIBRARY)
