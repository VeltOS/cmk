/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

// Removes implicit declaration warnings
#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API

#include "cmk-shadow.h"
#include <cogl/cogl.h>
#include <math.h>
#include <string.h>

struct _CmkShadowEffect
{
	ClutterEffect parent;
	gboolean gettingPaintVolume;
	
	gboolean inset;
	float dps;
	float size;
	float l, r, t, b; // If inset
	float tL, tR, tT, tB; // For animation, if inset
	float x, y, radius, tRadius, spread; // If not inset
	
	CoglContext *ctx;
	CoglPipeline *pipe;
	gboolean npot;
	
	CoglTexture *tex;
	guchar *texData;
	guchar *texTmp;
	guint texW, texH;
	gboolean invalidated;
	
	CoglPrimitive *prim;
	ClutterTimeline *anim;
};

static void cmk_shadow_effect_dispose(GObject *self_);
static void on_paint(ClutterEffect *self_, ClutterEffectPaintFlags flags);
static gboolean get_paint_volume(ClutterEffect *self_, ClutterPaintVolume *volume);

G_DEFINE_TYPE(CmkShadowEffect, cmk_shadow_effect, CLUTTER_TYPE_EFFECT);



CmkShadowEffect * cmk_shadow_effect_new(float size)
{
	CmkShadowEffect *e = CMK_SHADOW_EFFECT(g_object_new(CMK_TYPE_SHADOW_EFFECT, NULL));
	cmk_shadow_effect_set_size(e, size);
	return e;
}

ClutterEffect * cmk_shadow_effect_new_drop_shadow(float size, float x, float y, float radius, float spread)
{
	CmkShadowEffect *e = CMK_SHADOW_EFFECT(g_object_new(CMK_TYPE_SHADOW_EFFECT, NULL));
	cmk_shadow_effect_set_size(e, size);
	cmk_shadow_effect_set(e, x, y, radius, spread);
	return CLUTTER_EFFECT(e);
}

static void cmk_shadow_effect_class_init(CmkShadowEffectClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_shadow_effect_dispose;

	CLUTTER_EFFECT_CLASS(class)->paint = on_paint;
	CLUTTER_EFFECT_CLASS(class)->get_paint_volume = get_paint_volume;
}

static void cmk_shadow_effect_init(CmkShadowEffect *self)
{
	self->ctx = clutter_backend_get_cogl_context(clutter_get_default_backend());
	self->pipe = cogl_pipeline_new(self->ctx);
	self->npot = cogl_has_feature(self->ctx, COGL_FEATURE_ID_TEXTURE_NPOT_BASIC);
}

static void cmk_shadow_effect_dispose(GObject *self_)
{
	CmkShadowEffect *self = CMK_SHADOW_EFFECT(self_);
	g_clear_pointer(&self->tex, cogl_object_unref);
	g_clear_pointer(&self->prim, cogl_object_unref);
	g_clear_pointer(&self->pipe, cogl_object_unref);
	g_clear_pointer(&self->texData, g_free);
	g_clear_pointer(&self->texTmp, g_free);
	g_clear_object(&self->anim);
	G_OBJECT_CLASS(cmk_shadow_effect_parent_class)->dispose(self_);
}

static void set_invalidated(CmkShadowEffect *self)
{
	self->invalidated = TRUE;
	clutter_effect_queue_repaint(CLUTTER_EFFECT(self));
}

// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static inline guint32 next_pot(guint32 v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

/*
 * These blur functions are modified versions of the algorithms
 * descripted in the "Fastest Gaussion Blur" article:
 * http://blog.ivank.net/fastest-gaussian-blur.html
 * The difference is that they can blur a specific region within the
 * buffer, which allows for edge-only blurring.
 */

/*
 * x + r <= ~stride/2 (?)
 * y + h <= buffer height
 * w <= stride
 * 0 < r
 */
