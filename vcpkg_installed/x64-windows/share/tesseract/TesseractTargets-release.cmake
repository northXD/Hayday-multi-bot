#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Tesseract::libtesseract" for configuration "Release"
set_property(TARGET Tesseract::libtesseract APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Tesseract::libtesseract PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/tesseract55.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/tesseract55.dll"
  )

list(APPEND _cmake_import_check_targets Tesseract::libtesseract )
list(APPEND _cmake_import_check_files_for_Tesseract::libtesseract "${_IMPORT_PREFIX}/lib/tesseract55.lib" "${_IMPORT_PREFIX}/bin/tesseract55.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
