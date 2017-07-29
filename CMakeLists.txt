cmake_minimum_required(VERSION 3.4)
project(libcmk)

set(CMK_VERSION 0.4)
set(CMAKE_BUILD_TYPE DEBUG)

# Assume the user wants to install Cmk to /usr unless otherwise specified
# with the -DCMAKE_INSTALL_PREFIX=<path> flag to cmake. http://stackoverflow.com/a/16076855
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "default install path" FORCE)
endif()

configure_file(libcmk.pc.in libcmk.pc @ONLY)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CLUTTERDEPS REQUIRED cogl-1.0>=1.21.2 cogl-path-1.0 cairo-gobject>=1.14.0 gio-2.0>=2.44.0 atk>=2.5.3 pangocairo>=1.30 cogl-pango-1.0 json-glib-1.0>=0.12.0 gdk-3.0 wayland-cursor wayland-client xkbcommon x11 xext xdamage xcomposite>=0.4 xi pangoft2 gdk-pixbuf-2.0 libudev>=136 libinput>=0.19.0)
pkg_check_modules(LIBRSVG REQUIRED librsvg-2.0)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

ExternalProject_Add(cmk-clutter
	PREFIX ${CMAKE_SOURCE_DIR}/cmk-clutter/
	#GIT_SUBMODULES cmk-clutter
	DOWNLOAD_COMMAND git submodule update --init --recursive
	# Without these, Clutter doesn't compile for me. The removal of the --prototypes thing is just because I have an outdated glib version because Arch is behind, but the first one, I have no idea how the Clutter devs compile with this
	PATCH_COMMAND sed -i "/-Werror=missing-declarations/d" ${CMAKE_SOURCE_DIR}/cmk-clutter/configure.ac && sed -i "/--prototypes/d" ${CMAKE_SOURCE_DIR}/cmk-clutter/build/autotools/Makefile.am.marshal
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/cmk-clutter/
	BINARY_DIR ${CMAKE_SOURCE_DIR}/cmk-clutter/
	CONFIGURE_COMMAND CFLAGS=-fPIC ${CMAKE_SOURCE_DIR}/cmk-clutter/autogen.sh --enable-static=yes --enable-shared=no --enable-introspection=no --disable-examples
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
)

# Apparently there's no way to make ExternalProject run
# BUILD_COMMAND every time. Because that makes sense.
add_custom_target(cmk-clutter-build
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmk-clutter/
	COMMAND make
)

