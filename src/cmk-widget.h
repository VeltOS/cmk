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

/**
 * SECTION:cmk-widget
 * @TITLE: CmkWidget
 * @SHORT_DESCRIPTION: The base Widget class of Cmk
 *
 * CmkWidget is the base widget class of Cmk. When created on its own, it
 * is little more than a #ClutterActor with a few fancy extra properties
 * like named colors.
 *
 * The useful part is subclassing CmkWidget, as the subclass can inherit
 * style properties such as padding, density-independent pixels scale (dps),
 * and named colors, which can be used for drawing.
 *
 * Style properties are inherited down the actor graph, and child widgets
 * automatically receive notifications through the styles_changed class method
 * when properties in their parents have changed. Some style properties
 * completely override inherited values when set (such as padding) but others
 * combine with their parents' values (such as dp). See the getters and
 * setters for each property for more details.
 *
 * Style properties are inherited automatically from the widget's parent if
 * the widget's direct parent is a CmkWidget. However, if it is not, you must
 * manually specify a widget to inherit properties from using
 * cmk_widget_set_style_parent(). Therefore, using #CmkWidgets in place of
 * #ClutterActors for simple container actors with no logic is still useful.
 *
 * CmkWidgets also have the feature of 'replacement', using the
 * #CmkWidget::replace and #CmkWidget::back signals. If the widget's parent
 * supports it, a widget subclass or controller can emit one of these signals
 * on itself/the widget to have the widget's parent hide the widget and
 * replace it with a new one. This is similar to the idea of Fragments and
 * a Fragment stack in Android, and is great for sequences of dialogs showed
 * to the user, or for widgets with subdialogs.
 */

/**
 * CMK_DP:
 * @dps: A measurement in dps
 *
 * Converts dps to pixels. This should be used when drawing raw pixels to
 * a canvas or when allocating child widgets.
 */
#define CMK_DP(widget, dps) (cmk_widget_style_get_dp_scale(widget)*dps)

/**
 * CmkStyleFlag:
 *
 * Flags for all #CmkWidget style properties.
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

/**
 * CmkWidgetClass:
 * @styles_changed: Class handler for the #CmkWidget::styles-changed signal.
 *        Override this method to receive style update notifications. Chain
 *        up to the parent class immediately, BEFORE updating your widget.
 * @key_focus_changed: Class handler for the #CmkWidget::key-focus-changed
 *        signal. Chain up to the parent class if the event goes unhandled.
 * @replace: Class handler for #CmkWidget::replace signal. You probably don't
 *        need to override this.
 * @back: Class handler for #CmkWidget::back signal. You probably don't
 *        need to override this.
 */
struct _CmkWidgetClass
{
	/*< private >*/
	ClutterActorClass parentClass;
	
	/*< public >*/
	void (*styles_changed) (CmkWidget *self, guint flags);
	
	void (*key_focus_changed) (CmkWidget *self, ClutterActor *newfocus);
	
	void (*replace) (CmkWidget *self, CmkWidget *replacement);
	
	void (*back) (CmkWidget *self);
};

typedef struct _CmkNamedColor CmkNamedColor;

/**
 * _CmkNamedColor:
 * @name: Name of color
 * @color: Color
 *
 * A named color for use with cmk_widget_set_named_colors().
 */
struct _CmkNamedColor
{
	const gchar *name;
	ClutterColor color;
};

/**
 * cmk_widget_new:
 *
 * #CmkWidget is not abstract. Creating a widget on its own is effectively
 * just a #ClutterActor with styling properties.
 */
CmkWidget * cmk_widget_new();

/**
 * cmk_widget_destroy:
 *
 * Exactly the same as clutter_actor_destroy(), except returns #G_SOURCE_REMOVE
 * so this can easily be called from timeouts. Safe to pass in a regular
 * #ClutterActor* casted to a #CmkWidget*.
 */
gboolean cmk_widget_destroy(CmkWidget *widget);

/**
 * CmkWidget::tabbable:
 *
 * Whether or not this widget can receive keyboard focus through
 * tabbing.
 */

/**
 * cmk_widget_set_style_parent:
 *
 * Instead of using the widget's actual parent, use a different widget to
 * inherit styles from. Set to %NULL to return to using the actual parent.
 *
 * If the parent is destroyed, the "style parent" association will
 * automatically be removed and the widget will return to using its real
 * parent or no parent.
 */
void cmk_widget_set_style_parent(CmkWidget *widget, CmkWidget *parent);

/**
 * cmk_widget_get_style_parent:
 *
 * Gets the current style parent. If cmk_widget_set_style_parent() has been
 * called, this returns that value. Otherwise, this returns the widget's
 * current actual parent if it is a #CmkWidget or %NULL otherwise.
 */
CmkWidget * cmk_widget_get_style_parent(CmkWidget *widget);

/**
 * cmk_widget_set_named_color:
 *
 * Sets a named color that can be used to draw the widget. This color will be
 * inherited by child #CmkWidgets.
 */
void cmk_widget_set_named_color(CmkWidget *widget, const gchar *name, const ClutterColor *color);

