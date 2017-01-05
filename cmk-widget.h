/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_WIDGET_H__
#define __CMK_WIDGET_H__

#include <clutter/clutter.h>
#include "style.h"

G_BEGIN_DECLS

#define CMK_TYPE_WIDGET cmk_widget_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkWidget, cmk_widget, CMK, WIDGET, ClutterActor);

typedef struct _CmkWidgetClass CmkWidgetClass;

struct _CmkWidgetClass
{
	ClutterActorClass parentClass;
	
	/*
	 * Emitted when a style property changes on the widget. This may include
	 * changes to the style of parent widgets. Automatically emitted during
	 * class property construction; until then, the widget has a NULL style.
	 * Must chain up to parent class.
	 */
	void (*style_changed) (CmkWidget *self, CmkStyle *style);

	/*
	 * Emitted when the value returned by cmk_widget_get_background_color
	 * changes. Must chain up to parent class.
	 */
	void (*background_changed) (CmkWidget *self);
};

/*
 * CmkWidget is not abstract. Creating a widget on its own is effectively
 * just a ClutterActor with CmkStyle styling properties.
 */
CmkWidget * cmk_widget_new();

/*
 * Set the widget's style. Set the style to NULL to inherit the parent's
 * style, and to receive style update signals from the parent. The default
 * value is to inherit (NULL) if the parent is a CmkWidget, and otherwise
 * to use the default style (cmk_style_get_default).
 */
void cmk_widget_set_style(CmkWidget *widget, CmkStyle *style);

/*
 * Returns the "actual" style of the widget. That is, if cmk_widget_set_style
 * has been called with a non-NULL value, this function returns that value.
 * Otherwise, this function returns the parent's style or the default style.
 * This is the value of the 'style' class property.
 */
CmkStyle * cmk_widget_get_actual_style(CmkWidget *widget);

/*
 * Returns the style, not taking into account the parent. This is always the
 * same value passed to cmk_widget_set_style.
 */
CmkStyle * cmk_widget_get_style(CmkWidget *widget);

/*
 * Instead of using the widget's actual parent, use a different widget to
 * inherit styles from. This only has any effect if the widget is
 * inheriting the parent's style. Set parent to NULL to unset.
 *
 * CmkWidget does not retain a reference to its style parent. If the parent
 * is destroyed, the "style parent" association will automatically be
 * removed and the widget will return to using its real parent or no parent.
 */
void cmk_widget_set_style_parent(CmkWidget *widget, CmkWidget *parent);
CmkWidget * cmk_widget_get_style_parent(CmkWidget *widget);

/*
 * Similar to clutter_actor_set_background_color, except it uses the named
 * color from the current style. Set to NULL to have a fully transparent
 * background (the default).
 * See cmk_widget_set_draw_background_color.
 */
void cmk_widget_set_background_color(CmkWidget *widget, const gchar *namedColor);

/*
 * If TRUE, cmk_widget_set_background_color will actually call
 * clutter_actor_set_background_color to fill in the actor. Otherwise,
 * the specified background color is just used for child actors calling
 * cmk_widget_get_background_color. (If not TRUE, you should probably
 * be drawing a background of that color yourself.)
 * Defaults to FALSE.
 */
void cmk_widget_set_draw_background_color(CmkWidget *widget, gboolean draw);

/*
 * Gets the effective background color of the widget. If no background
 * color for this widget has been set, the parent widget's get_background_color
 * method will be called. Foreground actors should base their color on this
 * background color. If this method returns NULL, the background color is
 * unknown, and foreground actors should use a default color.
 */
const gchar * cmk_widget_get_background_color(CmkWidget *widget);

G_END_DECLS

#endif
