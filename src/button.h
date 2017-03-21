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

typedef enum
{
	CMK_BUTTON_TYPE_RECT,
	CMK_BUTTON_TYPE_BEVELED,
	CMK_BUTTON_TYPE_CIRCLE,
} CmkButtonType;

CmkButton * cmk_button_new(void);
CmkButton * cmk_button_new_with_text(const gchar *text);
CmkButton * cmk_button_new_full(const gchar *text, CmkButtonType type);

void cmk_button_set_text(CmkButton *button, const gchar *text);
const gchar * cmk_button_get_text(CmkButton *button);

void cmk_button_set_content(CmkButton *button, CmkWidget *content);
CmkWidget * cmk_button_get_content(CmkButton *button);

void cmk_button_set_type(CmkButton *button, CmkButtonType type);
CmkButtonType cmk_button_get_btype(CmkButton *button); // Conflicts with GObject, so add b

void cmk_button_set_selected(CmkButton *button, gboolean selected);
gboolean cmk_button_get_selected(CmkButton *button);

/*
 * Returns the button actor's name (clutter_actor_set_name) if it is set,
 * or the button's text otherwise.
 */
const gchar * cmk_button_get_name(CmkButton *button);

G_END_DECLS

#endif