add_library(cmk SHARED
	src/cmk-button.c
	src/cmk-dialog.c
	src/cmk-icon.c
	src/cmk-icon-loader.c
	src/cmk-label.c
	src/cmk-scroll-box.c
	src/cmk-separator.c
	src/cmk-shadow.c
	src/cmk-textfield.c
	src/cmk-util.c
	src/cmk-widget.c
)
add_dependencies(cmk cmk-clutter-build)
set_target_properties(cmk PROPERTIES SOVERSION ${CMK_VERSION} VERSION ${CMK_VERSION})
target_link_libraries(cmk
	${CLUTTERDEPS_LIBRARIES}
	${LIBRSVG_LIBRARIES}
	
	-Wl,--whole-archive
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/.libs/libclutter-1.0.a
	-Wl,--no-whole-archive
)
target_include_directories(cmk PRIVATE
	${CLUTTERDEPS_INCLUDE_DIRS}
	${LIBRSVG_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/cmk-clutter/
)

find_package(GtkDoc 1.25)
if(GTKDOC_FOUND)
	gtk_doc_add_module(cmkdoc
		SOURCE ${CMAKE_SOURCE_DIR}/src
		LIBRARIES cmk
		# GtkDoc cmake package only does header files by default
		# Need to scan source files for signal documentation
		SUFFIXES "h,c"
		DEPENDS cmk
	)
	
	add_custom_target(documentation DEPENDS doc-cmkdoc)
	# Install not needed right now
	#install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmkdoc/html DESTINATION share/doc)
endif(GTKDOC_FOUND)

install(TARGETS cmk DESTINATION lib)
install(DIRECTORY src/ DESTINATION include/libcmk/cmk FILES_MATCHING PATTERN "*.h")
install(FILES
	cmk-clutter/clutter/clutter-action.h
	cmk-clutter/clutter/clutter-actor-meta.h
	cmk-clutter/clutter/clutter-actor.h
	cmk-clutter/clutter/clutter-align-constraint.h
	cmk-clutter/clutter/clutter-animatable.h
	cmk-clutter/clutter/clutter-autocleanups.h
	cmk-clutter/clutter/clutter-backend.h
	cmk-clutter/clutter/clutter-bin-layout.h
	cmk-clutter/clutter/clutter-bind-constraint.h
	cmk-clutter/clutter/clutter-binding-pool.h
	cmk-clutter/clutter/clutter-blur-effect.h
	cmk-clutter/clutter/clutter-box-layout.h
	cmk-clutter/clutter/clutter-brightness-contrast-effect.h
	cmk-clutter/clutter/clutter-cairo.h
	cmk-clutter/clutter/clutter-canvas.h
	cmk-clutter/clutter/clutter-child-meta.h
	cmk-clutter/clutter/clutter-click-action.h
	cmk-clutter/clutter/clutter-clone.h
	cmk-clutter/clutter/clutter-cogl-compat.h
	cmk-clutter/clutter/clutter-color-static.h
	cmk-clutter/clutter/clutter-color.h
	cmk-clutter/clutter/clutter-colorize-effect.h
	cmk-clutter/clutter/clutter-config.h
	cmk-clutter/clutter/clutter-constraint.h
	cmk-clutter/clutter/clutter-container.h
	cmk-clutter/clutter/clutter-content.h
	cmk-clutter/clutter/clutter-deform-effect.h
	cmk-clutter/clutter/clutter-deprecated.h
	cmk-clutter/clutter/clutter-desaturate-effect.h
	cmk-clutter/clutter/clutter-device-manager.h
	cmk-clutter/clutter/clutter-drag-action.h
	cmk-clutter/clutter/clutter-drop-action.h
	cmk-clutter/clutter/clutter-effect.h
	cmk-clutter/clutter/clutter-enum-types.h
	cmk-clutter/clutter/clutter-enums.h
	cmk-clutter/clutter/clutter-event.h
	cmk-clutter/clutter/clutter-feature.h
	cmk-clutter/clutter/clutter-fixed-layout.h
	cmk-clutter/clutter/clutter-flow-layout.h
	cmk-clutter/clutter/clutter-gesture-action.h
	cmk-clutter/clutter/clutter-grid-layout.h
	cmk-clutter/clutter/clutter-group.h
	cmk-clutter/clutter/clutter-image.h
	cmk-clutter/clutter/clutter-input-device.h
	cmk-clutter/clutter/clutter-interval.h
	cmk-clutter/clutter/clutter-keyframe-transition.h
	cmk-clutter/clutter/clutter-keysyms.h
	cmk-clutter/clutter/clutter-layout-manager.h
	cmk-clutter/clutter/clutter-layout-meta.h
	cmk-clutter/clutter/clutter-macros.h
	cmk-clutter/clutter/clutter-main.h
	cmk-clutter/clutter/clutter-marshal.h
	cmk-clutter/clutter/clutter-offscreen-effect.h
	cmk-clutter/clutter/clutter-page-turn-effect.h
	cmk-clutter/clutter/clutter-paint-node.h
	cmk-clutter/clutter/clutter-paint-nodes.h
	cmk-clutter/clutter/clutter-pan-action.h
	cmk-clutter/clutter/clutter-path-constraint.h
	cmk-clutter/clutter/clutter-path.h
	cmk-clutter/clutter/clutter-property-transition.h
	cmk-clutter/clutter/clutter-rotate-action.h
	cmk-clutter/clutter/clutter-script.h
	cmk-clutter/clutter/clutter-scriptable.h
	cmk-clutter/clutter/clutter-scroll-actor.h
	cmk-clutter/clutter/clutter-settings.h
	cmk-clutter/clutter/clutter-shader-effect.h
	cmk-clutter/clutter/clutter-shader-types.h
	cmk-clutter/clutter/clutter-snap-constraint.h
	cmk-clutter/clutter/clutter-stage-manager.h
	cmk-clutter/clutter/clutter-stage.h
	cmk-clutter/clutter/clutter-swipe-action.h
	cmk-clutter/clutter/clutter-tap-action.h
	cmk-clutter/clutter/clutter-test-utils.h
	cmk-clutter/clutter/clutter-text-buffer.h
	cmk-clutter/clutter/clutter-text.h
	cmk-clutter/clutter/clutter-texture.h
	cmk-clutter/clutter/clutter-timeline.h
	cmk-clutter/clutter/clutter-transition-group.h
	cmk-clutter/clutter/clutter-transition.h
	cmk-clutter/clutter/clutter-types.h
	cmk-clutter/clutter/clutter-units.h
	cmk-clutter/clutter/clutter-version.h
	cmk-clutter/clutter/clutter-zoom-action.h
	cmk-clutter/clutter/clutter.h
	DESTINATION include/libcmk/clutter)

install(FILES
	cmk-clutter/clutter/x11/clutter-x11-texture-pixmap.h
	cmk-clutter/clutter/x11/clutter-x11.h
	DESTINATION include/libcmk/clutter/x11)

install(FILES
	cmk-clutter/clutter/deprecated/clutter-actor.h
	cmk-clutter/clutter/deprecated/clutter-alpha.h
	cmk-clutter/clutter/deprecated/clutter-animatable.h
	cmk-clutter/clutter/deprecated/clutter-animation.h
	cmk-clutter/clutter/deprecated/clutter-animator.h
	cmk-clutter/clutter/deprecated/clutter-backend.h
	cmk-clutter/clutter/deprecated/clutter-behaviour-depth.h
	cmk-clutter/clutter/deprecated/clutter-behaviour-ellipse.h
	cmk-clutter/clutter/deprecated/clutter-behaviour-opacity.h
	cmk-clutter/clutter/deprecated/clutter-behaviour-path.h
	cmk-clutter/clutter/deprecated/clutter-behaviour-rotate.h
	cmk-clutter/clutter/deprecated/clutter-behaviour-scale.h
	cmk-clutter/clutter/deprecated/clutter-behaviour.h
	cmk-clutter/clutter/deprecated/clutter-bin-layout.h
	cmk-clutter/clutter/deprecated/clutter-box.h
	cmk-clutter/clutter/deprecated/clutter-cairo-texture.h
	cmk-clutter/clutter/deprecated/clutter-container.h
	cmk-clutter/clutter/deprecated/clutter-fixed.h
	cmk-clutter/clutter/deprecated/clutter-frame-source.h
	cmk-clutter/clutter/deprecated/clutter-group.h
	cmk-clutter/clutter/deprecated/clutter-input-device.h
	cmk-clutter/clutter/deprecated/clutter-keysyms.h
	cmk-clutter/clutter/deprecated/clutter-list-model.h
	cmk-clutter/clutter/deprecated/clutter-main.h
	cmk-clutter/clutter/deprecated/clutter-media.h
	cmk-clutter/clutter/deprecated/clutter-model.h
	cmk-clutter/clutter/deprecated/clutter-rectangle.h
	cmk-clutter/clutter/deprecated/clutter-score.h
	cmk-clutter/clutter/deprecated/clutter-shader.h
	cmk-clutter/clutter/deprecated/clutter-stage-manager.h
	cmk-clutter/clutter/deprecated/clutter-stage.h
	cmk-clutter/clutter/deprecated/clutter-state.h
	cmk-clutter/clutter/deprecated/clutter-table-layout.h
	cmk-clutter/clutter/deprecated/clutter-texture.h
	cmk-clutter/clutter/deprecated/clutter-timeline.h
	cmk-clutter/clutter/deprecated/clutter-timeout-pool.h
	cmk-clutter/clutter/deprecated/clutter-util.h
	DESTINATION include/libcmk/clutter/deprecated)

install(FILES ${PROJECT_BINARY_DIR}/libcmk.pc DESTINATION lib/pkgconfig)