static void boxBlurH(guchar *src, guchar *dst, guint stride, guint x, guint y, guint w, guint h, guint r)
{
	float iarr = 1.0 / (r+r+1.0);
	for(guint i=y; i<y+h; i++)
	{
		guint ti = i*stride+x, li = ti, ri = ti+r;
		guint fv = src[ti], lv = src[ti+w-1], val = (r+1)*fv;
		for(guint j=0; j<r; j++)
			val += src[ti+j];
		for(guint j=0; j<=r; j++) {
			val += src[ri++] - fv;
			dst[ti++] = round(val*iarr);
		}
		for(guint j=r+1; j<w-r; j++) {
			val += src[ri++] - src[li++];
			dst[ti++] = round(val*iarr);
		}
		for(guint j=w-r; j<w; j++) {
			val += lv - src[li++];
			dst[ti++] = round(val*iarr);
		}
	}
}

static void boxBlurV(guchar *src, guchar *dst, guint stride, guint x, guint y, guint w, guint h, guint r)
{
	float iarr = 1.0 / (r+r+1.0);
	for(guint i=x; i<x+w; i++)
	{
		guint ti = i+y*stride, li = ti, ri = ti+r*stride;
		guint fv = src[ti], lv = src[ti+stride*(h-1)], val = (r+1)*fv;
		for(guint j=0; j<r; j++)
			val += src[ti+j*stride];
		for(guint j=0; j<=r; j++) {
			val += src[ri] - fv;
			dst[ti] = round(val*iarr);
			ri+=stride;
			ti+=stride;
		}
		for(guint j=r+1; j<h-r; j++) {
			val += src[ri] - src[li];
			dst[ti] = round(val*iarr);
			li+=stride;
			ri+=stride;
			ti+=stride;
		}
		for(guint j=h-r; j<h; j++) {
			val += lv - src[li];
			dst[ti] = round(val*iarr);
			li+=stride;
			ti+=stride;
		}
	}
}

static gboolean maybe_reallocate_texture(CmkShadowEffect *self, guint width, guint height)
{
	// Expand size for shadow room
	if(self->inset)
	{
		width += self->size * self->dps * 2;
		height += self->size * self->dps * 2;
	}
	else
	{
		width += (self->size+self->spread) * self->dps * 2;
		height += (self->size+self->spread) * self->dps * 2;
	}
	
	// Some GPUs only support power-of-two textures
	if(!self->npot)
	{
		width = next_pot(width);
		height = next_pot(height);
	}
	
	if(self->tex 
	&& self->texData
	&& self->texW == width
	&& self->texH == height)
		return FALSE;
	
	g_clear_pointer(&self->tex, cogl_object_unref);
	g_clear_pointer(&self->texData, g_free);
	g_clear_pointer(&self->texTmp, g_free);
	
	// Will be zeroed when drawing, so don't use g_new0
	self->texData = g_new(guchar, width*height);
	self->texTmp = g_new(guchar, width*height);
	self->tex = cogl_texture_2d_new_with_size(self->ctx, width, height);
	cogl_texture_set_components(self->tex, COGL_TEXTURE_COMPONENTS_A);
	cogl_pipeline_set_layer_texture(self->pipe, 0, self->tex);
	self->texW = width;
	self->texH = height;
	
	return TRUE;
}

static void fill_rect(guchar *data, guint stride, guint x, guint y, guint w, guint h)
{
	guchar *ptr = data + y*stride + x;
	for(guint i=0; i<h; ++i)
	{
		memset(ptr, 255, w);
		ptr += stride;
	}
}

// Each blur should actually blur twice, both to make it look
// better, and to make it go from data -> tmp -> data.
#define blurH(x, y, w, h, r) { \
	boxBlurH(data, tmp, stride, x, y, w, h, r); \
	boxBlurH(tmp, data, stride, x, y, w, h, r); }
#define blurV(x, y, w, h, r) { \
	boxBlurV(data, tmp, stride, x, y, w, h, r); \
	boxBlurV(tmp, data, stride, x, y, w, h, r); }

