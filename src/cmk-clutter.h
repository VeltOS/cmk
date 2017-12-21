/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMKCL_WIDGET_H__
#define __CMKCL_WIDGET_H__

#include <clutter/clutter.h>
#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMKCL_TYPE_WIDGET cmkcl_widget_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkclWidget, cmkcl_widget, CMKCL, WIDGET, ClutterActor);

/**
 * SECTION:cmkcl-widget
 * @TITLE: CmkclWidget
 * @SHORT_DESCRIPTION: ClutterActor wrapper around #CmkWidget
 */

typedef struct _CmkclWidgetClass CmkclWidgetClass;

/**
 * CmkclWidgetClass:
 */
struct _CmkclWidgetClass
{
	/*< private >*/
	ClutterActorClass parentClass;
	
	/*< public >*/
};

/**
 * cmkcl_widget_new:
 */
ClutterActor * cmkcl_widget_new();
