/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_BUTTON_H__
#define __CMK_BUTTON_H__

#include "cmk-widget.h"

/**
 * SECTION:cmk-button
 * @TITLE: CmkButton
 *
 * CmkButtons can emit signals from #CmkWidget and
 *   "activate": The user clicked on the button or
 *      pressed enter. signaldata is NULL.
 */

typedef enum
{
	CMK_BUTTON_FLAT,
} CmkButtonType;

typedef struct
{
	/*< public >*/
	CmkWidget parent;

	/*< private >*/
	void *priv;
} CmkButton;

typedef struct
{
	/*< public >*/
	CmkWidgetClass parentClass;
} CmkButtonClass;

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
 * cmk_button_init:
 *
 * Initializes a CmkButton. For use when subclassing.
 */
void cmk_button_init(CmkButton *button, CmkButtonType type);

/**
 * cmk_button_set_label:
 *
 * Sets the label text on the button.
 */
void cmk_button_set_label(CmkButton *button, const char *label);

/**
 * cmk_button_activate:
 *
 * Simulate the user clicking the button or pressing enter on it.
 */
void cmk_button_activate(CmkButton *button);

#endif
