#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Tesseract::libtesseract" for configuration "Debug"
set_property(TARGET Tesseract::libtesseract APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Tesseract::libtesseract PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/tesseract55d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/tesseract55d.dll"
  )

list(APPEND _cmake_import_check_targets Tesseract::libtesseract )
list(APPEND _cmake_import_check_files_for_Tesseract::libtesseract "${_IMPORT_PREFIX}/debug/lib/tesseract55d.lib" "${_IMPORT_PREFIX}/debug/bin/tesseract55d.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
