/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "shadow.h"
#include <math.h>

struct _CmkShadow
{
	ClutterActor parent;
	ClutterCanvas *canvas;
	gfloat vRadius, hRadius;
};

static void cmk_shadow_dispose(GObject *self_);
static void cmk_shadow_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void cmk_shadow_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);
static void cmk_shadow_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
static void on_size_changed(CmkShadow *self, GParamSpec *spec, gpointer userdata);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkShadow *self);

G_DEFINE_TYPE(CmkShadow, cmk_shadow, CMK_TYPE_WIDGET);



CmkShadow * cmk_shadow_new()
{
	return CMK_SHADOW(g_object_new(CMK_TYPE_SHADOW, NULL));
}

static void cmk_shadow_class_init(CmkShadowClass *class)
{
	G_OBJECT_CLASS(class)->dispose = cmk_shadow_dispose;
	
	CLUTTER_ACTOR_CLASS(class)->get_preferred_width = cmk_shadow_get_preferred_width;
	CLUTTER_ACTOR_CLASS(class)->get_preferred_height = cmk_shadow_get_preferred_height;
	CLUTTER_ACTOR_CLASS(class)->allocate = cmk_shadow_allocate;
}

static void cmk_shadow_init(CmkShadow *self)
{
	self->canvas = CLUTTER_CANVAS(clutter_canvas_new());
	g_signal_connect(self->canvas, "draw", G_CALLBACK(on_draw_canvas), self);
	clutter_actor_set_content_gravity(CLUTTER_ACTOR(self), CLUTTER_CONTENT_GRAVITY_CENTER);
	clutter_actor_set_content(CLUTTER_ACTOR(self), CLUTTER_CONTENT(self->canvas));

	//clutter_actor_set_size(self, 0, 0); // Stops infinite loop of notify::size events
	g_signal_connect(self, "notify::size", G_CALLBACK(on_size_changed), NULL);
	//clutter_actor_add_child(CLUTTER_ACTOR(self), self->shadow);
}

static void cmk_shadow_dispose(GObject *self_)
{
	CmkShadow *self = CMK_SHADOW(self_);
	G_OBJECT_CLASS(cmk_shadow_parent_class)->dispose(self_);
}

static void cmk_shadow_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	ClutterActor *child = clutter_actor_get_first_child(self_);
	*minWidth = 0;
	*natWidth = 0;
	if(child)
		clutter_actor_get_preferred_width(child, forHeight, minWidth, natWidth);
	*minWidth += CMK_SHADOW(self_)->hRadius*2;
	*natWidth += CMK_SHADOW(self_)->hRadius*2;
}

static void cmk_shadow_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	ClutterActor *child = clutter_actor_get_first_child(self_);
	*minHeight = 0;
	*natHeight = 0;
	if(child)
		clutter_actor_get_preferred_height(child, forWidth, minHeight, natHeight);
	*minHeight += CMK_SHADOW(self_)->vRadius*2;
	*natHeight += CMK_SHADOW(self_)->vRadius*2;
}

static void cmk_shadow_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	CmkShadow *self = CMK_SHADOW(self_);
	ClutterActor *child = clutter_actor_get_first_child(self_);

	if(child)
	{
		gfloat width = box->x2 - box->x1;
		gfloat height = box->y2 - box->y1;
		ClutterActorBox childBox = {self->hRadius, self->vRadius, width - self->hRadius, height - self->vRadius};
		clutter_actor_allocate(child, &childBox, flags);
	}

	CLUTTER_ACTOR_CLASS(cmk_shadow_parent_class)->allocate(self_, box, flags);
}

static void on_size_changed(CmkShadow *self, GParamSpec *spec, gpointer userdata)
{
	gfloat width, height;
	clutter_actor_get_size(CLUTTER_ACTOR(self), &width, &height);
	clutter_canvas_set_size(self->canvas, width, height);
}

//void boxesForGauss(float sigma, float *sizes, guint n)
//{
//	float wIdeal = sqrt((12*sigma*sigma/n)+1);  // Ideal averaging filter width 
//	guint wl = floor(wIdeal);
//	if(wl%2==0)
//		wl--;
//	guint wu = wl+2;
//
//	float mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
//	float m = round(mIdeal);
//
//	for(guint i=0;i<n;++i)
//		sizes[i] = i<m?wl:wu;
//}

