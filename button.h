/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_BUTTON_H__
#define __CMK_BUTTON_H__

#include <clutter/clutter.h>
#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_BUTTON cmk_button_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkButton, cmk_button, CMK, BUTTON, CmkWidget);

typedef struct _CmkButtonClass CmkButtonClass;

struct _CmkButtonClass
{
	CmkWidgetClass parentClass;

	void (*activate) (CmkButton *self);
};

CmkButton *cmk_button_new();
CmkButton *cmk_button_new_with_text(const gchar *text);

void cmk_button_set_text(CmkButton *button, const gchar *text);
const gchar * cmk_button_get_text(CmkButton *button);

/*
 * Returns the button actor's name (clutter_actor_set_name) if it is set,
 * or the button's text otherwise.
 */
const gchar * cmk_button_get_name(CmkButton *button);

G_END_DECLS

#endif

