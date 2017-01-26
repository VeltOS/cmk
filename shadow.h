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

G_END_DECLS

#endif

