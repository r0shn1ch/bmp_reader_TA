cmake_minimum_required(VERSION 3.16)

project(effective_mobile LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(effective_mobile main.cpp
    bmp_image.h bmp_image.cpp)

include(GNUInstallDirs)
install(TARGETS effective_mobile
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
