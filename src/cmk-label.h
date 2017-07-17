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
 * #PangoFontDescription; instead, use cmk_label_set_font_size()
 * or cmk_widget_set_dp_scale().
 *
 * Returns: (transfer none): The #ClutterText used by this CmkLabel.
 */
ClutterText * cmk_label_get_clutter_text(CmkLabel *label);

/**
 * cmk_label_set_font_size:
 * @size: The font size in dps, or -1 for system default.
 *
 * Sets the font size in dps. The default size is the system default.
 * The relationship between dps and pts is not exactly defined, but
 * it's normally a ratio of (96./72.). That is, 12pt = 16dp.
 *
 * Note that even on single-line labels, the height request of the
 * label will be slightly larger than the font size due to Pango
 * adding line spacing.
 */
void cmk_label_set_font_size(CmkLabel *label, gfloat size);

/*
 * cmk_label_set_font_size:
 * @size: The font size in dps, or -1 for system default.
 * @duration: The time to transition, in ms.
 *
 * Same as cmk_label_set_font_size(), except smoothly transitions
 * font size over @duration. If this is called while the font size
 * is already in progress transitioning, the animation will be
 * stopped where it is and start sizing to the new @size with the
 * new @duration. The exception to this is if @size is the same
 * value as the current animation final value, in which case
 * nothing happens.
 *
 * A notify signal will not be emitted on CmkLabel::size until
 * the animation completes. This function will have the exact
 * same effect as cmk_label_set_font_size() if @label is not mapped.
 */
void cmk_label_animate_font_size(CmkLabel *label, float size, guint duration);

/*
 * Gets the font size in dps.
 */
gfloat cmk_label_get_font_size(CmkLabel *label);

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

/*
 * cmk_label_set_editable:
 *
 * Equivelent to calling clutter_text_set_editable(),
 * clutter_actor_set_reactive(), and cmk_widget_set_tabbable()
 * on the underyling #ClutterText actor.
 */
void cmk_label_set_editable(CmkLabel *self, gboolean editable);

/*
 * cmk_label_set_no_spacing:
 *
 * By default Pango adds some spacing around each line of text
 * rendered in order to make room for characters like 'g', which
 * hang below the baseline. However, this extra room makes
 * allocating text more confusing: if you set the font size to 16,
 * you might end up with a height request of 20 depending on the
 * font used. (Note: This is NOT the spacing set with
 * pango_layout_set_spacing(), which is extra line padding.)
 *
 * This method removes the extra spacing around the text so that
 * if you set the font size to 16, you are guaranteed to have a
 * height request of 16*dpScale. HOWEVER, this only works on
 * single-line text; if there is more than one line, this will
 * break and you'll have a bad day.
 *
 * This also means that characters like 'g' will hang below the
 * allocation of the CmkLabel, so make sure you don't set it to
 * clip drawing to allocation bounds.
 */
void cmk_label_set_no_spacing(CmkLabel *label, gboolean nospacing);

G_END_DECLS

#endif
