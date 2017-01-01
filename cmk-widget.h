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
	
	void (*style_changed) (CMKWidget *self, CMKStyle *style);
};

CMKStyle * cmk_widget_get_style(CMKWidget *widget);
void cmk_widget_set_style(CMKWidget *widget, CMKStyle *style);

G_END_DECLS

#endif