static void draw_inner_shadow(CmkShadowEffect *self, const guint width, const guint height)
{
	const guint margin = self->size*self->dps;
	const guint stride = self->texW;
	const guint length = self->texW * self->texH;
	guchar *data = self->texData;
	guchar *tmp = self->texTmp;
	memset(data, 0, length);
	
	// Fill the outside area of the shadow
	if(self->l)
		fill_rect(data, stride, 0, margin, margin, height);
	if(self->r)
		fill_rect(data, stride, width + margin, margin, margin, height);
	if(self->t)
		fill_rect(data, stride, margin, 0, width, margin);
	if(self->b)
		fill_rect(data, stride, margin, height + margin, width, margin);

	// Blur
	const guint l = self->l * margin/2;
	const guint r = self->r * margin/2;
	const guint t = self->t * margin/2;
	const guint b = self->b * margin/2;

	if(self->l)
		blurH(0, margin, margin*2, height, l);
	if(self->r)
		blurH(width, margin, margin*2, height, r);
	if(self->t)
		blurV(margin, 0, width, margin*2, t);
	if(self->b)
		blurV(margin, height, width, margin*2, b);
	
	// Upload texture
	cogl_texture_set_data(self->tex,
		COGL_PIXEL_FORMAT_A_8,
		self->texW,
		data,
		0,
		NULL);
	self->invalidated = FALSE;
}

static void draw_outer_shadow(CmkShadowEffect *self, const guint width, const guint height)
{
	const guint margin = self->size*self->dps;
	const guint stride = self->texW;
	const guint length = self->texW * self->texH;
	guchar *data = self->texData;
	guchar *tmp = self->texTmp;
	memset(data, 0, length);
	
	guint sWidth = width + self->spread*self->dps*2;
	guint sHeight = height + self->spread*self->dps*2;
	
	// Fill the center of the region
	for(guint i=0, offset=margin+stride*margin; i<sHeight; ++i)
	{
		memset(data+offset, 255, stride-margin*2);
		offset += stride;
	}
	
	// Blur
	const guint r = self->radius * margin/2;
	blurH(0, 0, margin*2, sHeight+margin*2, r);
	blurH(sWidth, 0, margin*2, sHeight+margin*2, r);
	blurV(0, 0, sWidth+margin*2, margin*2, r);
	blurV(0, sHeight, sWidth+margin*2, margin*2, r);
	
	// Upload texture
	cogl_texture_set_data(self->tex,
		COGL_PIXEL_FORMAT_A_8,
		self->texW,
		data,
		0,
		NULL);
	self->invalidated = FALSE;
}

static void maybe_draw_shadow(CmkShadowEffect *self, const guint width, const guint height)
{
	if(!maybe_reallocate_texture(self, width, height)
	&& !self->invalidated)
		return;
	
	if(self->inset)
		draw_inner_shadow(self, width, height);
	else
		draw_outer_shadow(self, width, height);
}

