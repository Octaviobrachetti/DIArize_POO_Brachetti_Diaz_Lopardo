add_library(nucleo STATIC
    src/Hablante.cpp
    src/Segmento.cpp
    src/Transcripcion.cpp
)

# PUBLIC: quien linkee nucleo ve automaticamente su include/
target_include_directories(nucleo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(nucleo PUBLIC cxx_std_17)
