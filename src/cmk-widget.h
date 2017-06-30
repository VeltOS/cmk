/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_WIDGET_H__
#define __CMK_WIDGET_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CMK_TYPE_WIDGET cmk_widget_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkWidget, cmk_widget, CMK, WIDGET, ClutterActor);

/*
 * Converts dps to pixels. This should be used when drawing raw pixels to
 * a canvas or when allocating child widgets.
 */
#define CMK_DP(widget, dps) (cmk_widget_style_get_dp_scale(widget)*dps)

/*
 * Flags for all CmkWidget style properties.
 */
enum
{
	CMK_STYLE_FLAG_COLORS = 1<<0,
	CMK_STYLE_FLAG_BACKGROUND_NAME = 1<<1,
	CMK_STYLE_FLAG_PADDING_MUL = 1<<2,
	CMK_STYLE_FLAG_BEVEL_MUL = 1<<3,
	CMK_STYLE_FLAG_DP = 1<<4,
	CMK_STYLE_FLAG_ALL = (1<<5)-1,
} CmkStyleFlag;

typedef struct _CmkWidgetClass CmkWidgetClass;

struct _CmkWidgetClass
{
	ClutterActorClass parentClass;
	
	/*
	 * This method is called when style properties are updated on this
	 * widget or any parent widgets. CmkWidget subclasses should use this
	 * as an indicator to invalidate ClutterCanvases or update colors.
	 * When overriding this method, chain up to the parent class BEFORE
	 * updating your widget.
	 *
	 * flags is a set of CmkWidgetStyleFlags indicating which style(s)
	 * have changed. 
	 * 
	 * Changes to the dp and padding properties automatically queue
	 * relayouts, which in turn queue redraws, so you may not need to
	 * manually update anything when those properties change.
	 */
	void (*styles_changed) (CmkWidget *self, guint flags);
	
	/*
	 * Emitted when a CmkWidget in the actor heirarchy below this widget
	 * gains key focus. Chain up to parent class if event goes unhandled.
	 */
	void (*key_focus_changed) (CmkWidget *self, ClutterActor *newfocus);
	
	/*
	 * Emitted by widget subclasses when the parent widget should replace
	 * them with a new widget. It is up to the parent widget to honor this.
	 * (This can also be used as a general 'forward' signal, with replacement
	 * being NULL and the parent having a planned list of screens.)
	 */
	void (*replace) (CmkWidget *self, CmkWidget *replacement);
	
	/*
	 * Emitted by widget subclasses when the parent should remove this widget
	 * and return to the previously shown widget (opposite of 'replace').
	 * It is up to the parent widget to honor this.
	 */
	void (*back) (CmkWidget *self);
};

typedef struct _CmkNamedColor CmkNamedColor;
struct _CmkNamedColor
{
	const gchar *name;
	ClutterColor color;
};

/*
 * CmkWidget is not abstract. Creating a widget on its own is effectively
 * just a ClutterActor with styling properties.
 */
CmkWidget * cmk_widget_new();

/*
 * Exactly the same as clutter_actor_destroy, except returns G_SOURCE_REMOVE
 * so this can easily be called from timeouts. Safe to pass in a regular
 * ClutterActor* casted to a CmkWidget*.
 */
gboolean cmk_widget_destroy(CmkWidget *widget);

/*
 * Instead of using the widget's actual parent, use a different widget to
 * inherit styles from. Set to NULL to return to using the actual parent.
 *
 * If the parent is destroyed, the "style parent" association will
 * automatically be removed and the widget will return to using its real
 * parent or no parent.
 */
void cmk_widget_set_style_parent(CmkWidget *widget, CmkWidget *parent);

/*
 * Gets the current style parent. If cmk_widget_set_style_parent has been
 * called, this returns that value. Otherwise, this returns the widget's
 * current actual parent if it is a CmkWidget or NULL otherwise.
 */
CmkWidget * cmk_widget_get_style_parent(CmkWidget *widget);

/*
 * Sets a named color that can be used to draw the widget. This color will be
 * inherited by child CmkWidgets.
 */
void cmk_widget_set_named_color(CmkWidget *widget, const gchar *name, const ClutterColor *color);

/*
 * Takes an array of CmkNamedColor (last one must be a NULLed
 * struct) and calls cmk_widget_style_set_color on each.
 */
void cmk_widget_set_named_colors(CmkWidget *widget, const CmkNamedColor *colors);

/*
 * Gets a named color set with cmk_widget_set_style_color. If it is not
 * found, an inherited value is returned. If there is no inherited value,
 * NULL is returned.
 */
const ClutterColor * cmk_widget_get_named_color(CmkWidget *widget, const gchar *name);

/*
 * Similar to clutter_actor_set_background_color, except it uses the named
 * color from the current style. Set to NULL to have a fully transparent
 * background (the default).
 * See cmk_widget_set_draw_background_color.
 */
void cmk_widget_set_background_color(CmkWidget *widget, const gchar *namedColor);

/*
 * Gets the effective background color of the widget. If no background
 * color for this widget has been set, the parent widget's get_background_color
 * method will be called. Foreground actors should base their color on this
 * background color. If this method returns NULL, the background color is
 * unknown, and foreground actors should use a default color.
 */
const gchar * cmk_widget_get_background_color(CmkWidget *widget);

/*
 * Similar to calling cmk_widget_get_background_color and
 * cmk_widget_get_named_color, except returns solid white if no color is found.
 */
const ClutterColor * cmk_widget_get_background_clutter_color(CmkWidget *widget);

