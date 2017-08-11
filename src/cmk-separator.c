/*
 * This file is part of cmk-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */
 
#include "cmk-separator.h"

struct _CmkSeparator
{
	CmkWidget parent;
	ClutterOrientation orientation;
};

static void get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);

G_DEFINE_TYPE(CmkSeparator, cmk_separator, CMK_TYPE_WIDGET);


CmkWidget * cmk_separator_new_h(void)
{
	CmkSeparator *s = CMK_SEPARATOR(g_object_new(CMK_TYPE_SEPARATOR, NULL));
	s->orientation = CLUTTER_ORIENTATION_HORIZONTAL;
	clutter_actor_set_x_expand(CLUTTER_ACTOR(s), TRUE);
	return (CmkWidget *)s;
}

CmkWidget * cmk_separator_new_v(void)
{
	CmkSeparator *s = CMK_SEPARATOR(g_object_new(CMK_TYPE_SEPARATOR, NULL));
	s->orientation = CLUTTER_ORIENTATION_VERTICAL;
	clutter_actor_set_y_expand(CLUTTER_ACTOR(s), TRUE);
	return (CmkWidget *)s;
}

static void cmk_separator_class_init(CmkSeparatorClass *class)
{
	CLUTTER_ACTOR_CLASS(class)->get_preferred_width = get_preferred_width;
	CLUTTER_ACTOR_CLASS(class)->get_preferred_height = get_preferred_height;
}

static void cmk_separator_init(CmkSeparator *self)
{
	ClutterColor c = {0,0,0,25};
	clutter_actor_set_background_color(CLUTTER_ACTOR(self), &c);
}

static void get_preferred_width(ClutterActor *self_, UNUSED gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	if(CMK_SEPARATOR(self_)->orientation == CLUTTER_ORIENTATION_HORIZONTAL)
		*minWidth = *natWidth = 0;
	else
		*minWidth = *natWidth = CMK_DP(self_, 1);
}

static void get_preferred_height(ClutterActor *self_, UNUSED gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	if(CMK_SEPARATOR(self_)->orientation == CLUTTER_ORIENTATION_VERTICAL)
		*minHeight = *natHeight = 0;
	else
		*minHeight = *natHeight = CMK_DP(self_, 1);
}
