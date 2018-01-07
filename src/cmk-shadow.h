/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

/**
 * SECTION:cmk-shadow
 * @TITLE: CmkShadow
 *
 * Drawing shadows.
 */

#ifndef __CMK_SHADOW_H__
#define __CMK_SHADOW_H__

#include "cmk-widget.h"

#define CMK_TYPE_SHADOW cmk_shadow_get_type()
G_DECLARE_FINAL_TYPE(CmkShadow, cmk_shadow, CMK, SHADOW, GObject);

/**
 * cmk_shadow_new:
 *
 * Create a new CmkShadow.
 */
CmkShadow * cmk_shadow_new(float maxRadius, bool inner);

/**
 * cmk_shadow_set_radius:
 *
 * Sets the shadow's blur radius as a percentage of the
 * maximum radius (range 0 to 1).
 */
void cmk_shadow_set_radius(CmkShadow *shadow, float radius);

/**
 * cmk_shadow_get_radius:
 *
 * Gets the shadow's blue radius as a percentage of the
 * maximum radius.
 */
float cmk_shadow_get_radius(CmkShadow *shadow);

/**
 * cmk_shadow_set_shape:
 *
 * Uses the current path of the cairo context to describe
 * the shape of object needing a shadow. Alternatively,
 * NULL can be passed to clear the shadow's path.
 *
 * If there is already a path set on this CmkShadow, it
 * will not be changed unless overwrite is true. This can
 * be used to clear the path when the shape changes (set
 * cr to NULL) and then next time the shape is drawn,
 * pass the cairo context with overwrite false to only
 * set the shape on the first draw. If the shape is
 * continuously changing, set overwrite to true.
 *
 * The size request of the shadow will be the extents
 * of the path. The draw rect will be the extents plus
 * the maximum radius of the shadow on each side.
 *
 * Set rectangle to true if the path is rectangular shape.
 * This allows faster drawing for large rectangular shadows.
 */
void cmk_shadow_set_shape(CmkShadow *shadow, cairo_path_t *path);

void cmk_shadow_set_rectangle(CmkShadow *self, float width, float height);

void cmk_shadow_draw(CmkShadow *shadow, cairo_t *cr);

#endif
