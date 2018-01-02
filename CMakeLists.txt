cmake_minimum_required(VERSION 3.4)
project(libcmk)

# Config
set(CMK_VERSION 0.6)
set(CMK_WITH_GTK TRUE)
set(CMK_WITH_CLUTTER TRUE)

# Build settings
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -DUNUSED=__attribute__\\(\\(__unused__\\)\\)")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")

# Assume the user wants to install Cmk to /usr unless otherwise specified
# with the -DCMAKE_INSTALL_PREFIX=<path> flag to cmake. http://stackoverflow.com/a/16076855
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "default install path" FORCE)
endif()

# Create library target
set(CMK_DEPENDENCIES cairo>=1.15 pango>=1.40)
set(CMK_SOURCES
	#src/cmk-button.c
	src/cmk-timeline.c
	src/cmk-widget.c
	src/cmk-label.c
	src/cmk-palette.c
)

if(CMK_WITH_GTK)
	set(CMK_DEPENDENCIES ${CMK_DEPENDENCIES} gtk+-3.0>=3.22)
	set(CMK_SOURCES ${CMK_SOURCES} src/cmk-gtk.c)
endif()
if(CMK_WITH_CLUTTER)
	set(CMK_DEPENDENCIES ${CMK_DEPENDENCIES} clutter-1.0>=1.26)
	set(CMK_SOURCES ${CMK_SOURCES} src/cmk-clutter.c)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED ${CMK_DEPENDENCIES})

add_library(cmk SHARED ${CMK_SOURCES})
set_target_properties(cmk PROPERTIES SOVERSION ${CMK_VERSION} VERSION ${CMK_VERSION})
target_link_libraries(cmk ${DEPS_LIBRARIES})
target_include_directories(cmk PRIVATE ${DEPS_INCLUDE_DIRS})

add_subdirectory(tests)

# Generate pkg-config file
configure_file(libcmk.pc.in libcmk.pc @ONLY)

# Generate documentation
find_package(GtkDoc 1.25)
if(GTKDOC_FOUND)
	gtk_doc_add_module(cmkdoc
		SOURCE ${CMAKE_SOURCE_DIR}/src
		LDFLAGS ${CMAKE_BINARY_DIR}/libcmk.so
		# GtkDoc cmake package only does header files by default
		# Need to scan source files for signal documentation
		SUFFIXES "h,c"
		DEPENDS cmk
	)
	
	add_custom_target(documentation DEPENDS doc-cmkdoc)
	# Install not needed right now
	#install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmkdoc/html DESTINATION share/doc)
endif(GTKDOC_FOUND)

# Install files
install(TARGETS cmk DESTINATION lib)
install(DIRECTORY src/ DESTINATION include/libcmk/cmk FILES_MATCHING PATTERN "*.h")
install(FILES ${PROJECT_BINARY_DIR}/libcmk.pc DESTINATION lib/pkgconfig)