static void on_paint(ClutterEffect *self_, UNUSED ClutterEffectPaintFlags flags)
{
	CmkShadowEffect *self = CMK_SHADOW_EFFECT(self_);

	ClutterActor *actor = clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self_));
	self->gettingPaintVolume = TRUE;
	const ClutterPaintVolume *vol = clutter_actor_get_paint_volume(actor);
	self->gettingPaintVolume = FALSE;
	float width, height;
	if(vol)
	{
		width = clutter_paint_volume_get_width(vol);
		height = clutter_paint_volume_get_height(vol);
	}
	else
		clutter_actor_get_size(actor, &width, &height);

	self->dps = 1;
	if(CMK_IS_WIDGET(actor))
		self->dps = cmk_widget_get_dp_scale(CMK_WIDGET(actor));

	maybe_draw_shadow(self, width, height);
	
	// Update primitive
	if(self->inset)
	{
		gfloat tx1 = (gfloat)self->size*self->dps / self->texW;
		gfloat ty1 = (gfloat)self->size*self->dps / self->texH;
		gfloat tx2 = 1 - tx1;
		gfloat ty2 = 1 - ty1;
		
		if(!self->npot)
		{
			tx2 = (gfloat)(self->size*self->dps + width) / self->texW;
			ty2 = (gfloat)(self->size*self->dps + height) / self->texH;
		}
		
		CoglVertexP2T2C4 verts[6] = {
			{0,     0,      tx1, ty1, 0, 0, 0, 255},
			{0,     height, tx1, ty2, 0, 0, 0, 255},
			{width, 0,      tx2, ty1, 0, 0, 0, 255},
			
			{width, 0,      tx2, ty1, 0, 0, 0, 255},
			{0,     height, tx1, ty2, 0, 0, 0, 255},
			{width, height, tx2, ty2, 0, 0, 0, 255},
		};
		
		g_clear_pointer(&self->prim, cogl_object_unref);
		self->prim = cogl_primitive_new_p2t2c4(self->ctx, COGL_VERTICES_MODE_TRIANGLES, 6, verts);	
	}
	else
	{
		gfloat tx1 = 0;
		gfloat ty1 = 0;
		gfloat tx2 = 1;
		gfloat ty2 = 1;
		
		if(!self->npot)
		{
			tx2 = (gfloat)((self->size*self->dps + self->spread*self->dps)*2 + width) / self->texW;
			ty2 = (gfloat)((self->size*self->dps + self->spread*self->dps)*2 + height) / self->texH;
		}
		ClutterColor c = {0,0,0,128};
		ClutterActor *actor = clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
		if(CMK_IS_WIDGET(actor))
		{
			const ClutterColor *n = cmk_widget_get_named_color(CMK_WIDGET(actor), "shadow");
			if(n) cmk_copy_color(&c, n);
		}
		
		c.alpha *= clutter_actor_get_paint_opacity(actor)/255.0;
		
		float x = self->x, y = self->y;
		gint m = (self->size+self->spread)*self->dps;
		CoglVertexP2T2C4 verts[6] = {
			{-m+x,       -m+y,       tx1, ty1, c.red, c.green, c.blue, c.alpha},
			{-m+x,       height+m+y, tx1, ty2, c.red, c.green, c.blue, c.alpha},
			{width+m+x,  -m+y,       tx2, ty1, c.red, c.green, c.blue, c.alpha},
			
			{width+m+x,  -m+y,       tx2, ty1, c.red, c.green, c.blue, c.alpha},
			{-m+x,       height+m+y, tx1, ty2, c.red, c.green, c.blue, c.alpha},
			{width+m+x,  height+m+y, tx2, ty2, c.red, c.green, c.blue, c.alpha}
		};
		
		g_clear_pointer(&self->prim, cogl_object_unref);
		self->prim = cogl_primitive_new_p2t2c4(self->ctx, COGL_VERTICES_MODE_TRIANGLES, 6, verts);
	}

	CoglFramebuffer *fb = cogl_get_draw_framebuffer();
	
	if(!self->inset)
		cogl_primitive_draw(self->prim, fb, self->pipe);
	
	clutter_actor_continue_paint(actor);
	
	if(self->inset)
		cogl_primitive_draw(self->prim, fb, self->pipe);
}

static gboolean get_paint_volume(ClutterEffect *self_, ClutterPaintVolume *volume)
{
	CmkShadowEffect *self = CMK_SHADOW_EFFECT(self_);
	if(!self->gettingPaintVolume && !self->inset)
	{
		ClutterActor *actor = clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
		
		self->dps = 1;
		if(CMK_IS_WIDGET(actor))
			self->dps = cmk_widget_get_dp_scale(CMK_WIDGET(actor));

		float m = (self->size*self->radius+self->spread)*self->dps;
		ClutterVertex v = {
			-m,
			-m,
			0
		};
		clutter_paint_volume_set_origin(volume, &v);
		gfloat w = clutter_paint_volume_get_width(volume);
		w += m*2;
		gfloat h = clutter_paint_volume_get_height(volume);
		h += m*2;
		clutter_paint_volume_set_width(volume, w);
		clutter_paint_volume_set_height(volume, h);
	}
	return TRUE;
}


