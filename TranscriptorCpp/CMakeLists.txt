cmake_minimum_required(VERSION 3.16)

project(TranscriptorCpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Network)

add_executable(TranscriptorCpp
    main.cpp
    ventanaprincipal.h
    ventanaprincipal.cpp
    motortranscripcion.h
    motortranscripcion.cpp
    asistenteia.h
    asistenteia.cpp
)

target_link_libraries(TranscriptorCpp PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
)
