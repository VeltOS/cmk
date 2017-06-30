cmake_minimum_required(VERSION 3.4)
project(libcmk)

set(CMK_VERSION 0.3)
set(CMAKE_BUILD_TYPE DEBUG)

# Assume the user wants to install Cmk to /usr unless otherwise specified
# with the -DCMAKE_INSTALL_PREFIX=<path> flag to cmake. http://stackoverflow.com/a/16076855
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "default install path" FORCE)
endif()

configure_file(libcmk.pc.in libcmk.pc @ONLY)

find_package(PkgConfig REQUIRED)
pkg_check_modules(COGL2 REQUIRED cogl2)
pkg_check_modules(CLUTTER REQUIRED clutter-1.0>=1.26)
pkg_check_modules(LIBRSVG REQUIRED librsvg-2.0)

add_library(cmk SHARED
	src/cmk-button.c
	src/cmk-icon.c
	src/cmk-icon-loader.c
	src/cmk-label.c
	src/cmk-scroll-box.c
	src/cmk-shadow.c
	src/cmk-util.c
	src/cmk-widget.c
)
set_target_properties(cmk PROPERTIES SOVERSION ${CMK_VERSION} VERSION ${CMK_VERSION})
target_link_libraries(cmk
	${CLUTTER_LIBRARIES}
	${LIBRSVG_LIBRARIES}
	${COGL2_LIBRARIES}
)
target_include_directories(cmk PRIVATE
	${CLUTTER_INCLUDE_DIRS}
	${LIBRSVG_INCLUDE_DIRS}
)

install(TARGETS cmk DESTINATION lib)
install(DIRECTORY src/ DESTINATION include/libcmk FILES_MATCHING PATTERN "*.h")
install(FILES ${PROJECT_BINARY_DIR}/libcmk.pc DESTINATION lib/pkgconfig)
