/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 *
 * TODO: cogl.h uses <cogl/..> includes for both the cogl and cogl2 version
 * which means that including cogl2 with clutter doesn't work correctly.
 * Clutter seems to mix both cogl and cogl2 somehow, and this shadow code
 * uses cogl2. Because of the weird includes, many of the cogl2 functions
 * (and some of the Clutter ones??) say implicit declaration.
 */

#include "cmk-shadow.h"
#include <math.h>
#include <string.h>

struct _CmkShadow
{
	ClutterActor parent;
	ClutterActor *shadow;
	ClutterCanvas *canvas;
	guint radius;
	guint shadowMask;
};

static void cmk_shadow_dispose(GObject *self_);
static void cmk_shadow_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void cmk_shadow_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);
static void cmk_shadow_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
static void on_shadow_size_changed(ClutterActor *shadow, GParamSpec *spec, gpointer userdata);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkShadow *self);

G_DEFINE_TYPE(CmkShadow, cmk_shadow, CMK_TYPE_WIDGET);



CmkShadow * cmk_shadow_new()
{
	return CMK_SHADOW(g_object_new(CMK_TYPE_SHADOW, NULL));
}

CmkShadow * cmk_shadow_new_full(guint shadowMask, gfloat radius)
{
	CmkShadow *self = CMK_SHADOW(g_object_new(CMK_TYPE_SHADOW, NULL));
	cmk_shadow_set_mask(self, shadowMask);
	cmk_shadow_set_radius(self, radius);
	return self;
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
	self->shadow = clutter_actor_new();

	self->canvas = CLUTTER_CANVAS(clutter_canvas_new());
	g_signal_connect(self->canvas, "draw", G_CALLBACK(on_draw_canvas), self);
	clutter_actor_set_content_gravity(CLUTTER_ACTOR(self), CLUTTER_CONTENT_GRAVITY_CENTER);
	clutter_actor_set_content(self->shadow, CLUTTER_CONTENT(self->canvas));

	g_signal_connect(self->shadow, "notify::size", G_CALLBACK(on_shadow_size_changed), NULL);
	clutter_actor_add_child(CLUTTER_ACTOR(self), self->shadow);
}

static void cmk_shadow_dispose(GObject *self_)
{
	CmkShadow *self = CMK_SHADOW(self_);
	G_OBJECT_CLASS(cmk_shadow_parent_class)->dispose(self_);
}

static void cmk_shadow_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	*minWidth = 0;
	*natWidth = 0;
	ClutterActor *child = cmk_shadow_get_first_child(CMK_SHADOW(self_));
	if(child)
		clutter_actor_get_preferred_width(child, forHeight, minWidth, natWidth);
}

static void cmk_shadow_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	*minHeight = 0;
	*natHeight = 0;
	ClutterActor *child = cmk_shadow_get_first_child(CMK_SHADOW(self_));
	if(child)
		clutter_actor_get_preferred_height(child, forWidth, minHeight, natHeight);
}

