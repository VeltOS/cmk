/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 *
 * A ClutterText wrapper but following CmkWidget style properties.
 * This includes proper font size handling with DPI scaling, which ClutterText
 * doesn't get right.
 */

#ifndef __CMK_LABEL_H__
#define __CMK_LABEL_H__

#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_LABEL cmk_label_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkLabel, cmk_label, CMK, LABEL, CmkWidget);

typedef struct _CmkLabelClass CmkLabelClass;

struct _CmkLabelClass
{
	CmkWidgetClass parentClass;
};

CmkLabel * cmk_label_new(void);
CmkLabel * cmk_label_new_with_text(const gchar *text);
CmkLabel * cmk_label_new_full(const gchar *text, gfloat scale, gboolean bold);

/*
 * Gets the ClutterText used by this CmkLabel. [transfer none]
 *
 * Do not manually modify the ClutterText's font size through its
 * PangoFontDescription; instead, use cmk_label_set_font_size_pt
 * or cmk_label_set_font_scale.
 */
ClutterText * cmk_label_get_clutter_text(CmkLabel *label);

/*
 * Sets the font size in points. The default size is Clutter's default.
 * Use -1 for size to use default size.
 * px = pt * (96.0/72.0) * fontScale * [cmk dpi scale]
 */
void cmk_label_set_font_size_pt(CmkLabel *label, gfloat pt);

gfloat cmk_label_get_font_size_pt(CmkLabel *label);

/*
 * Sets the font scale. This is a similar concept to ems. Defaults to 1.
 * This will probably become a CmkWidget style property eventually.
 *
 * This is preferred over cmk_label_set_font_size_pt, because it takes
 * into account the user's global font size preference.
 */
void cmk_label_set_font_scale(CmkLabel *label, gfloat fontScale);

gfloat cmk_label_get_font_scale(CmkLabel *label);

/*
 * Sets a font weight of PANGO_WEIGHT_BOLD or PANGO_WEIGHT_NORMAL.
 */
void cmk_label_set_bold(CmkLabel *label, gboolean bold);

gboolean cmk_label_get_bold(CmkLabel *label);

/*
 * The following are convenience functions that map directly
 * to the internal ClutterText's version.
 */

void cmk_label_set_text(CmkLabel *label, const gchar *val);
void cmk_label_set_markup(CmkLabel *label, const gchar *markup);
const gchar * cmk_label_get_text(CmkLabel *label);
void cmk_label_set_line_alignment(CmkLabel *label, PangoAlignment alignment);

G_END_DECLS

#endif
