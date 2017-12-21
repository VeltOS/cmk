/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_WIDGET_H__
#define __CMK_WIDGET_H__

#include <glib-object.h>
#include <cairo/cairo.h>
#include "cmk-event.h"

G_BEGIN_DECLS

/**
 * SECTION:cmk-widget
 * @TITLE: CmkWidget
 * @SHORT_DESCRIPTION: The base Widget class of Cmk
 */

#define CMK_TYPE_WIDGET cmk_widget_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkWidget, cmk_widget, CMK, WIDGET, GInitiallyUnowned);

/**
 * CmkWidgetClass:
 * @invalidate: Class handler for the #CmkWidget::invalidate signal.
 * @relayout: Class handler for the #CmkWidget::relayout signal.
 *            Subclasses must chain up.
 * @draw: Draw handler. Chain up to this before doing your own
 *        drawing to get standard widget effects.
 * @event: User input event handler. Return TRUE if the widget
 *         captured and responded to the event, FALSE to propagate
 *         the event.
 * @get_preferred_width: Preferred width handler.
 * @get_preferred_height: Preferred height handler.
 * @disable: Disabled event handler.
 */
struct _CmkWidgetClass
{
	/*< private >*/
	GObjectClass parentClass;
	
	/*< public >*/
	void (*invalidate) (CmkWidget *self, const cairo_region_t *region);
	void (*relayout) (CmkWidget *self);
	void (*draw) (CmkWidget *self, cairo_t *cr);
	gboolean (*event) (CmkWidget *self, const CmkEvent *event);
	void (*get_preferred_width) (CmkWidget *self, float forHeight, float *min, float *nat);
	void (*get_preferred_height) (CmkWidget *self, float forWidth, float *min, float *nat);
	void (*disable) (CmkWidget *self, gboolean disabled);
};
typedef struct _CmkWidgetClass CmkWidgetClass;

/**
 * cmk_widget_new:
 *
 * Creates a blank widget.
 */
CmkWidget * cmk_widget_new(void);

/**
 * cmk_widget_set_wrapper:
 *
 * Sets the toolkit-specific wrapper object.
 * This may only be called once on a widget.
 */
void cmk_widget_set_wrapper(CmkWidget *widget, void *wrapper);

/**
 * cmk_widget_get_wrapper:
 *
 * Gets the toolkit-specific wrapper object
 * set by cmk_widget_set_wrapper().
 */
void * cmk_widget_get_wrapper(CmkWidget *widget);

/**
 * cmk_widget_invalidate:
 *
 * Invalidates (requests a redraw) for a rectangle of the widget.
 * For use by CmkWidget subclasses. Controlling code should listen
 * to the #CmkWidget::invalidate signal. Set region to NULL to
 * redraw the entire widget.
 */
void cmk_widget_invalidate(CmkWidget *self, const cairo_region_t *region);

/**
 * cmk_widget_relayout:
 *
 * Indicates that the size request of the widget has changed,
 * and that a layout should be performed. For use by CmkWidget
 * subclasses. Controlling code should listen to the
 * #CmkWidget::relayout signal.
 */
void cmk_widget_relayout(CmkWidget *self);

/**
 * cmk_widget_draw:
 *
 * Draws the widget at the current origin of the cairo context.
 */
void cmk_widget_draw(CmkWidget *widget, cairo_t *cr);

/**
 * cmk_widget_event:
 *
 * Emits an event on this widget. Coordinates should be relative
 * to this widget's origin.
 */
gboolean cmk_widget_event(CmkWidget *widget, const CmkEvent *event);

/**
 * cmk_widget_get_preferred_width:
 *
 * Gets the preferred minimum and natural width.
 * Safe to pass NULL for min and nat, if those values
 * are not desired.
 */
void cmk_widget_get_preferred_width(CmkWidget *widget, float forHeight, float *min, float *nat);

/**
 * cmk_widget_get_preferred_height:
 *
 * Gets the preferred minimum and natural height.
 * Safe to pass NULL for min and nat, if those values
 * are not desired.
 */
void cmk_widget_get_preferred_height(CmkWidget *widget, float forWidth, float *min, float *nat);

/**
 * cmk_widget_set_size:
 *
 * Sets the actual size, in cairo units, of the widget.
 * This lets the widget know how large it can draw its view.
 *
 * Set width and/or height to less-than-zero to unset the value.
 *
 * The widget may render outside its size, especially if the
 * size is less than the minimum requested width or height. No
 * automatic clipping is performed.
 */
void cmk_widget_set_size(CmkWidget *widget, float width, float height);

/**
 * cmk_widget_get_size:
 *
 * Gets the size of the widget. If the width and/or height have been
 * set with cmk_widget_set_size(), those values are returned. Otherwise,
 * the preferred natural width and height are returned.
 */
void cmk_widget_get_size(CmkWidget *widget, float *width, float *height);

/**
 * cmk_widget_set_disabled:
 *
 * Sets or unsets the disabled flag, and invalidates the widget.
 */
void cmk_widget_set_disabled(CmkWidget *widget, gboolean disabled);

/**
 * cmk_widget_get_disabled:
 */
gboolean cmk_widget_get_disabled(CmkWidget *widget);

#endif
