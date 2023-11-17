cmake_minimum_required(VERSION 3.14)

project(QtCurl
  LANGUAGES CXX
  VERSION 1.0.0
  DESCRIPTION "Simple wrapper around libcurl for Qt")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)
find_package(CURL REQUIRED)

set(TARGET_NAME Qt${QT_VERSION_MAJOR}Curl)
message("Building ${PROJECT_NAME} with Qt${QT_VERSION_MAJOR}")

add_library(${TARGET_NAME} SHARED
  qcurl.cpp
  qcurl.h
)

target_link_libraries(${TARGET_NAME} PRIVATE curl Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network curl)

target_compile_definitions(${TARGET_NAME} PRIVATE QTCURL_LIBRARY)

configure_file(QtCurl.pc.in ${TARGET_NAME}.pc @ONLY)

install(FILES qcurl.h DESTINATION include)
install(TARGETS ${TARGET_NAME} DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/${TARGET_NAME}.pc DESTINATION lib/pkgconfig)
