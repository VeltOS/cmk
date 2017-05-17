/*
 * libcmk
 * Copyright (C) 2017 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-main.h"

//#ifndef CMK_NO_WAYLAND
//static gboolean cmk_init_wayland(void);
//#endif
//#ifndef CMK_NO_XORG
//static gboolean cmk_init_xorg(void);
//#endif


static void (*cmk_deinit)(void) = NULL;


static CmkWidget *styleDefault = NULL;
static GSettings *interfaceSettings = NULL;


gboolean cmk_init(gboolean wayland)
{
	cmk_auto_dpi_scale();
	
	
	//cwl_client_init();
	//cwl_window_new();
	//#ifndef CMK_NO_WAYLAND
	//if(wayland && cmk_init_wayland())
	//	return TRUE;
	//#endif
	//#ifndef CMK_NO_XORG
	//if(cmk_init_xorg())
	//	return TRUE;
	//#endif

	return FALSE;
}

void cmk_main(void)
{	
	clutter_main();
	if(cmk_deinit)
		cmk_deinit();
}


static void on_global_scale_factor_changed()
{
	guint scale = g_settings_get_uint(interfaceSettings, "scaling-factor");
	cmk_widget_style_set_scale_factor(styleDefault, scale);
}

void cmk_auto_dpi_scale()
{
	g_setenv("CLUTTER_SCALE", "1", TRUE);
	ClutterSettings *settings = clutter_settings_get_default();
	g_object_set(settings, "window-scaling-factor", 1, NULL);

	if(!styleDefault)
		styleDefault = cmk_widget_get_style_default();
	if(!interfaceSettings)
		interfaceSettings = g_settings_new("org.gnome.desktop.interface");

	g_signal_connect(interfaceSettings, "changed::scaling-factor", G_CALLBACK(on_global_scale_factor_changed), NULL);
	on_global_scale_factor_changed();
}


//static void on_global_cursor_theme_changed()
//{
//	const gchar *theme = g_settings_get_string(interfaceSettings, "cursor-theme");
//	
//	gboolean themeChanged = (g_strcmp0(theme, globalCursorIconTheme) != 0);
//	if(themeChanged)
//	{
//		g_clear_pointer(&globalCursorIconTheme, g_free);
//		globalCursorIconTheme = g_strdup(theme);
//	}
//	
//	gint size = g_settings_get_int(interfaceSettings, "cursor-size");
//	guint usize = MAX(0, size);
//	
//	gboolean sizeChanged = (usize != globalCursorIconSize);
//	globalCursorIconSize = usize;
//	
//	if(cmk_update_cursor_icon_theme
//	&& ((themeChanged && !cursorIconTheme) || (sizeChanged && cursorIconSize == 0)))
//		cmk_update_cursor_icon_theme();
//}

	
