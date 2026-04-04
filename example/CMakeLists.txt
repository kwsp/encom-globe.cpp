set(PROJECT_NAME EncomGlobeExample)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(${PROJECT_NAME}
    main.cpp
)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    EncomGlobe
    EncomGlobeplugin
)

# App Bundle properties
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Encom Globe"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.encom.globe"
    )
endif()

# Deployment (Commented out to avoid RPATH/abort issues during development)
# if(WIN32)
# ...
# elseif(APPLE)
# ...
# endif()

