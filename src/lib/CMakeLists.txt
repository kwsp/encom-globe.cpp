set(LIB_NAME EncomGlobeLib)

qt_add_library(${LIB_NAME} STATIC)

set_target_properties(${LIB_NAME} PROPERTIES
    AUTOMOC ON
)

target_sources(${LIB_NAME} PRIVATE
    Globe.h Globe.cpp
    GlobeRenderer.h GlobeRenderer.cpp
    SatelliteRenderer.h SatelliteRenderer.cpp
    PinRenderer.h PinRenderer.cpp
    MarkerRenderer.h MarkerRenderer.cpp
    SmokeRenderer.h SmokeRenderer.cpp
    IntroLinesRenderer.h IntroLinesRenderer.cpp
    Utils.h Utils.cpp
)

qt_add_qml_module(${LIB_NAME}
    URI EncomGlobe
    VERSION 1.0
    OUTPUT_DIRECTORY EncomGlobe
    RESOURCE_PREFIX "/qt/qml"
    RESOURCES
        grid.json
)

qt_add_shaders(${LIB_NAME} "shaders"
    PRECOMPILE
    PREFIX "/shaders"
    GLSL "440"
    SPIRV "100"
    HLSL 50
    MSL 12
    FILES
        shaders/globe.vert
        shaders/globe.frag
        shaders/satellite.vert
        shaders/satellite.frag
        shaders/pin.vert
        shaders/pin.frag
        shaders/pintop.vert
        shaders/pintop.frag
        shaders/smoke.vert
        shaders/smoke.frag
        shaders/introlines.vert
        shaders/introlines.frag
)

target_link_libraries(${LIB_NAME} PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

target_link_libraries(${LIB_NAME} PRIVATE
    Qt6::GuiPrivate  # For RHI headers
    Qt6::ShaderTools
)

target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
