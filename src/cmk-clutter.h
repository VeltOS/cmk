/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_CLUTTER_WIDGET_H__
#define __CMK_CLUTTER_WIDGET_H__

#include <clutter/clutter.h>
#include "cmk.h"

G_BEGIN_DECLS

/**
 * SECTION:cmk-clutter-widget
 * @TITLE: CmkClutterWidget
 * @SHORT_DESCRIPTION: ClutterActor wrapper around #CmkWidget
 */

#define CMK_TYPE_CLUTTER_WIDGET cmk_clutter_widget_get_type()
G_DECLARE_FINAL_TYPE(CmkClutterWidget, cmk_clutter_widget, CMK, CLUTTER_WIDGET, ClutterActor);

/**
 * cmk_widget_to_clutter:
 *
 * Creates a #CmkClutterWidget from a #CmkWidget. Once this has
 * been called, the value is stored and future calls will
 * return the same value. The CmkClutterWidget takes ownership of
 * the passed CmkWidget.
 *
 * Only one type of cast can be made per widget. That is,
 * you may not call cmk_widget_cast_clutter() and later
 * cmk_widget_cast_gtk().
 *
 * A shorter macro version, CMK_CLUTTER(), is available.
 *
 * This might be used as such:
 *
 *   CmkWidget *button = cmk_button_new();
 *   clutter_actor_add_child(parent, CMK_CLUTTER(button));
 *   cmk_button_set_text(button, "click me!");
 *   clutter_actor_...(CMK_CLUTTER(button), ...);
 */
ClutterActor * cmk_widget_to_clutter(CmkWidget *widget);

/**
 * CMK_CLUTTER:
 *
 * Shorter version of cmk_widget_to_clutter().
 */
#define CMK_CLUTTER(cmkwidget) cmk_widget_to_clutter((CmkWidget *)(cmkwidget))

/**
 * cmk_clutter_widget_get_widget:
 *
 * Inverse of cmk_widget_to_clutter(). Takes a #CmkClutterWidget
 * and returns its #CmkWidget.
 */
CmkWidget * cmk_clutter_widget_get_widget(CmkClutterWidget *widget);

#endif
