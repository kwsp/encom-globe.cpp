if(CONFIG STREQUAL "Release")
  # Bundle Qt frameworks/plugins into the .app and produce a .dmg
  execute_process(
    COMMAND "${MACDEPLOYQT}" "${BUNDLE_DIR}"
      -qmldir=${QMLDIR}
      -verbose=1
      -codesign=-
      -no-strip
      -dmg
  )
else()
  # Write a qt.conf that points to the system Qt installation so the app
  # can find platform plugins and QML modules without macdeployqt.
  file(WRITE "${BUNDLE_DIR}/Contents/Resources/qt.conf"
    "[Paths]\nPlugins = ${QT_PLUGINS_DIR}\nQml2Imports = ${QT_QML_DIR}\n"
  )
  message(STATUS "Wrote qt.conf for ${CONFIG} build")
endif()