void cmk_shadow_effect_set_size(CmkShadowEffect *self, float size)
{
	g_return_if_fail(CMK_IS_SHADOW_EFFECT(self));
	if(self->size != size)
	{
		self->size = size;
		set_invalidated(self);
	}
}

void cmk_shadow_effect_set(CmkShadowEffect *self, float x, float y, float radius, float spread)
{
	g_return_if_fail(CMK_IS_SHADOW_EFFECT(self));
	if(self->inset || self->x != x || self->y != y || self->radius != radius || self->spread != spread)
	{
		g_clear_object(&self->anim);
		self->inset = FALSE;
		self->x = x;
		self->y = y;
		self->radius = radius;
		self->spread = spread;
		set_invalidated(self);
	}
}

typedef struct
{
	CmkShadowEffect *self;
	float initialRadius;
} RadiusAnimateData;

static void radius_timeline_new_frame(ClutterTimeline *timeline, UNUSED gint msecs, RadiusAnimateData *data)
{
	data->self->radius = data->initialRadius + (data->self->tRadius - data->initialRadius)*clutter_timeline_get_progress(timeline);
	set_invalidated(data->self);
}

void cmk_shadow_effect_animate_radius(CmkShadowEffect *self, float radius)
{
	g_return_if_fail(CMK_IS_SHADOW_EFFECT(self));
	g_return_if_fail(!self->inset);
	if(self->tRadius != radius)
	{
		g_clear_object(&self->anim);
		self->anim = clutter_timeline_new(100);
		RadiusAnimateData *data = g_new(RadiusAnimateData, 1);
		data->self = self;
		data->initialRadius = self->radius;
		self->tRadius = radius;
		g_signal_connect_data(self->anim, "new-frame", G_CALLBACK(radius_timeline_new_frame), data, (GClosureNotify)g_free, 0);
		clutter_timeline_start(self->anim);
	}
}

void cmk_shadow_effect_set_inset(CmkShadowEffect *self, float l, float r, float t, float b)
{
	g_return_if_fail(CMK_IS_SHADOW_EFFECT(self));
	if(!self->inset || self->l != l || self->r != r || self->t != t || self->b != b)
	{
		g_clear_object(&self->anim);
		self->inset = TRUE;
		self->l = self->tL = l;
		self->r = self->tR = r;
		self->t = self->tT = t;
		self->b = self->tB = b;
		set_invalidated(self);
	}
}

typedef struct
{
	CmkShadowEffect *self;
	float iL, iR, iT, iB;
} InsetAnimateData;

static void inset_timeline_new_frame(ClutterTimeline *timeline, UNUSED gint msecs, InsetAnimateData *data)
{
	float progress = clutter_timeline_get_progress(timeline);
	data->self->l = data->iL + (data->self->tL - data->iL)*progress;
	data->self->r = data->iR + (data->self->tR - data->iR)*progress;
	data->self->t = data->iT + (data->self->tT - data->iT)*progress;
	data->self->b = data->iB + (data->self->tB - data->iB)*progress;
	set_invalidated(data->self);
}

void cmk_shadow_effect_inset_animate_edges(CmkShadowEffect *self, float l, float r, float t, float b)
{
	g_return_if_fail(CMK_IS_SHADOW_EFFECT(self));
	g_return_if_fail(self->inset);
	if(self->tL != l || self->tR != r || self->tT != t || self->tB != b)
	{
		g_clear_object(&self->anim);
		self->anim = clutter_timeline_new(100);
		InsetAnimateData *data = g_new(InsetAnimateData, 1);
		data->self = self;
		data->iL = self->l;
		data->iR = self->r;
		data->iT = self->t;
		data->iB = self->b;
		self->tL = l;
		self->tR = r;
		self->tT = t;
		self->tB = b;
		g_signal_connect_data(self->anim, "new-frame", G_CALLBACK(inset_timeline_new_frame), data, (GClosureNotify)g_free, 0);
		clutter_timeline_start(self->anim);
	}
}
