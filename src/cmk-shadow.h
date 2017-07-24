/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

/**
 * SECTION:cmk-shadow-effect
 * @TITLE: CmkShadowEffect
 * @SHORT_DESCRIPTION: ClutterEffect to render shadows on actors.
 *
 * TODO: Better documentation plz
 */

#ifndef __CMK_SHADOW_H__
#define __CMK_SHADOW_H__

#include <clutter/clutter.h>
#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_SHADOW_EFFECT cmk_shadow_effect_get_type()
G_DECLARE_FINAL_TYPE(CmkShadowEffect, cmk_shadow_effect, CMK, SHADOW_EFFECT, ClutterEffect);

/**
 * cmk_shadow_effect_new:
 *
 * Creates a new shadow effect with the given maximum size
 * as given in cmk_shadow_effect_set()). By default, the
 * shadow is an outer shadow with 0% blur.
 *
 * You must call cmk_shadow_effect_set() or cmk_shadow_effect_set_inset()
 * to specify how the shadow will look.
 *
 * Shadows are colored with the "shadow" named color if attached
 * to a CmkWidget.
 */
CmkShadowEffect * cmk_shadow_effect_new(float size);

/**
 * cmk_shadow_effect_new_drop_shadow:
 *
 * Similar to cmk_shadow_effect_new(), but calls
 * cmk_shadow_effect_set() for you.
 */
ClutterEffect * cmk_shadow_effect_new_drop_shadow(float size, float x, float y, float radius, float spread);

/**
 * cmk_shadow_effect_set_size:
 *
 * Sets the maximum radius of the shadow blur. If the actor is a
 * CmkWidget, @r is in dps. Otherwise, it is in Clutter units.
 *
 * The purpose of this function is for optimization; if CmkShadowEffect
 * knows the maximum shadow size, it has to do fewer texture allocations
 * when the shadow is animated. cmk_shadow_effect_set() and
 * cmk_shadow_effect_set_inset() use a certain percentage of this radius
 * value.
 */
void cmk_shadow_effect_set_size(CmkShadowEffect *effect, float size);

/**
 * cmk_shadow_effect_set:
 * @x: x offset of shadow (can be negative)
 * @y: y offset of shadow (can be negative)
 * @radius: Blur radius, as a percentage of maximum size (see
 *          cmk_shadow_effect_set_size())
 * @spread: Spread radius
 *
 * Sets the shadow properties, which work the same as the
 * box-shadow property in CSS. Values except @radius are in dps if
 * the actor is a CmkWidget, otherwise Clutter units.
 *
 * Mutually exclusive to cmk_shadow_effect_set_inset().
 */
void cmk_shadow_effect_set(CmkShadowEffect *effect, float x, float y, float radius, float spread);

/**
 * cmk_shadow_effect_animate_radius:
 *
 * Smoothly transitions the radius value of an outer shadow to a new value.
 * Only for shadows set with cmk_shadow_effect_set().
 */
void cmk_shadow_effect_animate_radius(CmkShadowEffect *self, float radius);

/**
 * cmk_shadow_effect_set_inset:
 *
 * Draws an inset shadow, with the left, right, top, and bottom
 * edges having @l, @r, @t, and @b percent maximum shadow size,
 * respectively. See cmk_shadow_effect_set_size().
 *
 * Mutually exclusive to cmk_shadow_effect_set().
 */
void cmk_shadow_effect_set_inset(CmkShadowEffect *effect, float l, float r, float t, float b);

/**
 * cmk_shadow_effect_inset_animate_edges:
 *
 * Smoothly transitions the inner edge values of an inset shadow.
 */
void cmk_shadow_effect_inset_animate_edges(CmkShadowEffect *self, float l, float r, float t, float b);

#endif
