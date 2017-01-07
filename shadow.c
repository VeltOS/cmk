/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "shadow.h"
#include <math.h>

struct _CmkShadowContainer
{
	ClutterActor parent;
	ClutterCanvas *canvas;
	gfloat vRadius, hRadius;
};

static void cmk_shadow_container_iface_init(ClutterContainerIface *iface);
static void cmk_shadow_container_dispose(GObject *self_);
static void cmk_shadow_container_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void cmk_shadow_container_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);
static void on_child_added(ClutterContainer *self_, ClutterActor *actor);
static void on_child_removed(ClutterContainer *self_, ClutterActor *actor);
static void on_size_changed(CmkShadowContainer *self, GParamSpec *spec, gpointer userdata);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkShadowContainer *self);

G_DEFINE_TYPE_WITH_CODE(CmkShadowContainer, cmk_shadow_container, CMK_TYPE_WIDGET,
	G_IMPLEMENT_INTERFACE(CLUTTER_TYPE_CONTAINER, cmk_shadow_container_iface_init));



CmkShadowContainer * cmk_shadow_container_new()
{
	return CMK_SHADOW_CONTAINER(g_object_new(CMK_TYPE_SHADOW_CONTAINER, NULL));
}

static void cmk_shadow_container_class_init(CmkShadowContainerClass *class)
{
	G_OBJECT_CLASS(class)->dispose = cmk_shadow_container_dispose;
	
	CLUTTER_ACTOR_CLASS(class)->get_preferred_width = cmk_shadow_container_get_preferred_width;
	CLUTTER_ACTOR_CLASS(class)->get_preferred_height = cmk_shadow_container_get_preferred_height;
}

static void cmk_shadow_container_iface_init(ClutterContainerIface *iface)
{
	iface->actor_added = on_child_added;
	iface->actor_removed = on_child_removed;
}

static void cmk_shadow_container_init(CmkShadowContainer *self)
{
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), clutter_bin_layout_new(CLUTTER_BIN_ALIGNMENT_START, CLUTTER_BIN_ALIGNMENT_START));

	self->canvas = CLUTTER_CANVAS(clutter_canvas_new());
	g_signal_connect(self->canvas, "draw", G_CALLBACK(on_draw_canvas), self);
	clutter_actor_set_content_gravity(CLUTTER_ACTOR(self), CLUTTER_CONTENT_GRAVITY_CENTER);
	clutter_actor_set_content(CLUTTER_ACTOR(self), CLUTTER_CONTENT(self->canvas));

	//clutter_actor_set_size(self, 0, 0); // Stops infinite loop of notify::size events
	g_signal_connect(self, "notify::size", G_CALLBACK(on_size_changed), NULL);
	//clutter_actor_add_child(CLUTTER_ACTOR(self), self->shadow_container);
}

static void cmk_shadow_container_dispose(GObject *self_)
{
	CmkShadowContainer *self = CMK_SHADOW_CONTAINER(self_);
	G_OBJECT_CLASS(cmk_shadow_container_parent_class)->dispose(self_);
}

static void cmk_shadow_container_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	// TOOD: Care about more than one child?
	ClutterActor *child = clutter_actor_get_first_child(self_);
	*minWidth = 0;
	*natWidth = 0;
	if(child)
		clutter_actor_get_preferred_width(child, forHeight, minWidth, natWidth);
}

static void cmk_shadow_container_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	ClutterActor *child = clutter_actor_get_first_child(self_);
	*minHeight = 0;
	*natHeight = 0;
	if(child)
		clutter_actor_get_preferred_height(child, forWidth, minHeight, natHeight);
}

static void on_child_added(ClutterContainer *self_, ClutterActor *actor)
{
	gfloat hRadius = CMK_SHADOW_CONTAINER(self_)->hRadius;
	gfloat vRadius = CMK_SHADOW_CONTAINER(self_)->vRadius;
	ClutterMargin margin = {hRadius, hRadius, vRadius, vRadius};
	clutter_actor_set_margin(actor, &margin);
}

static void on_child_removed(ClutterContainer *self_, ClutterActor *actor)
{
}

static void on_size_changed(CmkShadowContainer *self, GParamSpec *spec, gpointer userdata)
{
	gfloat width, height;
	clutter_actor_get_size(CLUTTER_ACTOR(self), &width, &height);
	clutter_canvas_set_size(self->canvas, width, height);
	
	//gint canvasWidth = width + self->hRadius*2;
	//gint canvasHeight = height + self->vRadius*2;

	//clutter_actor_set_position(self->shadow_container, -self->hRadius, -self->vRadius);
	//clutter_actor_set_size(self->shadow_container, canvasWidth, canvasHeight);
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

static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, gint width, gint height, CmkShadowContainer *self)
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
	//printf("shadow_containertime: %f\n", deltams);
	return TRUE;
}

void cmk_shadow_container_set_blur(CmkShadowContainer *self, gfloat radius)
{
	g_return_if_fail(CMK_IS_SHADOW_CONTAINER(self));
	self->hRadius = radius;
	self->vRadius = radius;
	clutter_content_invalidate(CLUTTER_CONTENT(self->canvas));
	ClutterActor *firstChild = clutter_actor_get_first_child(CLUTTER_ACTOR(self));
	if(firstChild)
		on_child_added(CLUTTER_CONTAINER(self), firstChild);
}


void cmk_shadow_container_set_vblur(CmkShadowContainer *self, gfloat radius)
{
	g_return_if_fail(CMK_IS_SHADOW_CONTAINER(self));
	self->vRadius = radius;
	clutter_content_invalidate(CLUTTER_CONTENT(self->canvas));
	ClutterActor *firstChild = clutter_actor_get_first_child(CLUTTER_ACTOR(self));
	if(firstChild)
		on_child_added(CLUTTER_CONTAINER(self), firstChild);
}

void cmk_shadow_container_set_hblur(CmkShadowContainer *self, gfloat radius)
{
	g_return_if_fail(CMK_IS_SHADOW_CONTAINER(self));
	self->hRadius = radius;
	clutter_content_invalidate(CLUTTER_CONTENT(self->canvas));
	ClutterActor *firstChild = clutter_actor_get_first_child(CLUTTER_ACTOR(self));
	if(firstChild)
		on_child_added(CLUTTER_CONTAINER(self), firstChild);
}

gfloat cmk_shadow_container_get_vblur(CmkShadowContainer *self)
{
	g_return_val_if_fail(CMK_IS_SHADOW_CONTAINER(self), 0);
	return self->vRadius;
}

gfloat cmk_shadow_container_get_hblur(CmkShadowContainer *self)
{
	g_return_val_if_fail(CMK_IS_SHADOW_CONTAINER(self), 0);
	return self->hRadius;
}
