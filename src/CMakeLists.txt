find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools QuickControls2)
find_package(Qt6 COMPONENTS GuiPrivate)  # For RHI headers
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(${PROJECT_NAME}
  main.cpp
  Utils.h Utils.cpp
  Globe.h Globe.cpp
  GlobeRenderer.h GlobeRenderer.cpp
  SatelliteRenderer.h SatelliteRenderer.cpp
  PinRenderer.h PinRenderer.cpp
  MarkerRenderer.h MarkerRenderer.cpp
  SmokeRenderer.h SmokeRenderer.cpp
  IntroLinesRenderer.h IntroLinesRenderer.cpp
)

qt_add_qml_module(${PROJECT_NAME}
  URI ${PROJECT_NAME}
  VERSION ${CMAKE_PROJECT_VERSION}
  QML_FILES
    Main.qml
  RESOURCES
    grid.json
)

qt_add_shaders(${PROJECT_NAME} "shaders"
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

configure_file(config.h.in config.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::GuiPrivate  # For RHI headers
  Qt6::Qml
  Qt6::Quick
  Qt6::ShaderTools
  Qt6::QuickControls2
)

# Generate .qmlls.ini
find_program(_qmake NAMES qmake6 qmake HINTS "${Qt6_DIR}/../../../bin")
if(_qmake)
  execute_process(
    COMMAND "${_qmake}" -query QT_INSTALL_QML
    OUTPUT_VARIABLE _qt_qml_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  file(RELATIVE_PATH _qmlls_build_dir "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
  file(WRITE "${CMAKE_SOURCE_DIR}/.qmlls.ini"
    "[General]\nbuildDir=${_qmlls_build_dir}\nimportPaths=${_qt_qml_dir}\n"
  )
else()
  message(WARNING "qmake not found. .qmlls.ini will not be generated, which may affect QML language features in some IDEs.")
endif()

# App Bundle
if (WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
  find_program(WINDEPLOYQT NAMES windeployqt6 windeployqt HINTS "${Qt6_DIR}/../../../bin" REQUIRED)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${WINDEPLOYQT}
      "$<IF:$<CONFIG:Debug>,--debug,--release>"
      --qmldir ${CMAKE_CURRENT_SOURCE_DIR}
      --verbose 1
      "$<TARGET_FILE:${PROJECT_NAME}>"
  )

elseif(APPLE)
  cmake_path(GET Qt6_DIR PARENT_PATH _qt_cmake_dir)
  cmake_path(GET _qt_cmake_dir PARENT_PATH _qt_lib_dir)

  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST cmake/MacOSXBundleInfo.plist.in
    BUILD_RPATH "${_qt_lib_dir}"
  )

  get_target_property(QT_BIN_DIR Qt::qmake IMPORTED_LOCATION)
  find_program(MACDEPLOYQT NAMES macdeployqt HINTS "${QT_BIN_DIR}")

  if(_qmake)
    execute_process(
      COMMAND "${_qmake}" -query QT_INSTALL_PLUGINS
      OUTPUT_VARIABLE _qt_plugins_dir
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
          -DMACDEPLOYQT:FILEPATH=${MACDEPLOYQT}
          -DBUNDLE_DIR:PATH=$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>
          -DQMLDIR:PATH=${CMAKE_CURRENT_SOURCE_DIR}
          -DQT_PLUGINS_DIR:PATH=${_qt_plugins_dir}
          -DQT_QML_DIR:PATH=${_qt_qml_dir}
          -DCONFIG:STRING=$<CONFIG>
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunMacDeployQt.cmake
      COMMENT "Deploying Qt libraries"
      VERBATIM
  )

endif()
