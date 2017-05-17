#include "cmk-widget.h"

/*
 * Forces the global scaling factor to 1, forces Xorg windowing
 * backend (because the Wayland and GDK-on-Wayland backends are
 * broken) and calls clutter_init.
 */
gboolean cmk_init(int *argc, char ***argv);

/*
 * Currently the same as clutter_main.
 */
void cmk_main(void);

/*
 * Automatically updates the default style widget's scaling factor
 * based on the global dconf scaling factor setting.
 */
void cmk_auto_dpi_scale(void);

/*
 * Convenience for creating a ClutterStage covered by a CmkWidget background.
 */
CmkWidget * cmk_window_new(const gchar *title, float width, float height, ClutterStage **stageptr);
