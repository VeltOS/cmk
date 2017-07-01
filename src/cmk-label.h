/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_LABEL_H__
#define __CMK_LABEL_H__

#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_LABEL cmk_label_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkLabel, cmk_label, CMK, LABEL, CmkWidget);

/**
 * SECTION:cmk-label
 * @TITLE: CmkLabel
 * @SHORT_DESCRIPTION: A label widget
 *
 * A ClutterText wrapper but following CmkWidget style properties.
 * This includes proper font size handling with DPI scaling, which ClutterText
 * doesn't get right.
 */

typedef struct _CmkLabelClass CmkLabelClass;

struct _CmkLabelClass
{
	/*< private >*/
	CmkWidgetClass parentClass;
};

/**
 * cmk_label_new:
 *
 * Creates an empty label with no text.
 */
CmkLabel * cmk_label_new(void);
CmkLabel * cmk_label_new_with_text(const gchar *text);
CmkLabel * cmk_label_new_full(const gchar *text, gboolean bold);

/**
 * cmk_label_get_clutter_text:
 *
 * Gets the ClutterText used by this CmkLabel.
 *
 * Do not manually modify the #ClutterText's font size through its
 * #PangoFontDescription; instead, use cmk_label_set_font_size_pt()
 * or cmk_widget_set_dp_scale().
 *
 * Returns: (transfer none): The #ClutterText used by this CmkLabel.
 */
ClutterText * cmk_label_get_clutter_text(CmkLabel *label);

/**
 * cmk_label_set_font_size_pt:
 *
 * Sets the font size in points. The default size is Clutter's default.
 * Use -1 for size to use default size.
 * px = pt * (96.0/72.0) * [dp scale]
 */
void cmk_label_set_font_size_pt(CmkLabel *label, gfloat pt);

gfloat cmk_label_get_font_size_pt(CmkLabel *label);

/**
 * cmk_label_set_bold:
 *
 * Sets a font weight of %PANGO_WEIGHT_BOLD if %TRUE or
 * PANGO_WEIGHT_NORMAL if %FALSE.
 */
void cmk_label_set_bold(CmkLabel *label, gboolean bold);

gboolean cmk_label_get_bold(CmkLabel *label);

/**
 * cmk_label_set_text:
 *
 * Equivelent to calling clutter_text_set_text() on the underlying
 * #ClutterText actor.
 */
void cmk_label_set_text(CmkLabel *label, const gchar *val);

/**
 * cmk_label_set_markup:
 *
 * Equivelent to calling clutter_text_set_markup() on the underlying
 * #ClutterText actor.
 */
void cmk_label_set_markup(CmkLabel *label, const gchar *markup);

/**
 * cmk_label_get_text:
 *
 * Equivelent to calling clutter_text_get_text() on the underlying
 * #ClutterText actor.
 */
const gchar * cmk_label_get_text(CmkLabel *label);

/**
 * cmk_label_set_line_alignment:
 *
 * Equivelent to calling clutter_text_set_line_alignment() on the underlying
 * #ClutterText actor.
 */
void cmk_label_set_line_alignment(CmkLabel *label, PangoAlignment alignment);

G_END_DECLS

#endif
