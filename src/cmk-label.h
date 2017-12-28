/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_LABEL_H__
#define __CMK_LABEL_H__

#include "cmk-widget.h"
#include <pango/pango.h>

/**
 * SECTION:cmk-label
 * @TITLE: CmkLabel
 *
 * It displays text.
 */

typedef struct
{
	/*< public >*/
	CmkWidget parent;

	/*< private >*/
	void *priv;
} CmkLabel;

typedef struct
{
	/*< public >*/
	CmkWidgetClass parentClass;
} CmkLabelClass;

extern CmkLabelClass *cmk_label_class;

/**
 * cmk_label_new:
 *
 * Create a new CmkLabel.
 */
CmkLabel * cmk_label_new(void);

/**
 * cmk_label_new_with_text:
 *
 * Create a new CmkLabel with the given text.
 */
CmkLabel * cmk_label_new_with_text(const char *text);

/**
 * cmk_label_new_full:
 *
 * Create a new, bold, CmkLabel.
 */
CmkLabel * cmk_label_new_bold(const char *text);

/**
 * cmk_label_init:
 *
 * Initializes a CmkLabel. For use when subclassing.
 */
void cmk_label_init(CmkLabel *label);

/**
 * cmk_label_set_text:
 *
 * Sets the text on the label in UTF-8.
 */
void cmk_label_set_text(CmkLabel *label, const char *text);

/**
 * cmk_label_set_markup:
 *
 * Sets the text on the label in UTF-8, but with markup support.
 */
void cmk_label_set_markup(CmkLabel *label, const char *markup);

/**
 * cmk_label_get_text:
 *
 * Gets the text on the label, in UTF-8.
 */
const char * cmk_label_get_text(CmkLabel *label);

/**
 * cmk_label_set_font_family:
 *
 * Sets the font family to use, or NULL for system default.
 *
 * The special value "mono" will correct to the default
 * system mono font.
 */
void cmk_label_set_font_family(CmkLabel *label, const char *family);

/**
 * cmk_label_get_font_family:
 *
 * Gets the current font family name.
 */
const char * cmk_label_get_font_family(CmkLabel *label);

/**
 * cmk_label_set_font_size:
 *
 * Sets the font size, in points. (calls pango_font_description_set_size()).
 * A negative size will result in system default font size.
 */
void cmk_label_set_font_size(CmkLabel *label, float size);

/**
 * cmk_label_animate_font_size:
 *
 * Smoothly adjusts the font size. The speed is in points
 * per second. So if the size was 12pt, taking it to 16pt
 * in 400ms would be a speed of (16 - 12) / 0.4 = 10.
 */
void cmk_label_animate_font_size(CmkLabel *label, float size, float speed);

/**
 * cmk_label_get_font_size:
 *
 * Gets the font size, in points (or, if
 * pango_font_description_set_absolute_size() has been
 * called on the #PangoLayout object returned from
 * cmk_label_get_layout(), this will return the absolute
 * size instead.)
 */
float cmk_label_get_font_size(CmkLabel *label);

/**
 * cmk_label_set_single_line:
 *
 * Enables or disables single-line mode. In single-line
 * mode, at most one line of text will be rendered.
 *
 * (This is achived by setting the PangoLayout's height
 * to 0 instead of the allocated height.)
 */
void cmk_label_set_single_line(CmkLabel *label, bool singleline);

/**
 * cmk_label_get_single_line:
 *
 * Gets if single-line mode is enabled or disabled.
 */
bool cmk_label_get_single_line(CmkLabel *label);

/**
 * cmk_label_set_alignment:
 *
 * Sets the text alignment.
 */
void cmk_label_set_alignment(CmkLabel *label, PangoAlignment alignment);

/**
 * cmk_label_set_bold:
 *
 * Use bold text (PANGO_WEIGHT_BOLD) or not (PANGO_WEIGHT_NORMAL).
 * More specific options can be set through the layout object (see
 * cmk_label_get_layout()).
 */
void cmk_label_set_bold(CmkLabel *label, bool bold);

/**
 * cmk_label_get_layout:
 *
 * If more detailed text handling is necessary, the #PangoLayout
 * may be acquired and edited. After making changes, call
 * cmk_widget_invalidate() on the label to apply. Most other
 * #CmkLabel methods simply modify or query this layout object.
 *
 * pango_layout_set_width() and pango_layout_set_height() are
 * are called each draw cycle, so don't bother changing them.
 *
 * Return value: (transfer none): A #PangoLayout which can be edited.
 */
PangoLayout * cmk_label_get_layout(CmkLabel *label);

/**
 * cmk_label_set_global_font_properties:
 *
 * Sets the global, default font properties. These should
 * be obtained from the system by the #CmkWidget wrapper.
 *
 * @resolution: Font dpi. Starts at 96.
 * @defaultFont: System default font. Starts at "sans 11".
 * @defaultMono: System default mono font. Starts at "mono 11".
 * @baseRTL: To use right-to-left as the default text direction.
 *      Starts at false.
 */
void cmk_label_set_global_font_properties(unsigned int resolution, const PangoFontDescription *defaultFont, const PangoFontDescription *defaultMono, bool baseRTL);

/**
 * cmk_label_get_global_font_properties:
 *
 * Get properties set with cmk_label_set_global_font_properties().
 * defaultFont and defaultMono will always be non-NULL; if they
 * were set to NULL, the original defaults are returned instead.
 */
void cmk_label_get_global_font_properties(unsigned int *resolution, const PangoFontDescription **defaultFont, const PangoFontDescription **defaultMono, bool *baseRTL);

typedef void (* CmkLabelGlobalPropertiesListenCallback) (void *userdata);

/**
 * cmk_label_global_properties_listen:
 *
 * Listen to calls to cmk_label_set_global_font_properties().
 * Pass NULL for callback to unlisten for the given userdata.
 */
void cmk_label_global_properties_listen(CmkLabelGlobalPropertiesListenCallback callback, void *userdata);

#endif