/**
 * cmk_widget_set_named_colors:
 *
 * Takes an array of #CmkNamedColor (last one must be a NULLed
 * struct) and calls cmk_widget_set_named_color() on each.
 */
void cmk_widget_set_named_colors(CmkWidget *widget, const CmkNamedColor *colors);

/**
 * cmk_widget_get_named_color:
 * @name: Name of color to get
 *
 * Gets a named color set with cmk_widget_set_named_color(). If it is not
 * found, an inherited value is returned. If there is no inherited value,
 * %NULL is returned.
 */
const ClutterColor * cmk_widget_get_named_color(CmkWidget *widget, const gchar *name);

/**
 * cmk_widget_set_background_color:
 * @namedColor: A color set with cmk_widget_set_named_color() on this widget
 * any of its parents.
 *
 * Similar to clutter_actor_set_background_color(), except it uses the named
 * color from the current style. Set to %NULL to have a fully transparent
 * background (the default).
 * See cmk_widget_set_draw_background_color().
 */
void cmk_widget_set_background_color(CmkWidget *widget, const gchar *namedColor);

/**
 * cmk_widget_get_background_color:
 *
 * Gets the effective background color of the widget. If no background
 * color for this widget has been set, the parent widget's get_background_color
 * method will be called. Foreground actors should base their color on this
 * background color. If this method returns %NULL, the background color is
 * unknown, and foreground actors should use a default color.
 */
const gchar * cmk_widget_get_background_color(CmkWidget *widget);

/**
 * cmk_widget_get_background_clutter_color:
 *
 * Similar to calling cmk_widget_get_background_color() and
 * cmk_widget_get_named_color(), except returns solid white if no color is found.
 */
const ClutterColor * cmk_widget_get_background_clutter_color(CmkWidget *widget);

/**
 * cmk_widget_set_draw_background_color:
 *
 * If %TRUE, #CmkWidget will draw the color set in
 * cmk_widget_set_background_color(). Otherwise, you should draw it.
 * Defaults to %FALSE.
 */
void cmk_widget_set_draw_background_color(CmkWidget *widget, gboolean draw);

/**
 * cmk_widget_get_foreground_clutter_color:
 *
 * Attempts to find a foreground color from the current background color.
 * This first tries to use the named color "<background name>-foreground",
 * then tries "foreground", and if both fail, returns solid black.
 */
const ClutterColor * cmk_widget_get_foreground_clutter_color(CmkWidget *widget);

/**
 * cmk_widget_set_dp_scale:
 * @dp: Pixels per 1 dp.
 *
 * Sets the number of pixels per 1 dp. Defaults to 1. Note that this
 * value is then scaled by parents in the actor graph (hence the "scale")
 * 
 * dps are "density-independent pixels", similar to ems in HTML.
 * Most Cmk functions that take measurements use dps.
 */
void cmk_widget_set_dp_scale(CmkWidget *widget, float dp);

/**
 * cmk_widget_get_dp_scale:
 *
 * Gets the number of pixels per 1 dp. This value is scaled by the widget
 * graph; therefore, calling cmk_widget_set_dp_scale(2) on the root
 * widget will scale all widgets below it by 2.
 *
 * When drawing a widget, remember to use dps instead of pixel values.
 * This can be done easily using the CMK_DP macro. Values in ClutterActorBox,
 * passed to widgets' allocation functions, must be in pixels.
 */
float cmk_widget_get_dp_scale(CmkWidget *widget);

/**
 * cmk_widget_set_bevel_radius_multiplier:
 *
 * Sets the bevel radius multiplier, which the widget's draw method may
 * use if it draws bevels. Defaults to 1. Set to - 1 for "inherit".
 * Automatically queues a redraw.
 */
void cmk_widget_set_bevel_radius_multiplier(CmkWidget *widget, float multiplier);

/**
 * cmk_widget_get_bevel_radius_multiplier:
 *
 * Gets the bevel radius multiplier, or inherits from the parent if -1.
 * If there is no parent to inherit from, returns 1.
 */
float cmk_widget_get_bevel_radius_multiplier(CmkWidget *widget);

/**
 * cmk_widget_set_padding_multiplier:
 *
 * Sets the padding multiplier, which the widget may use if it needs to
 * add padding. Defaults to 1. Set to -1 for inherit. Automatically queues
 * a relayout.
 */
void cmk_widget_set_padding_multiplier(CmkWidget *widget, float multiplier);

/**
 * cmk_widget_get_padding_multiplier:
 *
 * Gets the padding multiplier, or inherits from the parent if -1.
 * If there is no parent to inherit from, returns 1.
 */
float cmk_widget_get_padding_multiplier(CmkWidget *widget);

/**
 * cmk_widget_set_margin:
 *
 * Similar to calling clutter_actor_set_margin(), but these margin
 * values are in dps, so the margin will scale with the widget's dp.
 */
void cmk_widget_set_margin(CmkWidget *widget, float left, float right, float top, float bottom);