static void cmk_shadow_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	CmkShadow *self = CMK_SHADOW(self_);
	ClutterActor *child = clutter_actor_get_child_at_index(self_, 1);

	gfloat width = box->x2 - box->x1;
	gfloat height = box->y2 - box->y1;
	
	if(child)
	{
		ClutterActorBox childBox = {0, 0, width, height};
		clutter_actor_allocate(child, &childBox, flags);
	}

	gboolean mTop = (self->shadowMask & CMK_SHADOW_MASK_TOP) == CMK_SHADOW_MASK_TOP;
	gboolean mBottom = (self->shadowMask & CMK_SHADOW_MASK_BOTTOM) == CMK_SHADOW_MASK_BOTTOM;
	gboolean mLeft = (self->shadowMask & CMK_SHADOW_MASK_LEFT) == CMK_SHADOW_MASK_LEFT;
	gboolean mRight = (self->shadowMask & CMK_SHADOW_MASK_RIGHT) == CMK_SHADOW_MASK_RIGHT;

	// Set the shadow box's size depending on which sides get a shadow
	// The base size is the CmkShadow's allocated size, but then the
	// shadow box expands off the edges
	ClutterActorBox shadowBox = {
		mLeft ? (-(gfloat)self->radius) : 0,
		mTop ? (-(gfloat)self->radius) : 0,
		width + (mRight ? self->radius : 0),
		height + (mBottom ? self->radius : 0)
	};

	// If the CmkShadow's box gets too small, it can mess with the shadow
	// rendering. This makes sure the width and/or height (depending on
	// edges being rendered) is always > self->radius*2, which is enough
	// for the blurring code to not segfault. (The blur code also validates
	// the width and height before attempting to render just in case, but
	// if it errors it just doesn't render anything, which looks bad.)
	// 
	// Because of this expansion, a black region may start appearing off
	// of the opposite edge of the shadow (required due to how the blurring
	// works). To fix this in the easiest possible way, just use Clutter's
	// clip function to hide the extra pixels created by the expansion.
	//
	// TODO: This still has issues when rendering shadows on opposite edges
	// (ex TOP + BOTTOM, ALL) on very small areas. It won't segfault, but the
	// blurring starts looking very choppy and eventually disspears completely.
	// This might be fixed by also expanding the region in this case?
	guint clipTop = 0, clipBottom = 0, clipLeft = 0, clipRight = 0;
	gfloat prev;

	if(mLeft != mRight // If left XOR right
	&& (shadowBox.x2 - shadowBox.x1) <= self->radius*2)
	{
		if(mLeft)
		{
			prev = shadowBox.x2;
			shadowBox.x2 = shadowBox.x1 + self->radius*2 + 1;
			clipRight = shadowBox.x2 - prev;
		}
		else
		{
			prev = shadowBox.x1;
			shadowBox.x1 = shadowBox.x2 - self->radius*2 - 1;
			clipLeft = prev - shadowBox.x1;
		}
	}

	if(mTop != mBottom
	&& (shadowBox.y2 - shadowBox.y1) <= self->radius*2)
	{
		if(mTop)
		{
			prev = shadowBox.y2;
			shadowBox.y2 = shadowBox.y1 + self->radius*2 + 1;
			clipBottom = shadowBox.y2 - prev;
		}
		else
		{
			prev = shadowBox.y1;
			shadowBox.y1 = shadowBox.y2 - self->radius*2 - 1;
			clipTop = prev - shadowBox.y1;
		}
	}

	clutter_actor_allocate(self->shadow, &shadowBox, flags);

	// The clip is specified in distance from each edge, while the set_clip
	// method takes a x,y,width,height. So convert it.
	clutter_actor_set_clip(self->shadow, clipLeft, clipTop, (shadowBox.x2-shadowBox.x1)-clipLeft-clipRight, (shadowBox.y2-shadowBox.y1)-clipTop-clipBottom);
	
	CLUTTER_ACTOR_CLASS(cmk_shadow_parent_class)->allocate(self_, box, flags);
}

static void on_shadow_size_changed(ClutterActor *shadow, GParamSpec *spec, gpointer userdata)
{
	gfloat width, height;
	clutter_actor_get_size(shadow, &width, &height);
	clutter_canvas_set_size(CLUTTER_CANVAS(clutter_actor_get_content(shadow)), width, height);
}

