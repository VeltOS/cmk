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

#define CMK_TYPE_SHADOW_CONTAINER cmk_shadow_container_get_type()
G_DECLARE_FINAL_TYPE(CMKShadowContainer, cmk_shadow_container, CMK, SHADOW_CONTAINER, CMKWidget);

CMKShadowContainer * cmk_shadow_container_new();

void cmk_shadow_container_set_blur(CMKShadowContainer *shadow, gfloat radius);
void cmk_shadow_container_set_vblur(CMKShadowContainer *shadow, gfloat radius);
void cmk_shadow_container_set_hblur(CMKShadowContainer *shadow, gfloat radius);
gfloat cmk_shadow_container_get_vblur(CMKShadowContainer *shadow);
gfloat cmk_shadow_container_get_hblur(CMKShadowContainer *shadow);

G_END_DECLS

#endif

