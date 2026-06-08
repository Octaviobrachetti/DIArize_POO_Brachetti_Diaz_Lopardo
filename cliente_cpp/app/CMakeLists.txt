find_package(Qt6 REQUIRED COMPONENTS Widgets Network Multimedia Concurrent WebSockets)

qt_standard_project_setup()

qt_add_executable(DIArizeCpp
    src/main.cpp
    src/gui/VentanaPrincipal.cpp
    src/gui/PanelEnVivo.cpp
    src/gui/PantallaCarga.cpp
    src/red/ClienteHTTP.cpp
    src/red/ClienteTranscriptor.cpp
    src/red/ClienteStreaming.cpp
    # Headers con Q_OBJECT: listados explicitamente para AUTOMOC
    include/gui/VentanaPrincipal.h
    include/gui/PanelEnVivo.h
    include/gui/PantallaCarga.h
    include/red/ClienteStreaming.h
    # Recursos (logo embebido)
    resources/recursos.qrc
)

target_include_directories(DIArizeCpp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(DIArizeCpp PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::Concurrent
    Qt6::WebSockets
    nucleo
)

if(WIN32)
    set_target_properties(DIArizeCpp PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
