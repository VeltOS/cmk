/*
 * libcmk
 * Copyright (C) 2017 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-util.h"
#include <clutter/x11/clutter-x11.h>
#include <math.h>

static gboolean autoWindowScale = TRUE;

gboolean cmk_init(int *argc, char ***argv)
{
	g_unsetenv("CLUTTER_SCALE");
	
	ClutterSettings *settings = clutter_settings_get_default();
	// auto-window-scale is a custom Clutter feature
	// TRUE is default clutter behavior. FALSE is the window-scaling-factor
	// value will be updated as usual, but Clutter will not attempt
	// to automatically do anything based on that value.
	if(g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "auto-window-scale"))
	{
		autoWindowScale = FALSE;
		g_object_set(settings, "auto-window-scale", FALSE, NULL);
	}
	else
	{
		// Support both just in case we're running without Custom clutter.
		// This won't work as a user might expect when running on
		// "auto-detect" scale
		g_object_set(settings, "window-scaling-factor", 1, NULL);
	}
	
	clutter_set_windowing_backend(CLUTTER_WINDOWING_X11);
	
	return clutter_init(argc, argv) == CLUTTER_INIT_SUCCESS;
}

void cmk_main(void)
{
	clutter_main();
}

static void on_window_scale_factor_changed(gpointer settings, const gchar *key, CmkWidget *root)
{
	gint factor;
	if(CLUTTER_IS_SETTINGS(settings))
		g_object_get(settings, "window-scaling-factor", &factor, NULL);
	else if(G_IS_SETTINGS(settings))
		factor = g_settings_get_uint(settings, "scaling-factor");
	
	if(factor <= 0)
		factor = 1;
		
	cmk_widget_set_dp_scale(root, factor);
}

void cmk_auto_dpi_scale(CmkWidget *root)
{
	if(autoWindowScale)
	{
		GSettings *interface = g_settings_new("org.gnome.desktop.interface");
		g_signal_connect(interface,
			"changed::scaling-factor",
			G_CALLBACK(on_window_scale_factor_changed),
			root);
		on_window_scale_factor_changed(interface, "scaling-factor", root);
	}
	else
	{
		ClutterSettings *settings = clutter_settings_get_default();
		g_signal_connect(settings,
			"notify::window-scaling-factor",
			G_CALLBACK(on_window_scale_factor_changed),
			root);
		on_window_scale_factor_changed(settings, "window-scaling-factor", root);
	}
}

typedef struct
{
	float prevScale;
} ScaleData;

static void on_stage_dp_scale_changed(CmkWidget *stagebg)
{
	float scale = cmk_widget_get_dp_scale(stagebg);
	ScaleData *sd = g_object_get_data(G_OBJECT(stagebg), "prev-scale");
	float factor = scale / sd->prevScale;
	sd->prevScale = scale;

	ClutterStage *stage = CLUTTER_STAGE(clutter_actor_get_stage(CLUTTER_ACTOR(stagebg)));
	guint w, h;
	clutter_stage_get_minimum_size(stage, &w, &h);
	w = round(w*factor);
	h = round(h*factor);
	clutter_stage_set_minimum_size(stage, w, h);
	float fw, fh;
	clutter_actor_get_size(CLUTTER_ACTOR(stage), &fw, &fh);
	fw = fw*factor;
	fh = fh*factor;
	clutter_actor_set_size(CLUTTER_ACTOR(stage), fw, fh);
}

CmkWidget * cmk_window_new(const gchar *title, const gchar *class, float width, float height, ClutterStage **stageptr)
{	
	CmkWidget *bg = cmk_widget_new();
	float dpScale = cmk_widget_get_dp_scale(bg);
	ScaleData *sd = g_new(ScaleData, 1);
	sd->prevScale = dpScale;
	g_object_set_data_full(G_OBJECT(bg), "prev-scale", sd, g_free);
	
	ClutterActor *stage = clutter_stage_new();
	g_object_set_data(G_OBJECT(stage), "CmkWindow", bg);
	clutter_stage_set_title(CLUTTER_STAGE(stage), title);
	clutter_stage_set_user_resizable(CLUTTER_STAGE(stage), TRUE);
	clutter_stage_set_no_clear_hint(CLUTTER_STAGE(stage), TRUE);
	clutter_stage_set_minimum_size(CLUTTER_STAGE(stage), width/2, height/2);
	clutter_actor_set_size(stage, width, height);

	cmk_widget_set_draw_background_color(bg, TRUE);
	cmk_focus_stack_push(bg);
	clutter_actor_add_child(stage, CLUTTER_ACTOR(bg));

	g_signal_connect(bg, "notify::dp-scale", G_CALLBACK(on_stage_dp_scale_changed), NULL);
	cmk_auto_dpi_scale(bg);

	clutter_actor_show(stage);
	
	// TODO: Eventually wayland support
	Display *xdisplay = clutter_x11_get_default_display();
	Window xwindow = clutter_x11_get_stage_window(CLUTTER_STAGE(stage));
	if(xdisplay && xwindow)
	{
		XClassHint *classHint = XAllocClassHint();
		classHint->res_class = (char *)class;
		classHint->res_name = (char *)class;
		XSetClassHint(xdisplay, xwindow, classHint);
		XFree(classHint);
	}
	
	// TODO: This glitches during maximize/unmaximize. How to fix?
	clutter_actor_add_constraint(CLUTTER_ACTOR(bg), clutter_bind_constraint_new(CLUTTER_ACTOR(stage), CLUTTER_BIND_ALL, 0));
	
	if(stageptr)
		*stageptr = CLUTTER_STAGE(stage);
	return bg;
}

CmkWidget * cmk_widget_get_window(CmkWidget *widget)
{
	ClutterActor *stage = clutter_actor_get_stage(CLUTTER_ACTOR(widget));
	if(!stage)
		return NULL;
	return CMK_WIDGET(g_object_get_data(G_OBJECT(stage), "CmkWindow"));
}
