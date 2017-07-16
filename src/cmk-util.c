/*
 * libcmk
 * Copyright (C) 2017 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-util.h"

gboolean cmk_init(int *argc, char ***argv)
{
	g_setenv("CLUTTER_SCALE", "1", TRUE);
	ClutterSettings *settings = clutter_settings_get_default();
	g_object_set(settings, "window-scaling-factor", 1, NULL);
	
	clutter_set_windowing_backend(CLUTTER_WINDOWING_X11);
	
	return clutter_init(argc, argv) == CLUTTER_INIT_SUCCESS;
}

void cmk_main(void)
{
	clutter_main();
}

static void on_global_scale_changed(GSettings *interface, const gchar *key, CmkWidget *root)
{
	guint scale = g_settings_get_uint(interface, key);
	cmk_widget_set_dp_scale(root, scale);
}

void cmk_auto_dpi_scale(CmkWidget *root)
{
	GSettings *interface = g_settings_new("org.gnome.desktop.interface");
	
	g_signal_connect(interface,
	                 "changed::scaling-factor",
	                 G_CALLBACK(on_global_scale_changed),
	                 root);
	on_global_scale_changed(interface, "scaling-factor", root);
}

CmkWidget * cmk_window_new(const gchar *title, float width, float height, ClutterStage **stageptr)
{	
	CmkWidget *bg = cmk_widget_new();
	cmk_auto_dpi_scale(bg);
	float dpScale = cmk_widget_get_dp_scale(bg);
	
	ClutterActor *stage = clutter_stage_new();
	clutter_stage_set_title(CLUTTER_STAGE(stage), title);
	clutter_stage_set_user_resizable(CLUTTER_STAGE(stage), TRUE);
	clutter_stage_set_no_clear_hint(CLUTTER_STAGE(stage), TRUE);
	clutter_stage_set_minimum_size(CLUTTER_STAGE(stage), width*dpScale/2, height*dpScale/2);
	clutter_actor_set_size(stage, width*dpScale, height*dpScale);

	cmk_widget_set_draw_background_color(bg, TRUE);
	clutter_actor_add_child(stage, CLUTTER_ACTOR(bg));
	clutter_actor_show(stage);
	
	// TODO: This glitches during maximize/unmaximize. How to fix?
	clutter_actor_add_constraint(CLUTTER_ACTOR(bg), clutter_bind_constraint_new(CLUTTER_ACTOR(stage), CLUTTER_BIND_ALL, 0));
	
	if(stageptr)
		*stageptr = CLUTTER_STAGE(stage);
	return bg;
}
