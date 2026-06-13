#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "kcp::kcp" for configuration "MinSizeRel"
set_property(TARGET kcp::kcp APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(kcp::kcp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/kcp.lib"
  )

list(APPEND _cmake_import_check_targets kcp::kcp )
list(APPEND _cmake_import_check_files_for_kcp::kcp "${_IMPORT_PREFIX}/lib/kcp.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
