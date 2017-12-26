/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_GTK_WIDGET_H__
#define __CMK_GTK_WIDGET_H__

#include <gtk/gtk.h>
#include "cmk.h"

G_BEGIN_DECLS

/**
 * SECTION:cmk-gtk-widget
 * @TITLE: CmkGtkWidget
 * @SHORT_DESCRIPTION: GtkWidget wrapper around #CmkWidget
 */

#define CMK_TYPE_GTK_WIDGET cmk_gtk_widget_get_type()
G_DECLARE_FINAL_TYPE(CmkGtkWidget, cmk_gtk_widget, CMK, GTK_WIDGET, GtkWidget);

/**
 * cmk_widget_to_gtk:
 *
 * Creates a #CmkGtkWidget from a #CmkWidget. Once this has
 * been called, the value is stored and future calls will
 * return the same value. The CmkGtkWidget takes ownership of
 * the passed CmkWidget.
 *
 * Only one type of cast can be made per widget. That is,
 * you may not call cmk_widget_cast_gtk() and later
 * cmk_widget_cast_clutter().
 *
 * A shorter macro version, CMK_GTK(), is available.
 *
 * This might be used as such:
 *
 *   CmkWidget *button = cmk_button_new();
 *   gtk_container_add(container, CMK_GTK(button));
 *   cmk_button_set_text(button, "click me!");
 *   gtk_widget_...(CMK_GTK(button), ...);
 */
GtkWidget * cmk_widget_to_gtk(CmkWidget *widget);

/**
 * CMK_GTK:
 *
 * Shorter version of cmk_widget_to_gtk().
 */
#define CMK_GTK(cmkwidget) cmk_widget_to_gtk((CmkWidget *)(cmkwidget))

/**
 * cmk_get_widget_get_widget:
 *
 * Inverse of cmk_widget_to_gtk(). Takes a #CmkGtkWidget
 * and returns its #CmkWidget.
 */
CmkWidget * cmk_gtk_widget_get_widget(CmkGtkWidget *widget);

#endif
