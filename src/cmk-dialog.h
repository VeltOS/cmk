/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 *
 * This class creates a message dialog with button responses, but does not
 * handle displaying it to the user / centering it on screen.
 */

#ifndef __CMK_DIALOG_H__
#define __CMK_DIALOG_H__

#include "cmk-widget.h"
#include "cmk-button.h"

G_BEGIN_DECLS

#define CMK_TYPE_DIALOG  cmk_dialog_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkDialog, cmk_dialog, CMK, DIALOG, CmkWidget)

typedef struct _CmkDialogClass CmkDialogClass;

struct _CmkDialogClass
{
	CmkWidgetClass parentClass;
	
	void (*select) (CmkDialog *dialog, const gchar *selection);
};

/**
 * cmk_dialog_new:
 *
 * Creates a blank dialog. You may want to use cmk_dialog_show() to
 * present the dialog.
 */
CmkDialog * cmk_dialog_new(void);

/**
 * cmk_dialog_new_simple:
 *
 * Creates a dialog with a message, icon, and a variable number of buttons.
 * End the button list with a NULL.
 *
 * You may want to use cmk_dialog_show() to present the dialog.
 */
CmkDialog * cmk_dialog_new_simple(const gchar *message, const gchar *icon, ...);

/**
 * cmk_dialog_show:
 *
 * Present @dialog modally on the same stage that contains @parent.
 * The dialog will inherit Cmk style properties from @parent. It
 * is an error for @parent to not be attached a #ClutterStage
 * at the time of calling this method.
 *
 * This method also hooks to the #CmkDialog::select signal (connected
 * with the "after" flag) and automatically closes and destroys the
 * dialog when the user selects an option.
 *
 * CmkDialogs can be presented manually without using this method.
 */
void cmk_dialog_show(CmkDialog *dialog, CmkWidget *parent);

void cmk_dialog_set_content(CmkDialog *dialog, ClutterActor *content);
void cmk_dialog_set_message(CmkDialog *dialog, const gchar *message);

void cmk_dialog_set_buttons(CmkDialog *dialog, const gchar * const *buttons);

/**
 * cmk_dialog_get_buttons:
 *
 * Gets a list of the CmkButton actors that the dialog is using.
 * This can be used for making custom modifications to the buttons.
 */
GList * cmk_dialog_get_buttons(CmkDialog *dialog);

void cmk_dialog_set_icon(CmkDialog *dialog, const gchar *icon);
const gchar * cmk_dialog_get_icon(CmkDialog *dialog);

G_END_DECLS

#endif /* __CMK_DIALOG_H__ */
