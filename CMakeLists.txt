cmake_minimum_required(VERSION 3.4)
project(libcmk)

set(CMK_VERSION 0.4)
#set(CMAKE_BUILD_TYPE DEBUG)

# Assume the user wants to install Cmk to /usr unless otherwise specified
# with the -DCMAKE_INSTALL_PREFIX=<path> flag to cmake. http://stackoverflow.com/a/16076855
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "default install path" FORCE)
endif()

configure_file(libcmk.pc.in libcmk.pc @ONLY)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CLUTTERDEPS REQUIRED cogl-1.0>=1.21.2 cogl-path-1.0 cairo-gobject>=1.14.0 gio-2.0>=2.44.0 atk>=2.5.3 pangocairo>=1.30 cogl-pango-1.0 json-glib-1.0>=0.12.0 gdk-3.0 wayland-cursor wayland-client xkbcommon x11 xext xdamage xcomposite>=0.4 xi pangoft2 gdk-pixbuf-2.0 libudev>=136 libinput>=0.19.0 librsvg-2.0)

add_custom_target(cmk-clutter-config
	# The clutter source files expect a config.h and clutter-config.h.
	# I just replicate the parts of it that are necessary with
	# CMake add_definitions. Only touch config.h if it doesn't exist,
	# to avoid full rebuild every time.
	VERBATIM
	COMMAND bash -c "if [ ! -f ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/config.h ]; then echo '#include \"clutter-config.h\"' > ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/config.h; fi"
	# I have no idea how the Clutter devs compile with this flag; the code
	# has a number of missing declarations.
	COMMAND sed -i "/-Werror=missing-declarations/d" ${CMAKE_SOURCE_DIR}/cmk-clutter/configure.ac
	# I need this because my GLib hasn't been updated to 2.54 yet (Arch is
	# still on 2.52).
	COMMAND sed -i "/--prototypes/d" ${CMAKE_SOURCE_DIR}/cmk-clutter/build/autotools/Makefile.am.marshal
)

set(CLUTTER_MAJOR_VERSION 1)
set(CLUTTER_MINOR_VERSION 26)
set(CLUTTER_MICRO_VERSION 1)
set(CLUTTER_VERSION ${CLUTTER_MAJOR_VERSION}.${CLUTTER_MINOR_VERSION}.${CLUTTER_MICRO_VERSION})
set(CLUTTER_CONFIG_DEFINES
"#define CLUTTER_WINDOWING_X11 \"x11\"
#define CLUTTER_INPUT_X11 \"x11\"
#define CLUTTER_WINDOWING_EGL \"eglnative\"
#define CLUTTER_INPUT_EVDEV \"evdev\"
#define CLUTTER_WINDOWING_WAYLAND \"wayland\"
#define CLUTTER_INPUT_WAYLAND \"wayland\"
#define CLUTTER_INPUT_NULL \"null\"
#define _CLUTTER_EXTERN __attribute__((visibility(\"default\")))"
)

configure_file(cmk-clutter/clutter/clutter-version.h.in ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-version.h @ONLY)
configure_file(cmk-clutter/clutter/clutter-config.h.in ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-config.h @ONLY)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-marshal.c
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/
	COMMAND glib-genmarshal --prefix=_clutter_marshal --body clutter-marshal.list > clutter-marshal.c
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-marshal.h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/
	COMMAND glib-genmarshal --prefix=_clutter_marshal --header clutter-marshal.list > clutter-marshal.h
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-enum-types.c
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/
	COMMAND glib-mkenums --template clutter-enum-types.c.in *.h > clutter-enum-types.c
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-enum-types.h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/
	COMMAND glib-mkenums --template clutter-enum-types.h.in *.h > clutter-enum-types.h
)

