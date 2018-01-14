/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

/**
 * SECTION:cmk-label
 * @TITLE: CmkLabel
 *
 * It displays text.
 */

#ifndef __CMK_LABEL_H__
#define __CMK_LABEL_H__

#include "cmk-widget.h"

#define CMK_TYPE_LABEL cmk_label_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkLabel, cmk_label, CMK, LABEL, GInitiallyUnowned);
typedef struct _CmkLabelClass CmkLabelClass;

struct _CmkLabelClass
{
	/*< private >*/
	CmkWidgetClass parentClass;
};

/**
 * cmk_label_new:
 * @text: The text of the label, or NULL.
 *
 * Create a new CmkLabel.
 */
CmkLabel * cmk_label_new(const char *text);

/**
 * cmk_label_new_bold:
 * @text: The text of the label, or NULL.
 *
 * Create a new, bold, CmkLabel.
 */
CmkLabel * cmk_label_new_bold(const char *text);

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
 */
void cmk_label_set_font_family(CmkLabel *label, const char *family);

/**
 * cmk_label_get_font_family:
 *
 * Gets the current font family name. This may return NULL
 * if family is set.
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
//void cmk_label_animate_font_size(CmkLabel *label, float size, float speed);

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
 * cmk_label_set_editable:
 *
 * Makes the text editable. This currently has limited
 * support and must be single line.
 */
void cmk_label_set_editable(CmkLabel *self, bool editable);

/**
 * cmk_label_get_editable:
 *
 * Gets if the label is editable.
 */
bool cmk_label_get_editable(CmkLabel *self);

/**
 * cmk_label_get_bold:
 *
 * Returns true if the font weight is >= PANGO_WEIGHT_BOLD.
 */
bool cmk_label_get_bold(CmkLabel *self);

/**
 * cmk_label_get_layout:
 *
 * If more detailed text handling is necessary, the #PangoLayout
 * may be acquired and edited. After making changes, call
 * cmk_widget_relayout() on the label to apply. Most other
 * #CmkLabel methods simply modify or query this layout object.
 *
 * The label's #PangoLayout may change, so don't ever keep a
 * reference to the return value of this function. If the layout
 * object changes, any values set (including values set to the
 * layout's #PangoFontDescription) are retained.
 *
 * pango_layout_set_width() and pango_layout_set_height() are
 * are called each draw cycle, so don't bother changing them.
 *
 * Return value: (transfer none): A #PangoLayout which can be edited.
 */
PangoLayout * cmk_label_get_layout(CmkLabel *label);

#endif
