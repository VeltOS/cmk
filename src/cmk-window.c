/*
 * libcmk
 * Copyright (C) 2017 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

//#include "cmk-main.h"
//#include "cmk-window.h"
//
//#define CMK_WAYLAND
//#define CMK_XORG
//
//#ifdef CMK_WAYLAND
//#include <wayland-cursor.h>
//static gboolean cmk_init_wayland(void);
//#endif
//
//#ifdef CMK_XORG
//static gboolean cmk_init_xorg(void);
//#endif
//
//
//
///*
// * CmkWindow generic
// */
//
//typedef struct
//{
//	ClutterStage *stage;
//	
//	#ifdef CMK_WAYLAND
//	struct wl_surface *surface;
//	struct wl_shell_surface *shellSurface;
//	guint surfaceScale;
//	#endif
//} CmkWindow;
//
//static gboolean (*cmk_window_init)(CmkWindow *window) = NULL;
//static void (*cmk_window_deinit)(CmkWindow *window) = NULL;
//
//CmkWindow * cmk_window_new(const gchar *title, CmkWidget **stage)
//{
//	if(!cmk_window_init)
//		return NULL;
//	
//	CmkWindow *window = g_new0(CmkWindow, 1);
//	
//	if(!cmk_window_init(window))
//	{
//		g_free(window);
//		return NULL;
//	}
//	
//	ClutterStage *rstage = window->stage;
//	clutter_stage_set_no_clear_hint(rstage, TRUE);
//	clutter_stage_set_title(rstage, title);
//	clutter_actor_show(CLUTTER_ACTOR(rstage));
//	
//	*stage = cmk_widget_new();
//	cmk_widget_set_background_color_name(*stage, "background");
//	cmk_widget_set_draw_background_color(*stage, TRUE);
//	clutter_actor_add_child(CLUTTER_ACTOR(rstage), CLUTTER_ACTOR(*stage));
//	
//	clutter_actor_add_constraint(CLUTTER_ACTOR(*stage), clutter_bind_constraint_new(CLUTTER_ACTOR(rstage), CLUTTER_BIND_ALL, 0));
//	return window;
//}
//
//void cmk_window_destroy(CmkWindow *window)
//{
//	g_return_if_fail(window != NULL);
//	if(cmk_window_deinit)
//		cmk_window_deinit(window);
//}
//
//
//
///*
// * Generic Cmk initialization
// */
//
//static void (*cmk_deinit)(void) = NULL;
//static void (*cmk_update_cursor_icon_theme)(void) = NULL;
//static void (*cmk_input_device_set_cursor_icon_real)(ClutterInputDevice *device, const gchar *iconName) = NULL;
//static void (*cmk_input_device_set_cursor_pixmap_real)(ClutterInputDevice *device, void *data, guint width, guint height) = NULL;
//
//static guint globalScaleFactor = 1;
//static gchar *globalCursorIconTheme = NULL;
//static gchar *cursorIconTheme = NULL;
//static guint globalCursorIconSize = 0;
//static guint cursorIconSize = 0;
//
//
//void cmk_set_cursor_icon_theme(const gchar *theme, guint size)
//{
//	gboolean upd = FALSE;
//	if(g_strcmp0(theme, cursorIconTheme) != 0)
//	{
//		g_clear_pointer(&cursorIconTheme, g_free);
//		cursorIconTheme = g_strdup(theme);
//		upd = TRUE;
//	}
//	
//	if(cursorIconSize != size)
//	{
//		cursorIconSize = size;
//		upd = TRUE;
//	}
//	
//	if(cmk_update_cursor_icon_theme && upd)
//		cmk_update_cursor_icon_theme();
//}
//
//void cmk_input_device_set_cursor_icon(ClutterInputDevice *device, const gchar *iconName)
//{
//	if(cmk_input_device_set_cursor_icon_real)
//		cmk_input_device_set_cursor_icon_real(device, iconName);
//}
//
//void cmk_input_device_set_cursor_pixmap(ClutterInputDevice *device, void *data, guint width, guint height)
//{
//	if(cmk_input_device_set_cursor_pixmap_real)
//		cmk_input_device_set_cursor_pixmap_real(device, data, width, height);
//}
//
//
//
///*
// * Wayland Initialization
// * 
// * This overwrites a lot of the work that is normally preformed by
// * cogl/Clutter for DPI scaling and window decroations. Hopefully
// * when/if Clutter merges with GDK to become GTK's backend, this
// * will no longer be necessary.
// *
// * Mutter (and it seems most Wayland Compositors) don't provide
// * their clients with window decorations, expecting toolkits like
// * GTK to draw them. A (large) attempt was made to avoid writing
// * custom window decorations in Cmk by embedding the clutter drawing
// * surface into a GTK window (manually and through GtkClutterEmbed).
// * However, I was unable to get this to work for unknown reasons.
// * It's probably for the best that Cmk draws its own decorations for
// * a consistent material design look anyway.
// *
// * TODO: Since Wayland has to draw its own decorations, setup the
// * CMK_XORG backend to also use custom decorations.
// */
//#ifdef CMK_WAYLAND
//static struct wl_display *cwlDisplay = NULL;
//static struct wl_compositor *cwlCompositor = NULL;
//static struct wl_shell *cwlShell = NULL;
//static struct wl_shm *cwlShm = NULL;
//static struct wl_cursor_theme *cwlCursorTheme = NULL;
//static GHashTable *cwlProxies = NULL;
//
//static gboolean cmk_init_wayland(void)
//{
//	
//	cmk_window_init = cmk_window_init_wayland;
//	cmk_window_deinit = cmk_window_deinit_wayland;
//	cmk_deinit = cmk_deinit_wayland;
//	cmk_update_cursor_icon_theme = cwl_update_cursor_icon_theme;
//	cmk_input_device_set_cursor_icon_real = cmk_input_device_set_cursor_icon_wayland;
//	cmk_input_device_set_cursor_pixmap_real = NULL;
//	return TRUE;
//}
//
//static void cwl_update_cursor_icon_theme(void)
//{
//	if(cwlCursorTheme)
//		wl_cursor_theme_destroy(cwlCursorTheme);
//	
//	guint size = cursorIconSize;
//	if(size == 0)
//		size = globalCursorIconSize;
//	if(size == 0)
//		size = 32;
//	
//	// Cursor stuff should use the global integer scale factor instead
//	// of the Cmk scale factor because icons work best at integer scales,
//	// and also it will look weird if the cursor changes sizes when
//	// mousing over the Cmk window.
//	size *= globalScaleFactor;
//	
//	cwlCursorTheme = wl_cursor_theme_load(
//		cursorIconTheme ? cursorIconTheme : globalCursorIconTheme,
//		size,
//		cwlShm);
//	
//	// TODO: Update all cursors
//}
//
//static void cwl_pointer_set_cursor_icon(struct wl_pointer *pointer, uint32_t serial, const gchar *iconName)
//{
//	if(!iconName)
//	{
//		// Invisible cursor
//		wl_pointer_set_cursor(pointer, serial, NULL, 0, 0);
//		return;
//	}
//	
//	if(!cwlCursorTheme)
//		cwl_update_cursor_icon_theme();
//	if(!cwlCursorTheme)
//		return;
//	
//	// TODO: Cursor animations
//	struct wl_cursor *cursor = wl_cursor_theme_get_cursor(cwlCursorTheme, iconName);
//	if(!cursor || cursor->image_count == 0)
//		return;
//	
//	guint hx = cursor->images[0]->hotspot_x / globalScaleFactor;
//	guint hy = cursor->images[0]->hotspot_y / globalScaleFactor;
//	void *buffer = wl_cursor_image_get_buffer(cursor->images[0]);
//	
//	struct wl_surface *surface = wl_compositor_create_surface(cwlCompositor);
//	wl_surface_set_buffer_scale(surface, globalScaleFactor);
//	
//	// TODO: Do the buffer and/or surface need to be manually destroyed?
//	// If so, when?
//	
//	wl_pointer_set_cursor(pointer, serial, surface, hx, hy);
//	wl_surface_attach(surface, buffer, 0, 0);
//	wl_surface_damage(surface, 0, 0, cursor->images[0]->width, cursor->images[0]->height);
//	wl_surface_commit(surface);
//}
//
//// Clutter's device management is mostly fine, but has two problems:
//// 1. The coords are not multiplied to match the DPI scale (fixed using
////    clutter_event_add_filter below).
//// 2. The pointers are always 32px default theme, not user-changable.
////    Even though Clutter is still setting the cursor, this is called
////    after their enter event handler because it is registered after (I
////    hope). Therefore, we can just change the cursor immediately after,
////    and it doesn't seem to cause any flicker.
////    And this can also be used for changing cursor icons on the window
////    decorations.
//static void cwl_pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
//{
//	g_message("enter");
//	// TODO: This may interfere with widgets (namely the stage) setting
//	// a cursor icon on enter, if it so happens that the widget is at
//	// the edge of the surface.
//	cwl_pointer_set_cursor_icon(pointer, serial, "left_ptr");
//}
//
//static void cwl_pointer_other()
//{
//}
//
//static void cwl_handle_seat(void *data, struct wl_seat *seat, uint32_t capabilities)
//{
//	g_message("cwl handle seat");
//	if((capabilities & WL_SEAT_CAPABILITY_POINTER) == WL_SEAT_CAPABILITY_POINTER)
//	{
//		// TODO: There exists a wl_pointer_destroy, but I'm not sure when or where
//		// to call it. Does the pointer proxy automatically get destroyed when the
//		// proxy is destroyed, or if it loses the pointer capability?
//		struct wl_pointer *pointer = wl_seat_get_pointer(seat);
//		if(pointer)
//		{
//			g_message("pointer listeneder");
//		}
//	}
//}
//
//
//
//static void on_wl_surface_scale_factor_changed(CmkWindow *window)
//{
//	window->surfaceScale = g_settings_get_uint(_cmkGlobalInterfaceSettings, "scaling-factor");
//	
//	// The wl surface must always be set to the same scale the
//	// window manager is expecting, because otherwise it will
//	// attempt to stretch the surface to match, resulting in blur.
//	if(window->surface)
//		wl_surface_set_buffer_scale(window->surface, window->surfaceScale);
//}
//
//static gboolean on_wl_clutter_event(ClutterEvent *event, CmkWindow *window)
//{
//	// Scale all events coordinates by the buffer scale, otherwise
//	// Clutter thinks everything is in its regular unscaled position.
//
//	// TODO: This would be better to put directly into input handling in
//	// clutter-input-device-wayland.c, because that would avoid a 2nd
//	// actor picking (would be slightly faster). However, putting it
//	// there would make for some weird inter-library code mixing.
//
//	gfloat x, y;
//	clutter_event_get_coords(event, &x, &y);
//	x *= window->surfaceScale;
//	y *= window->surfaceScale;
//	clutter_event_set_coords(event, x, y);
//	ClutterActor *source = clutter_stage_get_actor_at_pos(window->stage, CLUTTER_PICK_REACTIVE, x, y);
//	clutter_event_set_source(event, source);
//	return CLUTTER_EVENT_PROPAGATE;
//}
//
//static gboolean cmk_window_init_wayland(CmkWindow *window)
//{
//	if(!cwlCompositor || !cwlShell)
//		return FALSE;
//	
//	window->surface = wl_compositor_create_surface(cwlCompositor);
//	if(!window->surface)
//		return FALSE;
//	
//	window->shellSurface = wl_shell_get_shell_surface(cwlShell, window->surface);
//	if(!window->shellSurface)
//		return FALSE;
//
//	//g_signal_connect_swapped(_cmkGlobalInterfaceSettings, "changed::scaling-factor", G_CALLBACK(on_wl_surface_scale_factor_changed), window);
//	on_wl_surface_scale_factor_changed(window);
//	
//	window->stage = CLUTTER_STAGE(clutter_stage_new());
//	clutter_wayland_stage_set_wl_surface(window->stage, window->surface);
//	
//	clutter_event_add_filter(window->stage, (ClutterEventFilterFunc)on_wl_clutter_event, NULL, window);
//	return TRUE;
//}
//
//static void cmk_window_deinit_wayland(CmkWindow *window)
//{
//	clutter_actor_destroy(CLUTTER_ACTOR(window->stage));
//	if(window->surface)
//		wl_surface_destroy(window->surface);
//	if(window->shellSurface)
//		wl_shell_surface_destroy(window->shellSurface);
//}
//
//void cmk_input_device_set_cursor_icon_wayland(ClutterInputDevice *device, const gchar *iconName)
//{
//	g_return_if_fail(CLUTTER_IS_INPUT_DEVICE(device));
//	
//	if(clutter_input_device_get_device_type(device) != CLUTTER_POINTER_DEVICE)
//		return;
//	
//	struct wl_seat *seat = clutter_wayland_input_device_get_wl_seat(device);
//	struct wl_pointer *pointer = wl_seat_get_pointer(seat);
//	cwl_pointer_set_cursor_icon(pointer, 0, iconName);
//}
//
//#endif
//
//
//
///*
// * Xorg Initialization
// *
// * The default Clutter Xorg backend handles DPI scaling and window
// * decorations fine, although the decorations depend on Mutter
// * drawing them, which is not a guarantee(?). So for now, just use
// * Clutter's backend unmodified.
// *
// * TODO: Since Wayland has to draw its own decorations, setup the
// * CMK_XORG backend to also use custom decorations.
// */
//#ifdef CMK_XORG
//static gboolean cmk_window_xorg_init(CmkWindow *window)
//{
//	window->stage = CLUTTER_STAGE(clutter_stage_new());
//	return TRUE;
//}
//
//static void cmk_window_xorg_deinit(CmkWindow *window)
//{
//	clutter_actor_destroy(CLUTTER_ACTOR(window->stage));
//}
//
//static gboolean cmk_init_xorg(void)
//{
//	clutter_set_windowing_backend("x11");
//	if(clutter_init(NULL, NULL) != CLUTTER_INIT_SUCCESS)
//		return FALSE;
//	cmk_window_init = cmk_window_xorg_init;
//	cmk_window_deinit = cmk_window_xorg_deinit;
//	return TRUE;
//}
//#endif
