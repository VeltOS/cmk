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
 */

#ifndef __CMK_WIDGET_H__
#define __CMK_WIDGET_H__

#include <stdbool.h>
#include <cairo/cairo.h>
#include <pango/pango.h>
#include <glib-object.h>
#include "cmk-event.h"
#include "cmk-palette.h"

G_BEGIN_DECLS

typedef struct
{
	float x, y, width, height;
} CmkRect;

#define CMK_TYPE_WIDGET cmk_widget_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkWidget, cmk_widget, CMK, WIDGET, GInitiallyUnowned);
typedef struct _CmkWidgetClass CmkWidgetClass;

/**
 * CmkWidgetClass:
 * @draw: Draw handler. Chain up to this before doing your own
 *        drawing to get standard widget effects.
 * @event: User input event handler. Return TRUE if the widget
 *         captured and responded to the event, FALSE to propagate
 *         the event.
 * @disable: Disabled event handler.
 * @get_preferred_width: Preferred width handler.
 * @get_preferred_height: Preferred height handler.
 * @get_draw_rect: Gets the area in which the widget will do
 *         drawing. This may be bigger or smaller than the widget's
 *         size. This is only checked on relayout.
 */
struct _CmkWidgetClass
{
	/*< private >*/
	GObjectClass parentClass;

	/*< public >*/
	void (*draw) (CmkWidget *self, cairo_t *cr);
	void (*disable) (CmkWidget *self, bool disabled);
	bool (*event) (CmkWidget *self, const CmkEvent *event);
	void (*get_preferred_width) (CmkWidget *self, float forHeight, float *min, float *nat);
	void (*get_preferred_height) (CmkWidget *self, float forWidth, float *min, float *nat);
	void (*get_draw_rect) (CmkWidget *self, CmkRect *rect);
};


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
 * cmk_widget_focus:
 *
 * Request keyboard focus of the widget.
 */
void cmk_widget_focus(CmkWidget *widget);

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
 *
 * This is automatically called by the #CmkWidget wrappers which
 * have a disabled property, including #CmkGtkWidget and
 * #CmkClutterWidget.
 */
void cmk_widget_set_disabled(CmkWidget *widget, bool disabled);

/**
 * cmk_widget_get_disabled:
 */
bool cmk_widget_get_disabled(CmkWidget *widget);

/**
 * cmk_widget_set_pango_context:
 *
 * Sets the Pango context, to be used by widgets
 * rendering text. This should only be used by
 * the widget's wrapper. If no context is set,
 * text rendering widgets will generate their own
 * with default values.
 */
void cmk_widget_set_pango_context(CmkWidget *self, PangoContext *context);

/**
 * cmk_widget_get_pango_context:
 *
 * Gets this widget's Pango context set thorugh
 * cmk_widget_set_pango_context().
 */
PangoContext * cmk_widget_get_pango_context(CmkWidget *self);

/**
 * cmk_widget_set_palette:
 *
 * Sets the color palette to use for this widget. A widget
 * implementation may call this during its initialization
 * to set its default color palette.
 *
 * Either using a custom palette, or something like
 * cmk_widget_set_palette(widget, cmk_palette_get_primary(0));
 * is appropriate.
 */
void cmk_widget_set_palette(CmkWidget *widget, CmkPalette *palette);

/**
 * cmk_widget_get_palette:
 *
 * Gets the palette set through cmk_widget_set_palette(),
 * or if not set, uses cmk_palette_get_base() with the type
 * of the widget.
 */
CmkPalette * cmk_widget_get_palette(CmkWidget *widget);

/**
 * cmk_widget_get_color:
 *
 * Equivelent to cmk_palette_get_color() with cmk_widget_get_palette()
 * as the palette.
 */
const CmkColor * cmk_widget_get_color(CmkWidget *widget, const char *name);

/**
 * cmk_cairo_set_source_color:
 *
 * Calls cairo_set_source_rgba() using the values in CmkColor.
 */
void cmk_cairo_set_source_color(cairo_t *cr, const CmkColor *color);

G_END_DECLS

#endif
