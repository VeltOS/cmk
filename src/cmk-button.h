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

/**
 * SECTION:cmk-button
 * @TITLE: CmkButton
 * @SHORT_DESCRIPTION: A button widget
 *
 * A widget which acts like a button, allowing users to click. Receive
 * click events through the #CmkButton::activate signal. CmkButton
 * handles mouse grabs using the cmk_grab() method.
 *
 * Buttons can have multiple different styles based on the Material
 * Design specification. Currently, CmkButton supports a rectangular
 * button, a beveled rectangular button, and a circular button.
 * Buttons with images/icons can be created using cmk_button_set_content().
 *
 * TODO: Built-in shadow support coming soon. Until then, use the CmkShadow
 * class.
 */

typedef struct _CmkButtonClass CmkButtonClass;

struct _CmkButtonClass
{
	/*< private >*/
	CmkWidgetClass parentClass;
	
	/*< public >*/
	void (*activate) (CmkButton *self);
};

typedef enum
{
	CMK_BUTTON_TYPE_RECT,
	CMK_BUTTON_TYPE_BEVELED,
	CMK_BUTTON_TYPE_CIRCLE,
} CmkButtonType;

/**
 * cmk_button_new:
 *
 * Creates a new button with no text or content.
 */
CmkButton * cmk_button_new(void);
CmkButton * cmk_button_new_with_text(const gchar *text);
CmkButton * cmk_button_new_full(const gchar *text, CmkButtonType type);

/*
 * Sets the text of the button. Set %NULL for no text (useful for
 * image-only buttons).
 */
void cmk_button_set_text(CmkButton *button, const gchar *text);
const gchar * cmk_button_get_text(CmkButton *button);

/**
 * cmk_button_set_content:
 *
 * Sets the "content" of this button. This is usually used for buttons
 * with images. The content is displayed to the left of the text, or
 * if the button has no text, directly in the center.
 */
void cmk_button_set_content(CmkButton *button, CmkWidget *content);
CmkWidget * cmk_button_get_content(CmkButton *button);

void cmk_button_set_type(CmkButton *button, CmkButtonType type);

/**
 * cmk_button_get_btype:
 *
 * Gets the value set with cmk_button_set_type.
 * (cmk_button_get_type is an existing GObject-created method.)
 */
CmkButtonType cmk_button_get_btype(CmkButton *button);

/**
 * cmk_button_set_selected:
 *
 * Sets the button to be "selected", which changes the color slightly.
 * This is a bit of a hack used for Graphene's task bar right now,
 * and will probably be removed or replaced later.
 */
void cmk_button_set_selected(CmkButton *button, gboolean selected);

gboolean cmk_button_get_selected(CmkButton *button);

/**
 * cmk_button_get_name:
 *
 * Returns the button actor's name (clutter_actor_set_name) if it is set,
 * or the button's text otherwise.
 */
const gchar * cmk_button_get_name(CmkButton *button);

G_END_DECLS

#endif

