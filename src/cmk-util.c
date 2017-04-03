#include "cmk-widget.h"
#include "cmk-icon-loader.h"

/*
 * Cmk uses its own DPI / GUI scaling features, so force Clutter to
 * think that the system's DPI scale is 1.
 * Call this before calling clutter_init().
 */
void cmk_disable_system_guiscale(void)
{
	// Should work for the GDK and X11 backends. Wayland?
	g_setenv("CLUTTER_SCALE", "1", TRUE);
	g_setenv("GDK_SCALE", "1", TRUE);
	g_unsetenv("GDK_DPI_SCALE");
	//clutter_set_windowing_backend(CLUTTER_WINDOWING_GDK);
	ClutterSettings *settings = clutter_settings_get_default();
	g_object_set(settings, "window-scaling-factor", 1, NULL);
}


static gint dpiWatchId = 0;
static const gint baseDPI = 1024 * 96;
static gint dpiReq = 1024*96;

/*
 * From all my investigation into the API and source code, there appears to
 * be no way to interrupt Clutter's auto-detection of font dpi from the
 * current system. Ideally, font dpi could be a Cmk style property. However,
 * that would be very annoying and hacky to setup without creating a custom
 * version of ClutterText (which I don't want to do).
 * This method is called at the end of the Backend's resolution-chaged
 * emission, and checks to see if the resolution that has been set is the
 * one we want. If not, change it back.
 */
static void reset_clutter_dpi(void)
{
	ClutterSettings *settings = clutter_settings_get_default();
	gint dpi = 0;
	g_object_get(settings, "font-dpi", &dpi, NULL);
	if(dpi != dpiReq)
	{
		// The setter for font-dpi scales the value by GDK_DPI_SCALE, which
		// is very annoying. So just make sure that's unset. Whatever.
		g_unsetenv("GDK_DPI_SCALE");
		g_object_set(settings, "font-dpi", dpiReq, NULL);
	}
}

/*
 * Sets the Cmk Style Widget's scale factor, but also forces Clutter to
 * use an appropriate DPI scale. This is a bit of a hack until I get around
 * to (a) implementing a custom backend instead of using Clutter's, or (b)
 * make my own text widget as a replacement for ClutterText.
 */
void cmk_set_gui_scale(gfloat scale)
{
	CmkWidget *style = cmk_widget_get_style_default();
	cmk_widget_style_set_scale_factor(style, scale);
	g_object_unref(style);
	dpiReq = (gint)(baseDPI * scale);
	g_object_set(clutter_settings_get_default(), "font-dpi", dpiReq, NULL);
	if(!dpiWatchId)
		dpiWatchId = g_signal_connect_after(clutter_get_default_backend(), "resolution-changed", G_CALLBACK(reset_clutter_dpi), NULL);
}

static void on_global_scale_changed(CmkIconLoader *iconLoader)
{
	cmk_set_gui_scale(cmk_icon_loader_get_scale(iconLoader));
}

static void on_window_allocate(ClutterActor *stage, ClutterActorBox *box, ClutterAllocationFlags flags, ClutterActor *bg)
{
	//ClutterActorBox child = {0,0,box->x2-box->x1,box->y2-box->y1};
	float w, h;
	clutter_actor_get_size(stage, &w, &h);
	clutter_actor_set_size(bg, w, h);
	//clutter_actor_allocate(bg, &child, flags);
}

/*
 * Creates a window with automatic GUI scaling based on the Cmk scale factor property.
 * Call cmk_disable_system_guiscale before calling this.
 */
CmkWidget * cmk_window_new(float width, float height)
{
	CmkWidget *style = cmk_widget_get_style_default();
	CmkIconLoader *iconLoader = cmk_icon_loader_get_default();
	g_signal_connect(iconLoader, "notify::scale", G_CALLBACK(on_global_scale_changed), NULL);
	on_global_scale_changed(iconLoader);

	gfloat scale = cmk_widget_style_get_scale_factor(style);

	ClutterActor *stage = clutter_stage_new();
	clutter_stage_set_user_resizable(CLUTTER_STAGE(stage), TRUE);
	clutter_stage_set_minimum_size(CLUTTER_STAGE(stage), 100*scale, 100*scale);
	//clutter_stage_set_no_clear_hint(CLUTTER_STAGE(stage), TRUE);
	clutter_actor_set_size(stage, width*scale, height*scale);
	clutter_actor_show(stage);

	CmkWidget *bg = cmk_widget_new();
	cmk_widget_set_background_color_name(bg, "background");
	cmk_widget_set_draw_background_color(bg, TRUE);
	clutter_actor_add_child(stage, CLUTTER_ACTOR(bg));
	g_signal_connect(stage, "allocation-changed", G_CALLBACK(on_window_allocate), bg);
	on_window_allocate(stage, NULL, 0, CLUTTER_ACTOR(bg));
	return bg;
}
