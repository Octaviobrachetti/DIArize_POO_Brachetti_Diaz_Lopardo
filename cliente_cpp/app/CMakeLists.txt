find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

qt_add_executable(DIArizeCpp
    src/main.cpp
    src/gui/PanelBase.cpp
    src/gui/PanelTranscripcion.cpp
    src/gui/PanelHistorial.cpp
    src/gui/VentanaPrincipal.cpp
    src/red/ClienteHTTP.cpp
    src/red/ClienteTranscriptor.cpp
    # VentanaPrincipal.h tiene Q_OBJECT: listado explicitamente para AUTOMOC
    include/gui/VentanaPrincipal.h
)

target_include_directories(DIArizeCpp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(DIArizeCpp PRIVATE
    Qt6::Widgets
    Qt6::Network
    nucleo
)

if(WIN32)
    set_target_properties(DIArizeCpp PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
