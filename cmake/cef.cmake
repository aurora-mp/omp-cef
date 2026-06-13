if(NOT DEFINED CEF_VERSION)
	set(CEF_VERSION "143.0.14+gdd46a37+chromium-143.0.7499.193")
	message(STATUS "CEF_VERSION is not defined, using default version: ${CEF_VERSION}")
endif()

set(CEF_ARCH "windows32")
set(CEF_DOWNLOAD_BASE "https://cef-builds.spotifycdn.com")
set(CEF_DEST_PARENT "${CMAKE_SOURCE_DIR}/deps/cef")
set(CEF_DIR_NAME "cef_binary_${CEF_VERSION}_${CEF_ARCH}")
set(CEF_ROOT_DIR "${CEF_DEST_PARENT}/${CEF_DIR_NAME}")

if(EXISTS "${CEF_ROOT_DIR}/include/cef_app.h")
	message(STATUS "CEF already present in ${CEF_ROOT_DIR}")
else()
	if(EXISTS "${CEF_ROOT_DIR}")
		message(STATUS "Removing an incomplete CEF in ${CEF_ROOT_DIR}")
		file(REMOVE_RECURSE "${CEF_ROOT_DIR}")
	endif()

  	file(MAKE_DIRECTORY "${CEF_DEST_PARENT}")

	set(CEF_ARCHIVE_NAME "cef_binary_${CEF_VERSION}_${CEF_ARCH}.tar.bz2")
	set(CEF_URL "${CEF_DOWNLOAD_BASE}/${CEF_ARCHIVE_NAME}")

	set(CEF_ARCHIVE_CACHE_DIR "${CMAKE_BINARY_DIR}/_cef_downloads")
	file(MAKE_DIRECTORY "${CEF_ARCHIVE_CACHE_DIR}")
	set(CEF_ARCHIVE_PATH "${CEF_ARCHIVE_CACHE_DIR}/${CEF_ARCHIVE_NAME}")

	message(STATUS "Downloading CEF ${CEF_VERSION} (${CEF_ARCH}) from ${CEF_URL}")

	file(DOWNLOAD
		"${CEF_URL}"
		"${CEF_ARCHIVE_PATH}"
		SHOW_PROGRESS
		STATUS _dl_status
		TIMEOUT 300
	)

	list(GET _dl_status 0 _dl_code)
	if(NOT _dl_code EQUAL 0)
		list(GET _dl_status 1 _dl_msg)
		message(FATAL_ERROR "Failed to download CEF: ${_dl_msg}")
	endif()

	message(STATUS "Extracting ${CEF_ARCHIVE_PATH} into ${CEF_DEST_PARENT}")
	file(ARCHIVE_EXTRACT INPUT "${CEF_ARCHIVE_PATH}" DESTINATION "${CEF_DEST_PARENT}")

	if(NOT EXISTS "${CEF_ROOT_DIR}/include/cef_app.h")
		message(FATAL_ERROR "Extraction failed or invalid archive: ${CEF_ROOT_DIR}/include/cef_app.h not found")
	endif()
endif()

set(CEF_ROOT "${CEF_ROOT_DIR}" CACHE PATH "Path to the downloaded CEF distribution" FORCE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
set(CEF_RUNTIME_LIBRARY_FLAG "/MT" CACHE STRING "Use /MT for CEF" FORCE)
set(USE_SANDBOX OFF CACHE BOOL "Disable CEF sandbox to avoid debug level conflicts")

find_package(CEF REQUIRED)

PRINT_CEF_CONFIG()