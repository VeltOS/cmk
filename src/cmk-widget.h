/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

/**
 * SECTION:cmk-widget
 * @TITLE: CmkWidget
 * @SHORT_DESCRIPTION: The base Widget class of Cmk
 *
 * CmkWidget is the base class of Cmk widgets, but usually
 * a wrapper class, like #CmkGtkWidget, should be used to
 * easily embed a widget in a windowing system.
 *
 * CmkWidgets do not directly support having child widgets
 * (a widget implementation can create another widget
 * internally for rendering); child widgets should be
 * handled by the wrapper toolkit: Gtk, Clutter, Cocoa, etc.
 *
 * A minimal version of a GObject is created, to avoid
 * unnecessary glib dependency on all platforms. Widgets
 * receive user events through cmk_widget_event() and
 * respond with signals through cmk_widget_listen().
 * See any existing CmkWidget subclass for reference on
 * how to subclass CmkWidget; it's very similar to GObject.
 *
 * CmkWidgets can emit
 *   "invalidate": A redraw of the widget should be performed.
 *      signaldata is a (const cairo_region_t *) specifying
 *      the region invalidated, or NULL to request the entire
 *      area of the widget.
 *   "relayout": The widget's size request has changed.
 *      signaldata is NULL. 
 *   "event-mask": The widget's event mask changed.
 *      signaldata is NULL.
 */

#ifndef __CMK_WIDGET_H__
#define __CMK_WIDGET_H__

#include <stdbool.h>
#include <stdio.h>
#include <cairo/cairo.h>
#include "cmk-event.h"

#define cmk_return_if_fail(expr, ret) if(!(expr)) { printf("Warning: Assertion '%s' failed in function '%s'.\n", #expr, __FUNCTION__); return ret; }

typedef struct
{
	float x, y, width, height;
} CmkRect;

typedef struct
{
	/*< public >*/
	void *class;

	/*< private >*/
	void *priv;
} CmkWidget;

/**
 * CmkWidgetClass:
 * @destroy: Called on widget destruction. You should destroy any
 *           class-private data, and then chain up.
 * @draw: Draw handler. Chain up to this before doing your own
 *        drawing to get standard widget effects.
 * @event: User input event handler. Return TRUE if the widget
 *         captured and responded to the event, FALSE to propagate
 *         the event.
 * @event_mask: Return a mask of CmkEventTypes that the widget
 *         should be able to recieve. For optimization purposes.
 * @disable: Disabled event handler.
 * @get_preferred_width: Preferred width handler.
 * @get_preferred_height: Preferred height handler.
 * @get_draw_rect: Gets the area in which the widget will do
 *         drawing. This may be bigger or smaller than the widget's
 *         size. This is only checked on relayout.
 */
typedef struct
{
	/*< public >*/
	void (*destroy) (CmkWidget *self);
	void (*draw) (CmkWidget *self, cairo_t *cr);
	void (*disable) (CmkWidget *self, bool disabled);
	bool (*event) (CmkWidget *self, const CmkEvent *event);
	void (*get_preferred_width) (CmkWidget *self, float forHeight, float *min, float *nat);
	void (*get_preferred_height) (CmkWidget *self, float forWidth, float *min, float *nat);
	void (*get_draw_rect) (CmkWidget *self, CmkRect *rect);
} CmkWidgetClass;

extern CmkWidgetClass *cmk_widget_class;

typedef void (*CmkCallback) (CmkWidget *widget, const void *signaldata, void *userdata);
#define CMK_CALLBACK(f) ((CmkCallback) (f))


/**
 * cmk_widget_new:
 *
 * Creates a blank widget.
 */
CmkWidget * cmk_widget_new(void);

/**
 * cmk_widget_init:
 *
 * Initializes a CmkWidget. For use when subclassing.
 */
void cmk_widget_init(CmkWidget *widget);

/**
 * cmk_widget_ref:
 *
 * Increases ref count by 1.
 */
CmkWidget * cmk_widget_ref(CmkWidget *widget);

/**
 * cmk_widget_unref:
 *
 * Decreases ref count by 1. If the ref count hits 0 the
 * #CmkWidget is deallocated.
 */
void cmk_widget_unref(CmkWidget *widget);

/**
 * cmk_widget_set_wrapper:
 *
 * Sets the toolkit-specific wrapper object.
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
 * cmk_widget_listen:
 *
 * Listens to a signal from the widget. Returns a unique
 * id which can be passed to cmk_widget_unlisten().
 */
uint32_t cmk_widget_listen(CmkWidget *widget, const char *signalName, CmkCallback callback, void *userdata);

/**
 * cmk_widget_unlisten:
 *
 * Unlisten to signal from cmk_widget_listen().
 */
void cmk_widget_unlisten(CmkWidget *widget, uint32_t id);

/**
 * Unlisten to all signals that match userdata,
 * for cmk_widget_listen().
 */
void cmk_widget_unlisten_by_userdata(CmkWidget *widget, const void *userdata);

/**
 * cmk_widget_emit:
 *
 * For internal use by widgets: emit a signal to listeners
 * attached through cmk_widget_listen().
 */
void cmk_widget_emit(CmkWidget *widget, const char *signalName, const void *data);

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
 * The widget may draw outside its size allocation. See
 * cmk_widget_get_draw_rect() to get the size of the region
 * that the widget may draw to.
 *
 * No attempt at changing the clip region is made. If the clip
 * region is not equal to or greater than the draw rect, then
 * all of the widget may not be drawn.
 */
void cmk_widget_draw(CmkWidget *widget, cairo_t *cr);

/**
 * cmk_widget_event:
 *
 * Emits an event on this widget. Coordinates should be relative
 * to this widget's origin.
 */
bool cmk_widget_event(CmkWidget *widget, const CmkEvent *event);

/**
 * cmk_widget_set_event_mask:
 *
 * Sets a widget's event mask in the form of a bitfield of
 * #CmkEventType s. This is for use internally by widgets.
 */
void cmk_widget_set_event_mask(CmkWidget *widget, unsigned int mask);

/**
 * cmk_widget_get_event_mask.
 *
 * Gets a widget's event mask. cmk_widget_event() won't
 * pass any events not in this mask. See
 * cmk_widget_set_event_mask().
 */
unsigned int cmk_widget_get_event_mask(CmkWidget *widget);

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
 * cmk_widget_get_draw_rect:
 *
 * Gets the area in which the widget will draw. This may be bigger
 * or smaller than the widget's size. Specified in cairo units with
 * (0,0) being the top-left corner of the widget. This may change
 * on a size change (including relayout event).
 *
 * This should be called after the widget's size has been set with
 * cmk_widget_set_size().
 *
 * @rect: A place to store the rect.
 */
void cmk_widget_get_draw_rect(CmkWidget *widget, CmkRect *rect);

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
void cmk_widget_set_disabled(CmkWidget *widget, bool disabled);

/**
 * cmk_widget_get_disabled:
 */
bool cmk_widget_get_disabled(CmkWidget *widget);

#endif
