cmake_minimum_required(VERSION 3.4)
project(libcmk)

set(CMK_VERSION 0.1.0)
set(CMAKE_BUILD_TYPE DEBUG)

# Assume the user wants to install Graphene to /usr unless otherwise specified
# with the -DCMAKE_INSTALL_PREFIX=<path> flag to cmake. http://stackoverflow.com/a/16076855
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "default install path" FORCE)
endif()

configure_file(libcmk.pc.in libcmk.pc @ONLY)

find_package(PkgConfig REQUIRED)
#pkg_check_modules(CLUTTER REQUIRED clutter-1.0>=1.26)
pkg_check_modules(LIBRSVG REQUIRED librsvg-2.0)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.44.0)
pkg_check_modules(COGL REQUIRED cogl-1.0>=1.21.2)
pkg_check_modules(COGL_PANGO REQUIRED cogl-pango-1.0)
pkg_check_modules(COGL_PATH REQUIRED cogl-path-1.0)
pkg_check_modules(PANGO REQUIRED pango>=1.30)
pkg_check_modules(ATK REQUIRED atk>=2.5.3)
pkg_check_modules(JSONGLIB REQUIRED json-glib-1.0>=0.12.0)

add_subdirectory(cmk-clutter)

add_library(cmk SHARED
	src/button.c
	src/shadow.c
	src/cmk-main.c
	#src/cmk-wayland-client.c
	src/cmk-window.c
	src/cmk-widget.c
	src/cmk-icon-loader.c
	src/cmk-icon.c
	src/cmk-label.c
	src/cmk-scroll-box.c
)
set_target_properties(cmk PROPERTIES SOVERSION ${CMK_VERSION} VERSION ${CMK_VERSION})
target_link_libraries(cmk
	#${CLUTTER_LIBRARIES}
	${LIBRSVG_LIBRARIES}
	-Wl,--whole-archive cmk-clutter -Wl,--no-whole-archive
	input
	${GLIB_LIBARIES}
	${COGL_LIBRARIES}
	${COGL_PANGO_LIBRARIES}
	${COGL_PATH_LIBRARIES}
	${PANGO_LIBRARIES}
	${ATK_LIBRARIES}
	${JSONGLIB_LIBRARIES}
)
target_include_directories(cmk PRIVATE
	#${CLUTTER_INCLUDE_DIRS}
	${LIBRSVG_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/cmk-clutter
	${GLIB_INCLUDE_DIRS}
	${COGL_INCLUDE_DIRS}
	${COGL_PANGO_INCLUDE_DIRS}
	${COGL_PATH_INCLUDE_DIRS}
	${PANGO_INCLUDE_DIRS}
	${ATK_INCLUDE_DIRS}
	${JSONGLIB_INCLUDE_DIRS}
)

install(TARGETS cmk DESTINATION lib)
install(DIRECTORY src/ DESTINATION include/libcmk FILES_MATCHING PATTERN "*.h")
install(FILES ${PROJECT_BINARY_DIR}/libcmk.pc DESTINATION lib/pkgconfig)
