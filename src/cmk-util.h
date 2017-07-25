#include "cmk-widget.h"

/**
 * SECTION:cmk-util
 * @TITLE: CmkUtil
 * @SHORT_DESCRIPTION: Utility methods for Cmk
 */

/**
 * cmk_init:
 *
 * Call in place of clutter_init().
 *
 * Forces the global scaling factor to 1, forces Xorg windowing
 * backend (because the Wayland and GDK-on-Wayland backends are
 * broken) and calls clutter_init.
 */
gboolean cmk_init(int *argc, char ***argv);

/**
 * cmk_main:
 *
 * Currently the same as clutter_main().
 */
void cmk_main(void);

/**
 * cmk_auto_dpi_scale:
 *
 * Automatically updates the widget's scaling factor
 * based on the global dconf scaling factor setting.
 * Automatically called on the CmkWidget returned by cmk_window_new.
 */
void cmk_auto_dpi_scale(CmkWidget *root);

/**
 * cmk_window_new:
 * @class: Window class. Used by some window managers as the window's icon name.
 *
 * Convenience for creating a ClutterStage covered by a CmkWidget background.
 */
CmkWidget * cmk_window_new(const gchar *title, const gchar *class, float width, float height, ClutterStage **stageptr);

/**
 * cmk_widget_get_window:
 *
 * If @widget is a child of a stage created by cmk_window_new(),
 * gets the #CmkWidget that represents the window.
 */
CmkWidget * cmk_widget_get_window(CmkWidget *widget);
