#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "leptonica" for configuration "Debug"
set_property(TARGET leptonica APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(leptonica PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/leptonica-1.85.0d.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG "openjp2;WebP::webp;WebP::libwebpmux"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/leptonica-1.85.0d.dll"
  )

list(APPEND _cmake_import_check_targets leptonica )
list(APPEND _cmake_import_check_files_for_leptonica "${_IMPORT_PREFIX}/debug/lib/leptonica-1.85.0d.lib" "${_IMPORT_PREFIX}/debug/bin/leptonica-1.85.0d.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