/*
 * If TRUE, CmkWidget will draw the color set in
 * cmk_widget_set_background_color_name. Otherwise, you should draw it.
 * Defaults to FALSE.
 */
void cmk_widget_set_draw_background_color(CmkWidget *widget, gboolean draw);

/*
 * Attempts to find a foreground color from the current background color.
 * This first tries to use the named color "<background name>-foreground",
 * then tries "foreground", and if both fail, returns solid black.
 */
const ClutterColor * cmk_widget_get_foreground_clutter_color(CmkWidget *widget);

/*
 * Sets the number of pixels per 1 dp. Defaults to 1.
 * 
 * dps are "density-independent pixels", similar to ems in HTML.
 * Most Cmk functions that take measurements use dps.
 */
void cmk_widget_set_dp_scale(CmkWidget *widget, float dp);

/*
 * Gets the number of pixels per 1 dp. This value is scaled by the widget
 * graph; therefore, calling cmk_widget_set_style_dp(2) on the root
 * widget will scale all widgets below it by 2.
 *
 * When drawing a widget, remember to use dps instead of pixel values.
 * This can be done easily using the CMK_DP macro. Values in ClutterActorBox,
 * passed to widgets' allocation functions, must be in pixels.
 */
float cmk_widget_get_dp_scale(CmkWidget *widget);

/*
 * Sets the bevel radius multiplier, which the widget's draw method may
 * use if it draws bevels. Defaults to 1. Set to - 1 for "inherit".
 * Automatically queues a redraw.
 */
void cmk_widget_set_bevel_radius_multiplier(CmkWidget *widget, float multiplier);

/*
 * Gets the bevel radius multiplier, or inherits from the parent if -1.
 * If there is no parent to inherit from, returns 1.
 */
float cmk_widget_get_bevel_radius_multiplier(CmkWidget *widget);

/*
 * Sets the padding multiplier, which the widget may use if it needs to
 * add padding. Defaults to 1. Set to -1 for inherit. Automatically queues
 * a relayout.
 */
void cmk_widget_set_padding_multiplier(CmkWidget *widget, float multiplier);

/*
 * Gets the padding multiplier, or inherits from the parent if -1.
 * If there is no parent to inherit from, returns 1.
 */
float cmk_widget_get_padding_multiplier(CmkWidget *widget);

/*
 * Similar to calling clutter_actor_set_margin, but these margin
 * values are in dps, so the margin will scale with the widget's dp.
 */
void cmk_widget_set_margin(CmkWidget *widget, float left, float right, float top, float bottom);

/*
 * Convenience for
 * g_signal_emit_by_name(widget, "replace", replacement);
 */
void cmk_widget_replace(CmkWidget *widget, CmkWidget *replacement);

/*
 * Convenience for
 * g_signal_emit_by_name(widget, "back");
 */
void cmk_widget_back(CmkWidget *widget);

/*
 * Same as clutter_actor_add_child but casts for you.
 */
void cmk_widget_add_child(CmkWidget *widget, CmkWidget *child);

/*
 * Convenience method to use a ClutterConstraint so that this widget
 * always fills its parent's allocation.
 * The bind can be removed by removing the clutter constraint
 * "cmk-widget-bind-fill"
 */
void cmk_widget_bind_fill(CmkWidget *widget);

/*
 * Fades out the actor and then hides it.
 * Set destroy to TRUE to destroy the actor after the fade completes.
 */
void cmk_widget_fade_out(CmkWidget *widget, gboolean destroy);

/*
 * Shows the actor and fades it in.
 */
void cmk_widget_fade_in(CmkWidget *widget);

/*
 * If tabbable is TRUE, this widget can receive keyboard focus through
 * the tab key. Otherwise, this widget will be skipped.
 * This value defaults to false. Widget subclasses should automatically
 * call this if the widget is reactive. (Also call clutter_actor_set_reactive).
 */
void cmk_widget_set_tabbable(CmkWidget *widget, gboolean tabbable);

/*
 * Gets the value set in cmk_widget_set_tabbable.
 */
gboolean cmk_widget_get_tabbable(CmkWidget *widget);

void cmk_widget_push_tab_modal(CmkWidget *widget);
void cmk_widget_pop_tab_modal();
guint cmk_redirect_keyboard_focus(ClutterActor *actor, ClutterActor *redirection);
guint cmk_focus_on_mapped(ClutterActor *actor);
void cmk_focus_stack_push(CmkWidget *widget);
void cmk_focus_stack_pop(CmkWidget *widget);

/*
 * Convenience for ClutterColor -> RGBA -> cairo_set_source_rgba.
 */
void cairo_set_source_clutter_color(cairo_t *cr, const ClutterColor *color);

/*
 * Convenience for scaling a ClutterActorBox. Set move to TRUE to scale
 * its top left x,y coordinates as well as its size.
 */
void cmk_scale_actor_box(ClutterActorBox *b, float scale, gboolean move);

/*
 * Simple 1-topalpha blend.
 */
void clutter_color_blend(const ClutterColor *top, const ClutterColor *bottom, ClutterColor *out);

/*
 * TRUE to grab, FALSE to ungrab. This could possibly called with
 * multiple TRUEs in a row, so the implementation should use some
 * counting method to pair grabs to ungrabs.
 */
typedef void (*CmkGrabHandler) (gboolean grab, gpointer userdata);

/*
 * Set a handler to do extra functionality on grabs. This is useful
 * for window managers to grab all input, even over the area not
 * drawn by Clutter.
 */
void cmk_set_grab_handler(CmkGrabHandler handler, gpointer userdata);
void cmk_grab(gboolean grab);

G_END_DECLS

#endif
