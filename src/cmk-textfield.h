/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_TEXTFIELD_H__
#define __CMK_TEXTFIELD_H__

#include <clutter/clutter.h>
#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_TEXTFIELD cmk_textfield_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkTextfield, cmk_textfield, CMK, TEXTFIELD, CmkWidget);

/**
 * SECTION:cmk-textfield
 * @TITLE: CmkTextfield
 * @SHORT_DESCRIPTION: Widget for user input
 *
 * The "error" named color is used when an error message is set.
 */

typedef struct _CmkTextfieldClass CmkTextfieldClass;

/**
 * CmkTextfieldClass:
 * @changed: Class handler for the #CmkTextfield::changed signal.
 * @activate: Class handler for the #CmkTextfield::activate signal.
 */
struct _CmkTextfieldClass
{
	/*< private >*/
	CmkWidgetClass parentClass;
	
	/*< public >*/
	void (*changed) (CmkTextfield *self);
	
	void (*activate) (CmkTextfield *self);
};

/**
 * cmk_textfield_new:
 * @name: Name label to place above the text field, or NULL.
 * @description: Description label to place below the text field, or NULL.
 *
 * Creates a text field widget into which the user can type text.
 */
CmkTextfield * cmk_textfield_new(const gchar *name, const gchar *description);

/**
 * cmk_textfield_set_text:
 *
 * Sets the text of the textfield as if the user had typed it.
 */
void cmk_textfield_set_text(CmkTextfield *textfield, const gchar *text);

/**
 * cmk_textfield_get_text:
 *
 * Gets the text typed by the user.
 */
const gchar * cmk_textfield_get_text(CmkTextfield *textfield);

/**
 * cmk_textfield_set_name:
 *
 * Sets the name label. When the label does not have keyboard focus,
 * this shows where the text normally is. When the label gains keyboard
 * focus, the transitions to above the text.
 */
void cmk_textfield_set_name(CmkTextfield *textfield, const gchar *text);

/**
 * cmk_textfield_set_description:
 *
 * Sets the description label. This label shows below the input text.
 */
void cmk_textfield_set_description(CmkTextfield *textfield, const gchar *text);

/*
 * cmk_textfield_set_placeholder:
 *
 * Sets text to show when the user has clicked on the textfield
 * but before they type anything.
 */
void cmk_textfield_set_placeholder(CmkTextfield *textfield, const gchar *placeholder);

/*
 * cmk_textfield_set_error:
 * @error: Error message, or NULL to unset.
 *
 * Replaces the description with a temporary error message, and changes
 * the textfield baseline named color to "error" (or default solid red).
 * Set error to NULL to remove the error message.
 *
 * You should check the user's text input on #ClutterActor::key-focus-out
 * and #CmkTextfield::activate events to see if the input is valid and
 * call this method accordingly.
 */
void cmk_textfield_set_error(CmkTextfield *textfield, const gchar *error);

/**
 * cmk_textfield_set_show_clear:
 *
 * Enable or disable the clear button on the right side of the text field.
 */
void cmk_textfield_set_show_clear(CmkTextfield *textfield, gboolean show);

/*
 * cmk_textfield_set_is_password:
 *
 * Use this to hide the input with dots on sensitive input fields.
 */
void cmk_textfield_set_is_password(CmkTextfield *textfield, gboolean isPassword);

G_END_DECLS

#endif

