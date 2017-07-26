/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_SCROLL_BOX_H__
#define __CMK_SCROLL_BOX_H__

#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_SCROLL_BOX cmk_scroll_box_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkScrollBox, cmk_scroll_box, CMK, SCROLL_BOX, CmkWidget);

typedef struct _CmkScrollBoxClass CmkScrollBoxClass;

/**
 * SECTION:cmk-scroll-box
 * @TITLE: CmkScrollBox
 * @SHORT_DESCRIPTION: A scrolling viewport widget
 *
 * A replacement for #ClutterScrollActor. This widget uses keyboard/mouse
 * input to scroll its content (child actors) vertically and/or horizontally,
 * and will automatically stop when it reaches the edge of the content.
 *
 * This container has other visual effects such as dropshadow when scrolling
 * and scroll bars.
 *
 * TODO: Make scroll bars draggable with mouse.
 */

struct _CmkScrollBoxClass
{
	/*< private >*/
	CmkWidgetClass parentClass;
};

/**
 * cmk_scroll_box_new:
 *
 * Creates a #CmkScrollBox actor which can scroll vertically,
 * horizontally, or both.
 */
CmkScrollBox * cmk_scroll_box_new(ClutterScrollMode scrollMode);

/**
 * cmk_scroll_box_set_show_scrollbars:
 *
 * Enable or disable scrollbars. Default enabled.
 */
void cmk_scroll_box_set_show_scrollbars(CmkScrollBox *box, gboolean show);

/**
 * cmk_scroll_box_set_use_shadow:
 * @l: Shadow on inner left edge
 * @r: Shadow on inner right edge
 * @t: Shadow on inner top edge
 * @b: Shadow on inner bottom edge
 *
 * Set whether to use an inner shadow on each of the given edges
 * to represent scrolling. Shadow is only shown when not scrolled
 * all the way to that edge.
 */
void cmk_scroll_box_set_use_shadow(CmkScrollBox *box, gboolean l, gboolean r, gboolean t, gboolean b);

void cmk_scroll_box_scroll_to_bottom(CmkScrollBox *box);

G_END_DECLS

#endif