void cmk_shadow_set_mask(CmkShadow *self, guint shadowMask)
{
	g_return_if_fail(CMK_IS_SHADOW(self));
	self->shadowMask = shadowMask;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

void cmk_shadow_set_radius(CmkShadow *self, guint radius)
{
	g_return_if_fail(CMK_IS_SHADOW(self));
	self->radius = radius;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

ClutterActor * cmk_shadow_get_first_child(CmkShadow *shadow)
{
	// The internal shadow actor takes up index 0, so the first child added
	// by a user will be at index 1.
	return clutter_actor_get_child_at_index(CLUTTER_ACTOR(shadow), 1);
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

//static unsigned long getms()
//{
//	struct timeval tv;
//	gettimeofday(&tv, NULL);
//	return 1000000 * tv.tv_sec + tv.tv_usec;
//}

/*
 * Draws a shadow onto the canvas. In reality, this just creates a blurred
 * black box. Only the edges specified in the shadow mask are actually
 * blurred, plus some optimizations are performed (like not filling/blurring
 * the center of the canvas where the "shadow-creating" object will be
 * covering this shadow actor).
 */
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, gint width, gint height, CmkShadow *self)
{
	cairo_surface_t *surface = cairo_get_target(cr);

	gboolean mTop = (self->shadowMask & CMK_SHADOW_MASK_TOP) == CMK_SHADOW_MASK_TOP;
	gboolean mBottom = (self->shadowMask & CMK_SHADOW_MASK_BOTTOM) == CMK_SHADOW_MASK_BOTTOM;
	gboolean mLeft = (self->shadowMask & CMK_SHADOW_MASK_LEFT) == CMK_SHADOW_MASK_LEFT;
	gboolean mRight = (self->shadowMask & CMK_SHADOW_MASK_RIGHT) == CMK_SHADOW_MASK_RIGHT;

	// Make sure we should actually do the shadow. Some radius/width/height
	// combinations could cause a segfault, so be very careful
	if(cairo_image_surface_get_format(surface) != CAIRO_FORMAT_ARGB32
	|| (self->radius == 0)
	|| (!mTop && !mBottom && !mLeft && !mRight)
	|| ((mLeft || mRight) && width <= self->radius*2)
	|| ((mTop || mBottom) && height <= self->radius*2))
	{
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);
		cairo_restore(cr);
		return TRUE;
	}

	// Get a pixel buffer to draw the shadow into
	guchar *buffer = cairo_image_surface_get_data(surface);
	guint length = width*height;

	// These buffers contain alpha-only versions of the shadow.
	// There are two for a "double buffer" usage, of which 'source'
	// will eventually be copied to buffer.
	guchar *source = g_new(guchar, length*2);
	guchar *osource = source;
	guchar *dest = source + length;
	guchar *tmp = NULL; // For buffer swaps

	// This creates an entirely black canvas, except for the edges that
	// should be blurred (on those edges, a border of size radius is created). 
	// This is pretty much the fastest I could get it: on my device, an area
	// of ~2000x1000 gets filled with borders in ~1ms.
	memset(source, 255, length*2);
	if(mTop)
	{
		memset(source, 0, width*self->radius);
		memset(dest, 0, width*self->radius);
	}
	if(mBottom)
	{
		memset(source+(guint)((height-self->radius)*width), 0, width*self->radius);
		memset(dest+(guint)((height-self->radius)*width), 0, width*self->radius);
	}
	if(mLeft)
	{
		guint yStart = mTop ? self->radius : 0;
		guint yEnd = mBottom ? (height-self->radius) : height;
		for(guint y=yStart; y<yEnd; ++y)
			memset(source+(y*width), 0, self->radius);
		for(guint y=yStart; y<yEnd; ++y)
			memset(dest+(y*width), 0, self->radius);
	}
	if(mRight)
	{
		guint yStart = mTop ? self->radius : 0;
		guint yEnd = mBottom ? (height-self->radius) : height;
		for(guint y=yStart; y<yEnd; ++y)
			memset(source+(guint)((y+1)*width-self->radius), 0, self->radius);
		for(guint y=yStart; y<yEnd; ++y)
			memset(dest+(guint)((y+1)*width-self->radius), 0, self->radius);
	}
	
	// By only blurring the required edges, render time goes from
	// ~30ms for ~2000x1000 area to under 4ms (on my device).
	// However, because blurring on an edge only transfers that set of
	// pixels, the pixels have to be transfered back to the source buffer
	// after a blur. Luckily, we want to do two passes of the blur anyway
	// for better looks. So just blur in sets of two on the same edge,
	// and solve both problems at once!
	if(mTop)
	{
		boxBlurV(source, dest, width, 0, 0, width, self->radius*2, self->radius/2);
		boxBlurV(dest, source, width, 0, 0, width, self->radius*2, self->radius/2);
	}
	if(mBottom)
	{
		boxBlurV(source, dest, width, 0, height - self->radius*2 - 1, width, self->radius*2, self->radius/2);
		boxBlurV(dest, source, width, 0, height - self->radius*2 - 1, width, self->radius*2, self->radius/2);
	}
	if(mLeft)
	{
		boxBlurH(source, dest, width, 0, 0, self->radius*2, height, self->radius/2);
		boxBlurH(dest, source, width, 0, 0, self->radius*2, height, self->radius/2);
	}
	if(mRight)
	{
		boxBlurH(source, dest, width, width - self->radius*2 - 1, 0, self->radius*2, height, self->radius/2);
		boxBlurH(dest, source, width, width - self->radius*2 - 1, 0, self->radius*2, height, self->radius/2);
	}

	// Copy result to cairo
	memset(buffer, 0, length*4);

	//unsigned long start = getms();
	
	// Just this loop alone takes more time than generating the entire
	// shadow. Fix? Possibly instead of using the extra source and dest
	// buffers, modify the blur functions to take channels into account
	// and just blur between this cairo buffer and a tmp buffer.
	for(guint i=0,j=3;i<length;++i,j+=4)
		buffer[j]=source[i];

	//unsigned long end = getms();
	//unsigned long delta = end - start;
	//double deltams = delta / 1000.0;
	//printf("shadowtime: %fms (for w: %i, h: %i, r: %i)\n", deltams, width, height, self->radius);

	g_free(osource);
	return TRUE;
}



#include <cogl/cogl.h>

struct _CmkShadoutil
{
	GObject parent;
	
	ClutterActor *actor;
	CoglContext *ctx;
	CoglPipeline *pipe;
	gboolean npot;
	
	CmkShadowMode mode;
	guint size;
	gfloat l, r, t, b;
	gboolean invalidated;
	
	CoglTexture *tex;
	guchar *texData;
	guchar *texTmp;
	guint texW, texH;
	
	CoglPrimitive *prim;
};

enum
{
	PROP_TOP = 1,
	PROP_BOTTOM,
	PROP_LEFT,
	PROP_RIGHT,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_shadoutil_dispose(GObject *self_);
static void cmk_shadoutil_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_shadoutil_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void set_invalidated(CmkShadoutil *self);

G_DEFINE_TYPE(CmkShadoutil, cmk_shadoutil, G_TYPE_OBJECT);



CmkShadoutil * cmk_shadoutil_new(void)
{
	return CMK_SHADOUTIL(g_object_new(CMK_TYPE_SHADOUTIL, NULL));
}

static void cmk_shadoutil_class_init(CmkShadoutilClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_shadoutil_dispose;
	base->set_property = cmk_shadoutil_set_property;
	base->get_property = cmk_shadoutil_get_property;

	properties[PROP_TOP] = g_param_spec_float("top", "top", "top", 0, 1, 0, G_PARAM_READWRITE);
	properties[PROP_BOTTOM] = g_param_spec_float("bottom", "bottom", "bottom", 0, 1, 0, G_PARAM_READWRITE);
	properties[PROP_LEFT] = g_param_spec_float("left", "left", "left", 0, 1, 0, G_PARAM_READWRITE);
	properties[PROP_RIGHT] = g_param_spec_float("right", "right", "right", 0, 1, 0, G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_shadoutil_init(CmkShadoutil *self)
{
	self->ctx = clutter_backend_get_cogl_context(clutter_get_default_backend());
	self->pipe = cogl_pipeline_new(self->ctx);
	cogl_pipeline_set_color4ub(self->pipe, 0, 0, 0, 255);
	self->npot = cogl_has_feature(self->ctx, COGL_FEATURE_ID_TEXTURE_NPOT_BASIC);
}

static void cmk_shadoutil_dispose(GObject *self_)
{
	CmkShadoutil *self = CMK_SHADOUTIL(self_);
	g_clear_pointer(&self->tex, cogl_object_unref);
	g_clear_pointer(&self->prim, cogl_object_unref);
	g_clear_pointer(&self->pipe, cogl_object_unref);
	g_clear_pointer(&self->texData, g_free);
	g_clear_pointer(&self->texTmp, g_free);
	G_OBJECT_CLASS(cmk_shadoutil_parent_class)->dispose(self_);
}

static void cmk_shadoutil_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	CmkShadoutil *self = CMK_SHADOUTIL(self_);
	
	switch(propertyId)
	{
	case PROP_TOP:
		self->t = g_value_get_float(value);
		set_invalidated(self);
		break;
	case PROP_BOTTOM:
		self->b = g_value_get_float(value);
		set_invalidated(self);
		break;
	case PROP_LEFT:
		self->l = g_value_get_float(value);
		set_invalidated(self);
		break;
	case PROP_RIGHT:
		self->r = g_value_get_float(value);
		set_invalidated(self);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_shadoutil_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	CmkShadoutil *self = CMK_SHADOUTIL(self_);
	
	switch(propertyId)
	{
	case PROP_TOP:
		g_value_set_float(value, self->t);
		break;
	case PROP_BOTTOM:
		g_value_set_float(value, self->b);
		break;
	case PROP_LEFT:
		g_value_set_float(value, self->l);
		break;
	case PROP_RIGHT:
		g_value_set_float(value, self->r);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void set_invalidated(CmkShadoutil *self)
{
	self->invalidated = TRUE;
	if(self->actor)
		clutter_actor_queue_redraw(self->actor);
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

static gboolean maybe_update_texture(CmkShadoutil *self, ClutterActorBox *box)
{
	guint width = (guint)(box->x2-box->x1);
	guint height = (guint)(box->y2-box->y1);

	// Expand size for shadow room (needed for both drop and inner shadows)
	width += self->size * 2;
	height += self->size * 2;
	
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
	g_clear_pointer(&self->prim, cogl_object_unref);
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
	
	width = (guint)(box->x2-box->x1);
	height = (guint)(box->y2-box->y1);
	
	gfloat tx1 = (gfloat)self->size / self->texW;
	gfloat ty1 = (gfloat)self->size / self->texH;
	gfloat tx2 = 1 - tx1;
	gfloat ty2 = 1 - ty1;
	
	if(!self->npot)
	{
		tx2 = (gfloat)(self->size + width) / self->texW;
		ty2 = (gfloat)(self->size + height) / self->texH;
	}
	
	CoglVertexP2T2C4 verts[6] = {
		0,     0,      tx1, ty1, 0, 0, 0, 255,
		0,     height, tx1, ty2, 0, 0, 0, 255,
		width, 0,      tx2, ty1, 0, 0, 0, 255,
		
		width, 0,      tx2, ty1, 0, 0, 0, 255,
		0,     height, tx1, ty2, 0, 0, 0, 255,
		width, height, tx2, ty2, 0, 0, 0, 255,
	};
	
	self->prim = cogl_primitive_new_p2t2c4(self->ctx, COGL_VERTICES_MODE_TRIANGLES, 6, verts);	
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

static void maybe_draw_shadow(CmkShadoutil *self, ClutterActorBox *box)
{
	if(!maybe_update_texture(self, box)
	&& !self->invalidated)
		return;
	
	gboolean inner = (self->mode == CMK_SHADOW_MODE_INNER);
	const guint width = (guint)(box->x2-box->x1);
	const guint height = (guint)(box->y2-box->y1);
	const guint margin = self->size;
	const guint stride = self->texW;
	const guint length = self->texW * self->texH;
	guchar *data = self->texData;
	guchar *tmp = self->texTmp;
	
	memset(data, 0, length);
	
	// 1/2 size because of the double blur
	const guint l = self->l * self->size/2;
	const guint r = self->r * self->size/2;
	const guint t = self->t * self->size/2;
	const guint b = self->b * self->size/2;
	
	// Fill in rectangles on the inside if drawing a drop shadow
	// (inner=FALSE), or on the outside if drawing inner shadow.
	// Multiply times boolean for 0 or 'margin'.
	if(self->l)
		fill_rect(data, stride, margin*!inner, margin, margin, height);
	if(self->r)
		fill_rect(data, stride, width + margin*inner, margin, margin, height);
	if(self->t)
		fill_rect(data, stride, margin, margin*!inner, width, margin);
	if(self->b)
		fill_rect(data, stride, margin, height + margin*inner, width, margin);
	
	memcpy(tmp, data, length);
	
	// Each blur should actually blur twice, both to make it look
	// better, and to make it go from data -> tmp -> data.
	#define blurH(x, y, w, h, r) { \
		boxBlurH(data, tmp, stride, x, y, w, h, r); \
		boxBlurH(tmp, data, stride, x, y, w, h, r); }
	#define blurV(x, y, w, h, r) { \
		boxBlurV(data, tmp, stride, x, y, w, h, r); \
		boxBlurV(tmp, data, stride, x, y, w, h, r); }
	
	// Do the blur
	if(self->l)
		blurH(0, margin, margin*2, height, l);
	if(self->r)
		blurH(width, margin, margin*2, height, r);
	if(self->t)
		blurV(margin, 0, width, margin*2, t);
	if(self->b)
		blurV(margin, height, width, margin*2, b);
	
	cogl_texture_set_data(self->tex,
		COGL_PIXEL_FORMAT_A_8,
		self->texW,
		data,
		0,
		NULL);
	self->invalidated = FALSE;
}

void cmk_shadoutil_set_mode(CmkShadoutil *self, CmkShadowMode mode)
{
	g_return_if_fail(CMK_IS_SHADOUTIL(self));
	if(self->mode != mode)
	{
		self->mode = mode;
		set_invalidated(self);
	}
}

CmkShadowMode cmk_shadoutil_get_mode(CmkShadoutil *self)
{
	g_return_val_if_fail(CMK_IS_SHADOUTIL(self), CMK_SHADOW_MODE_OUTER);
	return self->mode;
}

void cmk_shadoutil_set_size(CmkShadoutil *self, guint size)
{
	g_return_if_fail(CMK_IS_SHADOUTIL(self));
	if(self->size != size)
	{
		self->size = size;
		set_invalidated(self);
	}
}

guint cmk_shadoutil_get_size(CmkShadoutil *self)
{
	g_return_val_if_fail(CMK_IS_SHADOUTIL(self), 0);
	return self->size;
}

void cmk_shadoutil_set_edges(CmkShadoutil *self, gfloat l, gfloat r, gfloat t, gfloat b)
{
	g_return_if_fail(CMK_IS_SHADOUTIL(self));
	if(self->l != l || self->r != r || self->t != t || self->b != b)
	{
		self->l = l;
		self->r = r;
		self->t = t;
		self->b = b;
		set_invalidated(self);
	}
}

void cmk_shadoutil_get_edges(CmkShadoutil *self, gfloat *l, gfloat *r, gfloat *t, gfloat *b)
{
	*l = 0;
	*r = 0;
	*t = 0;
	*b = 0;

	g_return_if_fail(CMK_IS_SHADOUTIL(self));

	*l = self->l;
	*r = self->r;
	*t = self->t;
	*b = self->b;
}

void cmk_shadoutil_set_actor(CmkShadoutil *self, ClutterActor *actor)
{
	g_return_if_fail(CMK_IS_SHADOUTIL(self));
	self->actor = actor;
}

void cmk_shadoutil_paint(CmkShadoutil *self, ClutterActorBox *box)
{
	g_return_if_fail(CMK_IS_SHADOUTIL(self));
	maybe_draw_shadow(self, box);
	
	CoglFramebuffer *fb = cogl_get_draw_framebuffer();
	cogl_primitive_draw(self->prim, fb, self->pipe);
}

