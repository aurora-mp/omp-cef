include(CMakeParseArguments)

function(add_win_version_resource TARGET)
	if (NOT WIN32)
		return()
	endif()

	set(options USE_MANIFEST)
	set(values VERSION COMPANY PRODUCT_NAME FILE_DESC INTERNAL_NAME COPYRIGHT)
	cmake_parse_arguments(VER "${options}" "${values}" "" ${ARGN})

	get_target_property(_type ${TARGET} TYPE)
	if (_type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "MODULE_LIBRARY")
		set(FILETYPE 2)
		set(_EXT "dll")
	elseif (_type STREQUAL "EXECUTABLE")
		set(FILETYPE 1)
		set(_EXT "exe")
	else()
		message(STATUS "[version.rc] ${TARGET}: type ${_type} ignore")
		return()
	endif()

	if (NOT VER_VERSION)
		set(VER_VERSION "1.0.0.0")
	endif()

	string(REPLACE "." ";" _v ${VER_VERSION})
	list(GET _v 0 VER_MAJOR)
	list(GET _v 1 VER_MINOR)
	list(GET _v 2 VER_PATCH)
	list(GET _v 3 VER_BUILD)
	set(VER_STR "${VER_VERSION}")

	if (NOT VER_COMPANY)
		set(VER_COMPANY "aurora-mp")
	endif()

	if (NOT VER_PRODUCT_NAME)
		set(VER_PRODUCT_NAME "${TARGET}")
	endif()

	if (NOT VER_FILE_DESC)
		set(VER_FILE_DESC "${TARGET}")
	endif()

	if (NOT VER_INTERNAL_NAME)
		set(VER_INTERNAL_NAME "${TARGET}")
	endif()

	if (NOT VER_COPYRIGHT)
		set(VER_COPYRIGHT "Copyright (c) 2025 Aurora Team")
	endif()

  	set(ORIGINAL_NAME "${VER_INTERNAL_NAME}.${_EXT}")
	set(PRODUCT_NAME "${VER_PRODUCT_NAME}")
	set(COMPANY "${VER_COMPANY}")
	set(FILE_DESC "${VER_FILE_DESC}")
	set(INTERNAL_NAME "${VER_INTERNAL_NAME}")
	set(COPYRIGHT "${VER_COPYRIGHT}")

	set(_out "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_version.rc")
	configure_file("${CMAKE_SOURCE_DIR}/cmake/version.rc.in" "${_out}" @ONLY)

  	target_sources(${TARGET} PRIVATE "${_out}")
endfunction()