// http://blog.ivank.net/fastest-gaussian-blur.html
void boxBlurH_4(guchar *scl, guchar *tcl, guint w, guint h, guint r)
{
    float iarr = 1.0 / (r+r+1.0);
    for(guint i=0; i<h; i++) {
        guint ti = i*w, li = ti, ri = ti+r;
        guint fv = scl[ti], lv = scl[ti+w-1], val = (r+1)*fv;
        for(guint j=0; j<r; j++) val += scl[ti+j];
        for(guint j=0  ; j<=r ; j++) { val += scl[ri++] - fv       ;   tcl[ti++] = round(val*iarr); }
        for(guint j=r+1; j<w-r; j++) { val += scl[ri++] - scl[li++];   tcl[ti++] = round(val*iarr); }
        for(guint j=w-r; j<w  ; j++) { val += lv        - scl[li++];   tcl[ti++] = round(val*iarr); }
    }
}

void boxBlurT_4(guchar *scl, guchar *tcl, guint w, guint h, guint r)
{
    float iarr = 1.0 / (r+r+1.0);
    for(guint i=0; i<w; i++) {
        guint ti = i, li = ti, ri = ti+r*w;
        guint fv = scl[ti], lv = scl[ti+w*(h-1)], val = (r+1)*fv;
        for(guint j=0; j<r; j++) val += scl[ti+j*w];
        for(guint j=0  ; j<=r ; j++) { val += scl[ri] - fv     ;  tcl[ti] = round(val*iarr);  ri+=w; ti+=w; }
        for(guint j=r+1; j<h-r; j++) { val += scl[ri] - scl[li];  tcl[ti] = round(val*iarr);  li+=w; ri+=w; ti+=w; }
        for(guint j=h-r; j<h  ; j++) { val += lv - scl[li]; tcl[ti] = round(val*iarr); li+=w; ti+=w; }
    }
}

//static unsigned long getms()
//{
//	struct timeval tv;
//	gettimeofday(&tv, NULL);
//	return 1000000 * tv.tv_sec + tv.tv_usec;
//}

static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, gint width, gint height, CmkShadow *self)
{
	//unsigned long start = getms();
	cairo_surface_t *surface = cairo_get_target(cr);

	if(cairo_image_surface_get_format(surface) != CAIRO_FORMAT_ARGB32)
	{
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);
		cairo_restore(cr);
		return TRUE;
	}

	guchar *buffer = cairo_image_surface_get_data(surface);

	guint length = width*height;
	guchar *source = g_new0(guchar, length*2);
	guchar *dest = source + length;

	// Create a black rectangle that matches the area of the parent actor
	for(guint x=self->hRadius;x<width-self->hRadius;++x)
		for(guint y=self->vRadius;y<height-self->vRadius;++y)
			source[y*width+x] = 255;

	// Do a gaussian blur on the rectangle
	const guint passes = 2;
	for(guint i=0;i<passes;++i)
	{
		guchar *sc = i%2 ? dest:source;
		guchar *dt = i%2 ? source:dest;
		for(guint j=0; j<length; j++)
			dt[j] = sc[j];
		boxBlurH_4(dt, sc, width, height, self->hRadius/2);
		boxBlurT_4(sc, dt, width, height, self->vRadius/2);
	}
	
	// Copy result to cairo
	for(guint i=0;i<length;i++)
	{
		buffer[i*4+0]=0;
		buffer[i*4+1]=0;
		buffer[i*4+2]=0;
		buffer[i*4+3]=source[i];
	}

	g_free(source);
	//unsigned long end = getms();
	//unsigned long delta = end - start;
	//double deltams = delta / 1000.0;
	//printf("shadowtime: %f\n", deltams);
	return TRUE;
}

void cmk_shadow_set_blur(CmkShadow *self, gfloat radius)
{
	g_return_if_fail(CMK_IS_SHADOW(self));
	self->hRadius = radius;
	self->vRadius = radius;
	clutter_content_invalidate(CLUTTER_CONTENT(self->canvas));
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}


void cmk_shadow_set_vblur(CmkShadow *self, gfloat radius)
{
	g_return_if_fail(CMK_IS_SHADOW(self));
	self->vRadius = radius;
	clutter_content_invalidate(CLUTTER_CONTENT(self->canvas));
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

void cmk_shadow_set_hblur(CmkShadow *self, gfloat radius)
{
	g_return_if_fail(CMK_IS_SHADOW(self));
	self->hRadius = radius;
	clutter_content_invalidate(CLUTTER_CONTENT(self->canvas));
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

gfloat cmk_shadow_get_vblur(CmkShadow *self)
{
	g_return_val_if_fail(CMK_IS_SHADOW(self), 0);
	return self->vRadius;
}

gfloat cmk_shadow_get_hblur(CmkShadow *self)
{
	g_return_val_if_fail(CMK_IS_SHADOW(self), 0);
	return self->hRadius;
}
