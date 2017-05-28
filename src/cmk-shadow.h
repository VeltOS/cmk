/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_SHADOW_H__
#define __CMK_SHADOW_H__

#include <clutter/clutter.h>
#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_SHADOUTIL cmk_shadoutil_get_type()
G_DECLARE_FINAL_TYPE(CmkShadoutil, cmk_shadoutil, CMK, SHADOUTIL, GObject);

typedef enum
{
	CMK_SHADOW_MODE_OUTER,
	// The default outer mode will not blur a corner unless
	// the shadow extends on both edges of that corner. 
	// ALL_CORNERS mode will blur all corners anyway, even if
	// only one edge has a shadow. This creates a more realistic
	// "corner" feel, but might be bad for a shadow on a
	// surface on the edge of the screen.
	CMK_SHADOW_MODE_OUTER_ALL_CORNERS,
	CMK_SHADOW_MODE_INNER,
} CmkShadowMode;

/*
 * Use one of these per actor that needs a shadow.
 */
CmkShadoutil * cmk_shadoutil_new(void);

/*
 * Set shadow mode
 */
void cmk_shadoutil_set_mode(CmkShadoutil *shadoutil, CmkShadowMode mode);

/*
 * Get shadow mode
 */
CmkShadowMode cmk_shadoutil_get_mode(CmkShadoutil *shadoutil);

/*
 * Sets the maximum shadow size. See cmk_shadoutil_set_edgues.
 */
void cmk_shadoutil_set_size(CmkShadoutil *shadoutil, guint size);

/*
 * Gets the shadow size.
 */
guint cmk_shadoutil_get_size(CmkShadoutil *shadoutil);

/*
 * Sets the amount of shadow on each edge, as a percentage [0,1] of
 * the maximum. This can be used for shadow animations.
 */
void cmk_shadoutil_set_edges(CmkShadoutil *shadoutil, gfloat l, gfloat r, gfloat t, gfloat b);

/*
 * Get shadow edge percentages.
 */
void cmk_shadoutil_get_edges(CmkShadoutil *shadoutil, gfloat *l, gfloat *r, gfloat *t, gfloat *b);

/*
 * The set actor will be automatically queued for a repaint whenever this
 * shadoutil is invalidated.
 */
void cmk_shadoutil_set_actor(CmkShadoutil *shadoutil, ClutterActor *actor);

/*
 * Call this in Clutter's 'paint' actor class handler.
 * Call before chaining to parent handler to paint a drop shadow,
 * and after to paint an inner shadow.
 */
void cmk_shadoutil_paint(CmkShadoutil *shadoutil, ClutterActorBox *box);


#define CMK_TYPE_SHADOW cmk_shadow_get_type()
G_DECLARE_FINAL_TYPE(CmkShadow, cmk_shadow, CMK, SHADOW, CmkWidget);

enum
{
	CMK_SHADOW_MASK_LEFT = 1,
	CMK_SHADOW_MASK_RIGHT = 2,
	CMK_SHADOW_MASK_TOP = 4,
	CMK_SHADOW_MASK_BOTTOM = 8,
	CMK_SHADOW_MASK_ALL = CMK_SHADOW_MASK_LEFT | CMK_SHADOW_MASK_RIGHT | CMK_SHADOW_MASK_TOP | CMK_SHADOW_MASK_BOTTOM
};

CmkShadow * cmk_shadow_new();
CmkShadow * cmk_shadow_new_full(guint shadowMask, gfloat radius);

void cmk_shadow_set_mask(CmkShadow *shadow, guint shadowMask);
void cmk_shadow_set_radius(CmkShadow *shadow, guint radius);

ClutterActor * cmk_shadow_get_first_child(CmkShadow *shadow);

G_END_DECLS

#endif

