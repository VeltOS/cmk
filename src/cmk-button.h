/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

/**
 * SECTION:cmk-button
 * @TITLE: CmkButton
 *
 *
 */

#ifndef __CMK_BUTTON_H__
#define __CMK_BUTTON_H__

#include "cmk-widget.h"
#include "cmk-label.h"

#define CMK_TYPE_BUTTON cmk_button_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkButton, cmk_button, CMK, BUTTON, GInitiallyUnowned);
typedef struct _CmkButtonClass CmkButtonClass;

/**
 * CmkButtonType:
 * @CMK_BUTTON_EMBED: No bevel, no background. Useful for menus.
 * @CMK_BUTTON_FLAT: Bevel but no background. Useful for popups.
 * @CMK_BUTTON_FLAT_CIRCLE: Flat, but circle shaped for icon-only buttons.
 * @CMK_BUTTON_RAISED: Bevel, background, and drop shadow.
 * @CMK_BUTTON_ACTION: Circular, bevel, background, larger drop shadow.
 */
typedef enum
{
	CMK_BUTTON_EMBED,
	CMK_BUTTON_FLAT,
	CMK_BUTTON_FLAT_CIRCLE,
	CMK_BUTTON_RAISED,
	CMK_BUTTON_ACTION,
} CmkButtonType;

struct _CmkButtonClass
{
	/*< private >*/
	CmkWidgetClass parentClass;
};

extern CmkButtonClass *cmk_button_class;

/**
 * cmk_button_new:
 *
 * Create a new CmkButton.
 */
CmkButton * cmk_button_new(CmkButtonType type);

/**
 * cmk_button_new_with_label:
 *
 * Create a new CmkButton with the given label text.
 */
CmkButton * cmk_button_new_with_label(CmkButtonType type, const char *label);

/**
 * cmk_button_set_label:
 *
 * Sets the label text on the button.
 */
void cmk_button_set_label(CmkButton *button, const char *label);

/**
 * cmk_button_get_label:
 *
 * Gets the label text.
 */
const char * cmk_button_get_label(CmkButton *button);

/**
 * cmk_button_get_label_widget:
 *
 * Returns: (transfer none): The #CmkLabel used for
 * displaying text in the button.
 */
CmkLabel * cmk_button_get_label_widget(CmkButton *button);

/**
 * cmk_button_activate:
 *
 * Simulate the user clicking the button or pressing enter on it.
 */
void cmk_button_activate(CmkButton *button);

#endif