add_library(cmk-clutter STATIC
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-marshal.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-marshal.h
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-enum-types.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-enum-types.h
	
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-device-manager.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-bind-constraint.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/tslib/clutter-event-tslib.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-timeline.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-shader-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-flatten-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-path.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-desaturate-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-text-buffer.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-bezier.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-container.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-scroll-actor.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-master-clock.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-input-device.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-color.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-flow-layout.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-tap-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-event.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-scriptable.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-gesture-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-transition-group.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-feature.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-property-transition.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-align-constraint.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/wayland/clutter-event-wayland.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/wayland/clutter-stage-wayland.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/wayland/clutter-wayland-surface.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/wayland/clutter-backend-wayland.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/wayland/clutter-input-device-wayland.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/wayland/clutter-device-manager-wayland.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-binding-pool.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-constraint.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-brightness-contrast-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/egl/clutter-stage-eglnative.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/egl/clutter-backend-eglnative.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-box-layout.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-master-clock-default.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-zoom-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-layout-meta.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cogl/clutter-stage-cogl.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-shader-types.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-child-meta.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-grid-layout.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-deform-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-path-constraint.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-script-parser.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-text.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-stage-manager.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-backend.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-clone.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-page-turn-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-stage.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-click-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-offscreen-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-drop-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-keysyms-table.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-transition.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-bin-layout.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-fixed-layout.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-actor-box.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-script.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-units.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-easing.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-event-translator.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cex100/clutter-backend-cex100.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/gdk/clutter-device-manager-gdk.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/gdk/clutter-event-gdk.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/gdk/clutter-backend-gdk.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/gdk/clutter-stage-gdk.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/gdk/clutter-master-clock-gdk.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/gdk/clutter-input-device-gdk.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-interval.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-stage-window.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-colorize-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-id-pool.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-actor.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/win32/clutter-backend-win32.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/win32/clutter-stage-win32.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/win32/clutter-device-manager-win32.c
	#${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/win32/clutter-event-win32.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-input-device-core-x11.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-event-x11.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-device-manager-core-x11.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-x11-texture-pixmap.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-device-manager-xi2.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-backend-x11.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-keymap-x11.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-input-device-xi2.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-glx-texture-pixmap.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/xsettings/xsettings-common.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/xsettings/xsettings-client.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/clutter-stage-x11.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-drag-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-keyframe-transition.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-rotate-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-snap-constraint.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-pan-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-paint-nodes.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-image.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-base-types.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-paint-volume.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-paint-node.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-animatable.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-media.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-behaviour-path.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-behaviour-scale.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-behaviour-rotate.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-model.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-animator.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-timeout-interval.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-input-device-deprecated.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-group.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-fixed.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-table-layout.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-behaviour-ellipse.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-shader.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-animation.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-behaviour-depth.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-score.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-rectangle.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-texture.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-behaviour-opacity.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-frame-source.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-behaviour.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-cairo-texture.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-state.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-layout-manager-deprecated.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-actor-deprecated.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-list-model.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-alpha.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-box.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/clutter-timeout-pool.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/evdev/clutter-input-device-evdev.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/evdev/clutter-xkb-utils.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/evdev/clutter-event-evdev.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/evdev/clutter-device-manager-evdev.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-util.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-layout-manager.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-main.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-canvas.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-group.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-rectangle.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-texture.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-actor.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-stage.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-root.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-util.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-clone.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/cally-text.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-settings.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-blur-effect.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-content.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-cairo.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-actor-meta.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-swipe-action.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-test-utils.c
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/clutter-action.c
)
target_link_libraries(cmk-clutter
	${CLUTTERDEPS_LIBRARIES}
)
target_include_directories(cmk-clutter PRIVATE
	${CLUTTERDEPS_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/cmk-clutter/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cally/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cex100/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/cogl/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/deprecated/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/egl/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/evdev/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/gdk/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/mir/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/osx/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/tslib/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/wayland/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/win32/
	${CMAKE_SOURCE_DIR}/cmk-clutter/clutter/x11/
)
target_compile_definitions(cmk-clutter PRIVATE
	HAVE_CONFIG_H
	CLUTTER_COMPILATION
	COGL_ENABLE_EXPERIMENTAL_2_0_API
	COGL_ENABLE_EXPERIMENTAL_API
	CLUTTER_DRIVERS=\"*\"
	GETTEXT_PACKAGE=\"clutter-1.0\"
	CLUTTER_LOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\"
	CLUTTER_SYSCONFDIR=\"${CMAKE_INSTALL_PREFIX}/etc\"
)
set_target_properties(cmk-clutter
	PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	C_VISIBILITY_PRESET hidden)
target_compile_options(cmk-clutter PRIVATE "-Wno-deprecated-declarations" "-O3")
add_dependencies(cmk-clutter cmk-clutter-config)

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
target_compile_options(cmk PRIVATE "-O3")
set_target_properties(cmk PROPERTIES SOVERSION ${CMK_VERSION} VERSION ${CMK_VERSION})
target_link_libraries(cmk
	${CLUTTERDEPS_LIBRARIES}
	-Wl,--whole-archive
	cmk-clutter
	-Wl,--no-whole-archive
)
target_include_directories(cmk PRIVATE
	${CLUTTERDEPS_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/cmk-clutter/
)

find_package(GtkDoc 1.25)
if(GTKDOC_FOUND)
	gtk_doc_add_module(cmkdoc
		SOURCE ${CMAKE_SOURCE_DIR}/src
		CFLAGS -I${CMAKE_SOURCE_DIR}/cmk-clutter/
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
	cmk-clutter/clutter/wayland/clutter-wayland-compositor.h
	cmk-clutter/clutter/wayland/clutter-wayland-surface.h
	cmk-clutter/clutter/wayland/clutter-wayland.h
	DESTINATION include/libcmk/clutter/wayland)

install(FILES
	cmk-clutter/clutter/egl/clutter-egl-headers.h
	cmk-clutter/clutter/egl/clutter-egl.h
	DESTINATION include/libcmk/clutter/egl)

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
