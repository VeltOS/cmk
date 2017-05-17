/*
 * libcmk
 * Copyright (C) 2017 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_MAIN_H__
#define __CMK_MAIN_H__

#include "cmk-widget.h"

G_BEGIN_DECLS

/*
 * Call when creating a Cmk client application, in place of clutter_init.
 * Do not call if using Cmk as part of an existing Clutter application, such
 * a mutter plugin; instead call cmk_post_init.
 *
 * Set wayland to allow Wayland as a backend; otherwise Xorg will be forced.
 */
gboolean cmk_init(gboolean wayland);

/*
 * Call in place of clutter_main (only if using cmk_init). All Cmk resources
 * are destroyed when this function returns.
 */
void cmk_main(void);

/*
 * Call this to enable automatic DPI scaling. This is automatically called
 * by cmk_init. If not using cmk_init, call before clutter_init is called.
 */
void cmk_auto_dpi_scale(void);

/*
 * Sets the pointer device's cursor icon using the current icon
 * theme and size (set with cmk_set_cursor_icon_theme, or defaults
 * if unset).
 * Set iconName to NULL to make cursor invisible.
 * TODO: Unimplemented
 */
void cmk_input_device_set_cursor_icon(ClutterInputDevice *device, const gchar *iconName);

/*
 * Sets the cursor icon of an input device to the given pixmap.
 * Set data to NULL to make cursor invisible.
 * TODO: Unimplemented
 */
void cmk_input_device_set_cursor_pixmap(ClutterInputDevice *device, void *data, guint width, guint height);

G_END_DECLS

#endif
