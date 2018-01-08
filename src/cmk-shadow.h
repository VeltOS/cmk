/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

/**
 * SECTION:cmk-shadow
 * @TITLE: CmkShadow
 *
 * CmkShadow is an object which can be used to draw shadows within
 * a #CmkWidget's draw method. It can draw shadows of any shape
 * with cmk_shadow_set_shape(), but it has a highly optimized method
 * for drawing shadows of rectangles with cmk_shadow_set_rectangle().
 *
 * CmkShadow supports animation with the cmk_shadow_set_percent()
 * method. This allows changing the percentage of blur on the shadow,
 * as if you had changed the radius to (original radius * percent).
 * Setting the percentage is faster as it does not require reallocation
 * of temporary drawing surfaces, however using a small percentage on
 * a very large radius could waste a MB or two of memory.
 *
 * The shadow is drawn such that the shape is centered at the
 * cairo current position. So if you set a rectangle with 10 radius
 * and draw it at a position of (200, 200), the blur will extend
 * back to (190, 190).
 */

#ifndef __CMK_SHADOW_H__
#define __CMK_SHADOW_H__

#include "cmk-widget.h"

#define CMK_TYPE_SHADOW cmk_shadow_get_type()
G_DECLARE_FINAL_TYPE(CmkShadow, cmk_shadow, CMK, SHADOW, GObject);

/**
 * cmk_shadow_new:
 *
 * Create a new CmkShadow with a given radius.
 */
CmkShadow * cmk_shadow_new(float radius);

/**
 * cmk_shadow_set_percent:
 *
 * Sets the real blur radius, as a percentage
 * of the base radius (range 0 to 1). This is
 * commonly used for animation of shadow radius.
 */
void cmk_shadow_set_percent(CmkShadow *shadow, float percent);

/**
 * cmk_shadow_get_radius:
 *
 * Gets value set with cmk_shadow_set_percent().
 */
float cmk_shadow_get_percent(CmkShadow *shadow);

/**
 * cmk_shadow_set_shape:
 * @path: (transfer full): The path representing the shape to shadow.
 * @inner: true to draw an inner shadow instead of a drop shadow.
 *
 * Uses the given Cairo path to create the shape of the
 * shadow. The #CmkShadow takes ownership of the path,
 * and it must be freeable by cairo_destroy_path() (thus
 * it should have been created through cairo_copy_path()
 * or cairo_copy_path_flat()).
 *
 * USING THIS IS VERY SLOW! If your shape is a rectangle,
 * use cmk_shadow_set_rectangle() instead.
 *
 * Every call to this method clears the cached shadow, even
 * if the given path has not changed. So call this as little
 * as possible.
 */
void cmk_shadow_set_shape(CmkShadow *shadow, cairo_path_t *path, bool inner);

/**
 * cmk_shadow_set_rectangle:
 * @width: width of rectangle to shadow.
 * @height: height of rectangle to shadow.
 * @inner: true to draw an inner shadow instead of a drop shadow.
 *
 * Implements a highly optimized version of
 * cmk_shadow_set_shape() for rectangles.
 */
void cmk_shadow_set_rectangle(CmkShadow *self, float width, float height, bool inner);

/**
 * cmk_shadow_draw:
 *
 * Draws the shadow using the given Cairo context.
 *
 * This should be performed before drawing the actual object
 * with the shadow, so that the shadow appears below the object.
 */
void cmk_shadow_draw(CmkShadow *shadow, cairo_t *cr);

#endif
