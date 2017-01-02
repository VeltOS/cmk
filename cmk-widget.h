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
G_DECLARE_DERIVABLE_TYPE(CMKWidget, cmk_widget, CMK, WIDGET, ClutterActor);

typedef struct _CMKWidgetClass CMKWidgetClass;

struct _CMKWidgetClass
{
	ClutterActorClass parentClass;
	
	/*
	 * Must be chained up to parent class. This signal is automatically
	 * emitted after the init function chain completes (during property
	 * construction). Until then, the widget will have the default style.
	 */
	void (*style_changed) (CMKWidget *self, CMKStyle *style);
};

/*
 * CMKWidget is not abstract. Creating a widget on its own is effectively
 * just a ClutterActor with CMKStyle styling properties.
 */
CMKWidget * cmk_widget_new();

void cmk_widget_set_style(CMKWidget *widget, CMKStyle *style);
CMKStyle * cmk_widget_get_style(CMKWidget *widget);

/*
 * Instead of setting this widget's style directly, attach it to another
 * widget. Whenever the "parent" widget's style object changes, this widget
 * will match it. This is mutually exclusive with cmk_widget_set_style,
 * however cmk_widget_get_style works with either style-setting method.
 *
 * CMKWidget does not retain a reference to its style parent. If the parent
 * is destroyed, the parent will be automatically removed and the widget
 * will return to the default style (cmk_style_get_default).
 */
void cmk_widget_set_style_parent(CMKWidget *widget, CMKWidget *parent);
CMKWidget * cmk_widget_get_style_parent(CMKWidget *widget);

void cmk_widget_set_background_color(CMKWidget *widget, const gchar *namedColor);
const gchar * cmk_widget_get_background_color(CMKWidget *widget);

G_END_DECLS

#endif
