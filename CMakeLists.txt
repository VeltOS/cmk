cmake_minimum_required(VERSION 3.4)
project(libcmk)

# Assume the user wants to install Graphene to /usr unless otherwise specified
# with the -DCMAKE_INSTALL_PREFIX=<path> flag to cmake. http://stackoverflow.com/a/16076855
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "default install path" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(CLUTTER REQUIRED clutter-1.0>=1.26)
pkg_check_modules(LIBRSVG REQUIRED librsvg-2.0)

add_library(cmk SHARED
	src/button.c
	src/shadow.c
	src/cmk-widget.c
	src/cmk-icon-loader.c
	src/cmk-icon.c
	src/cmk-label.c
	src/cmk-scroll-box.c
)
set_target_properties(cmk PROPERTIES SOVERSION 0.1.0 VERSION 0.1.0)
target_link_libraries(cmk
	${CLUTTER_LIBRARIES}
	${LIBRSVG_LIBRARIES}
)
target_include_directories(cmk PRIVATE
	${LIBRSVG_INCLUDE_DIRS}
	${CLUTTER_INCLUDE_DIRS}
)

install(TARGETS cmk DESTINATION lib)
install(DIRECTORY src/ DESTINATION include/libcmk FILES_MATCHING PATTERN "*.h")