/**
 * cmk_widget_replace:
 * @widget: The widget to be replaced
 * @replacement: The new widget
 *
 * Emits a signal requesting widget's parent to replace this widget with
 * a new widget. Similar to the idea of Fragments in Android. See
 * cmk_widget_back().
 *
 * Convenience for
 * g_signal_emit_by_name(widget, "replace", replacement);
 */
void cmk_widget_replace(CmkWidget *widget, CmkWidget *replacement);

/**
 * cmk_widget_back:
 *
 * Emits a signal requesting widget's parent to replace this widget with the
 * previously shown widget. See cmk_widget_replace().
 *
 * Convenience for
 * g_signal_emit_by_name(widget, "back");
 */
void cmk_widget_back(CmkWidget *widget);

/**
 * cmk_widget_add_child:
 *
 * Same as clutter_actor_add_child() but casts for you.
 */
void cmk_widget_add_child(CmkWidget *widget, CmkWidget *child);

/**
 * cmk_widget_bind_fill:
 *
 * Convenience method to use a #ClutterConstraint so that this widget
 * always fills its parent's allocation.
 * The bind can be removed by removing the clutter constraint
 * "cmk-widget-bind-fill"
 */
void cmk_widget_bind_fill(CmkWidget *widget);

/**
 * cmk_widget_fade_out:
 * @destroy: %TRUE to destroy the actor after the fade completes
 *
 * Fades out the actor and then hides it.
 */
void cmk_widget_fade_out(CmkWidget *widget, gboolean destroy);

/**
 * cmk_widget_fade_in:
 *
 * Shows the actor and fades it in.
 */
void cmk_widget_fade_in(CmkWidget *widget);

/**
 * cmk_widget_set_tabbable:
 *
 * If tabbable is %TRUE, this widget can receive keyboard focus through
 * the tab key. Otherwise, this widget will be skipped.
 * This value defaults to false. Widget subclasses should automatically
 * call this if the widget is reactive. (Also call clutter_actor_set_reactive()).
 */
void cmk_widget_set_tabbable(CmkWidget *widget, gboolean tabbable);

/**
 * cmk_widget_get_tabbable:
 *
 * Gets the value set in cmk_widget_set_tabbable().
 */
gboolean cmk_widget_get_tabbable(CmkWidget *widget);

/**
 * cmk_widget_push_tab_modal:
 *
 * In development.
 */
void cmk_widget_push_tab_modal(CmkWidget *widget);

/**
 * cmk_widget_pop_tab_modal:
 *
 * In development.
 */
void cmk_widget_pop_tab_modal();

/**
 * cmk_redirect_keyboard_focus:
 *
 * Whenever actor receives keyboard focus, it is immediately redirected
 * to redirection.
 */
guint cmk_redirect_keyboard_focus(ClutterActor *actor, ClutterActor *redirection);

/**
 * cmk_focus_on_mapped:
 *
 * Grabs keyboard focus for this widget whenever it becomes mapped.
 * Useful for popups.
 */
guint cmk_focus_on_mapped(ClutterActor *actor);

/**
 * cmk_focus_stack_push:
 *
 * In development.
 */
void cmk_focus_stack_push(CmkWidget *widget);

/**
 * cmk_focus_stack_pop:
 *
 * In development.
 */
void cmk_focus_stack_pop(CmkWidget *widget);

/**
 * cairo_set_source_clutter_color:
 *
 * Convenience for ClutterColor -> RGBA -> cairo_set_source_rgba().
 */
void cairo_set_source_clutter_color(cairo_t *cr, const ClutterColor *color);

/**
 * cmk_scale_actor_box:
 * @box: #ClutterActorBox to scale
 * @scale: Amount to scale
 * @move: %TRUE to scale x,y coordinates too; %FALSE to only scale size
 *
 * Convenience for scaling a #ClutterActorBox.
 */
void cmk_scale_actor_box(ClutterActorBox *box, float scale, gboolean move);

/**
 * clutter_color_blend:
 *
 * Simple 1-topalpha blend.
 */
void clutter_color_blend(const ClutterColor *top, const ClutterColor *bottom, ClutterColor *out);

/**
 * CmkGrabHandler:
 * @grab: %TRUE when a grab has been started; %FALSE when a grab has stopped
 *
 * This could possibly called with multiple TRUEs in a row, so the
 * implementation should use some counting method to pair grabs to ungrabs.
 */
typedef void (*CmkGrabHandler) (gboolean grab, gpointer userdata);

/**
 * cmk_set_grab_handler:
 *
 * Set a handler to do extra functionality on grabs. This is useful
 * for window managers to grab all input, even over the area not
 * drawn by Clutter.
 */
void cmk_set_grab_handler(CmkGrabHandler handler, gpointer userdata);

/**
 * cmk_grab:
 * @grab: Set %TRUE to enable grab, %FALSE to disable
 *
 * Starts or stops a grab. Every call with grab set to %TRUE must be
 * paired with a call to grab set to %FALSE.
 */
void cmk_grab(gboolean grab);

G_END_DECLS

#endif
