cmake_minimum_required(VERSION 3.21)

project(DIArize VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UTF-8 en MSVC (necesario para comentarios y strings en español)
if(MSVC)
    add_compile_options(/utf-8 /W3)
endif()

# MOC/UIC/RCC automáticos para todos los targets Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(nucleo)
add_subdirectory(app)
