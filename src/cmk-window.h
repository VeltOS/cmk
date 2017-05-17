/*
 * libcmk
 * Copyright (C) 2017 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_WINDOW_H__
#define __CMK_WINDOW_H__

#include "cmk-widget.h"

G_BEGIN_DECLS

/*
 * Creates a regular new window.
 * The size of the window will be determined by the size request
 * of the returned widget. This size does not include the window's
 * header bar, and is NOT directly affected by DPI scaling.
 *
 * Destroy the returned widget to close the window.
 */
CmkWidget * cmk_window_new(const gchar *title);

/*
 * Set window title
 */
void cmk_window_set_title(CmkWidget *window, const gchar *title);

/*
 * Get window title
 */
const gchar * cmk_window_get_title(CmkWidget *window);

/*
 * Enables or disables showing the header bar (including window
 * title, close buttons, etc).
 */
void cmk_window_set_show_header(CmkWidget *header, gboolean show);

G_END_DECLS

#endif
