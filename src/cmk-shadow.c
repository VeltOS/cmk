/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "cmk-shadow.h"

typedef enum
{
	SHADOW_RECTANGLE,
	SHADOW_RECTANGLE_INNER, // TODO
	//SHADOW_CIRCLE, // TODO
	//SHADOW_CIRCLE_INNER, // TODO

	// This can be any shape, but is very slow compared to
	// the optimized methods used to draw the other types
	SHADOW_ANY,

	SHADOW_ANY_INNER, // TODO
} ShadowType;

typedef struct
{
	cairo_surface_t *surface;
	unsigned char *tmp; // temp buffer for two-pass box blur
	cairo_path_t *path; // only use for pointer comparisons
	ShadowType type;
	float radius, percent;
} ShadowCache;

struct _CmkShadow
{
	GObject parent;

	double x, y, width, height;
	cairo_path_t *path;

	ShadowType type;
	float radius, percent;

	ShadowCache cache;
};

enum
{
	PROP_RADIUS = 1,
	PROP_PERCENT,
	PROP_LAST,
};

static GParamSpec *properties[PROP_LAST];

G_DEFINE_TYPE(CmkShadow, cmk_shadow, G_TYPE_OBJECT);

static void on_dispose(GObject *self_);
static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec);
static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec);
static void get_path_extents(cairo_path_t *path, double *ox1, double *oy1, double *ox2, double *oy2);


CmkShadow * cmk_shadow_new(float radius)
{
	return CMK_SHADOW(g_object_new(CMK_TYPE_SHADOW,
		"radius", radius,
		NULL));
}

static void cmk_shadow_class_init(CmkShadowClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;
	base->get_property = get_property;
	base->set_property = set_property;

	/**
	 * CmkWidget:radius:
	 *
	 * The maxiumum radius of the shadow, in cairo units.
	 */
	properties[PROP_RADIUS] =
		g_param_spec_float("radius", "radius", "radius",
		                   0, G_MAXFLOAT, 0,
		                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * CmkWidget:percent:
	 *
	 * Radius of the shadow, as a percentage from 0 to 1 of
	 * the maxiumum percent.
	 */
	properties[PROP_PERCENT] =
		g_param_spec_float("percent", "percent", "percent",
		                   0, 1, 1,
		                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_shadow_init(UNUSED CmkShadow *self)
{
}

static void on_dispose(GObject *self_)
{
	CmkShadow *self = CMK_SHADOW(self_);
	g_clear_pointer(&self->path, cairo_path_destroy);
	g_clear_pointer(&self->cache.surface, cairo_surface_destroy);
	g_clear_pointer(&self->cache.tmp, g_free);
	G_OBJECT_CLASS(cmk_shadow_parent_class)->dispose(self_);
}

static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec)
{
	CmkShadow *self = CMK_SHADOW(self_);
	
	switch(id)
	{
	case PROP_RADIUS:
		g_value_set_float(value, self->radius);
		break;
	case PROP_PERCENT:
		g_value_set_float(value, self->percent);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec)
{
	CmkShadow *self = CMK_SHADOW(self_);

	switch(id)
	{
	case PROP_RADIUS:
		self->radius = g_value_get_float(value);
		break;
	case PROP_PERCENT:
		cmk_shadow_set_percent(self, g_value_get_float(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

/*
 * These blur functions are modified versions of the algorithms
 * descripted in the "Fastest Gaussion Blur" article:
 * http://blog.ivank.net/fastest-gaussian-blur.html
 * The difference is that they can blur a specific region within
 * the buffer, which allows for edge-only blurring.
 */

/*
 * x + r <= ~stride/2 (?)
 * y + h <= buffer height
 * w <= stride
 * 0 < r
 */
static void boxBlurH(unsigned char *src, unsigned char *dst, unsigned int stride, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int r)
{
	float iarr = 1.0 / (r+r+1.0);
	for(unsigned int i=y; i<y+h; i++)
	{
		unsigned int ti = i*stride+x, li = ti, ri = ti+r;
		unsigned int fv = src[ti], lv = src[ti+w-1], val = (r+1)*fv;
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

static void boxBlurV(unsigned char *src, unsigned char *dst, unsigned int stride, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int r)
{
	float iarr = 1.0 / (r+r+1.0);
	for(unsigned int i=x; i<x+w; i++)
	{
		unsigned int ti = i+y*stride, li = ti, ri = ti+r*stride;
		unsigned int fv = src[ti], lv = src[ti+stride*(h-1)], val = (r+1)*fv;
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

// Each blur should actually blur twice, both to make it look
// better, and to make it go from data -> tmp -> data.
static inline void blurH(unsigned char *src, unsigned char *dst, unsigned int stride, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int r)
{
	boxBlurH(src, dst, stride, x, y, w, h, r);
	boxBlurH(dst, src, stride, x, y, w, h, r);
}

static inline void blurV(unsigned char *src, unsigned char *dst, unsigned int stride, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int r)
{
	boxBlurV(src, dst, stride, x, y, w, h, r);
	boxBlurV(dst, src, stride, x, y, w, h, r);
}

static void fill_rect(
	unsigned char *data,
	unsigned int stride,
	unsigned int x,
	unsigned int y,
	unsigned int w,
	unsigned int h)
{
	unsigned char *ptr = data + (y * stride) + x;
	for(guint i = 0; i < h; ++i)
	{
		memset(ptr, 255, w);
		ptr += stride;
	}
}

static inline bool fequal(float a, float b)
{
	// There are better ways to tests this, but it works here
	return fabs(a - b) < FLT_EPSILON;
}

// Returns true if the surface must be redrawn
bool check_surface(CmkShadow *self, const unsigned int width, unsigned int height, ShadowType type, bool *clear)
{
	if(!self->cache.surface
	|| (unsigned int)cairo_image_surface_get_width(self->cache.surface) != width
	|| (unsigned int)cairo_image_surface_get_height(self->cache.surface) != height)
	{
		g_clear_pointer(&self->cache.surface, cairo_surface_destroy);
		g_clear_pointer(&self->cache.tmp, g_free);
		self->cache.surface =
			cairo_image_surface_create(CAIRO_FORMAT_A8, width, height);

		unsigned int stride = cairo_image_surface_get_stride(self->cache.surface);
		self->cache.tmp = g_new(unsigned char, stride * height);

		self->cache.type = type;
		self->cache.path = self->path;
		self->cache.radius = self->radius;
		self->cache.percent = self->percent;
		if(clear)
			*clear = false;
		return true;
	}

	if(self->cache.type == type
	&& ((type != SHADOW_ANY && type != SHADOW_ANY_INNER) || self->cache.path == self->path)
	&& fequal(self->cache.radius, self->radius)
	&& fequal(self->cache.percent, self->percent))
	{
		// Nothing has changed, use cached surface
		if(clear)
			*clear = false;
		return false;
	}

	self->cache.type = type;
	self->cache.path = self->path;
	self->cache.radius = self->radius;
	self->cache.percent = self->percent;

	// Clear surface for new drawing
	unsigned int stride = cairo_image_surface_get_stride(self->cache.surface);
	unsigned char *data = cairo_image_surface_get_data(self->cache.surface);
	memset(data, 0, stride * height);
	if(clear)
		*clear = true;
	return true;
}

// Generic shadow generation for any shape, but very slow
void draw_any_shadow_surface(CmkShadow *self, const float xscale, const float yscale)
{
	const unsigned int swidth = ceil((self->width + self->radius * 2) * xscale);
	const unsigned int sheight = ceil((self->height + self->radius * 2) * yscale);

	const unsigned int hradius = ceil(self->radius * self->percent * xscale);
	const unsigned int vradius = ceil(self->radius * self->percent * yscale);

	bool clear;
	if(!check_surface(self, swidth, sheight, SHADOW_ANY, &clear))
		return; // Cached, no need to redraw

	// Fill in the given path onto the surface, offset by the max radius
	cairo_t *cr = cairo_create(self->cache.surface);

	if(clear)
	{
		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);
	}

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_scale(cr, xscale, yscale);
	cairo_translate(cr, self->radius, self->radius);

	if(self->type == SHADOW_ANY && self->path)
		cairo_append_path(cr, self->path);
	else if(self->type == SHADOW_RECTANGLE)
		cairo_rectangle(cr, 0, 0, self->width, self->height);
	cairo_fill(cr);

	cairo_destroy(cr);
	cairo_surface_flush(self->cache.surface);

	// Blur the shape
	if(hradius > 0 || vradius > 0)
	{
		unsigned int stride = cairo_image_surface_get_stride(self->cache.surface);
		unsigned char *data = cairo_image_surface_get_data(self->cache.surface);

		// VERY SLOW
		blurH(data, self->cache.tmp, stride, 0, 0, swidth, sheight, hradius / 2);
		blurV(data, self->cache.tmp, stride, 0, 0, swidth, sheight, vradius / 2);
	}

	cairo_surface_mark_dirty(self->cache.surface);
}

static void draw_any_shadow(CmkShadow *self, cairo_t *cr, const float xscale, const float yscale)
{
	draw_any_shadow_surface(self, xscale, yscale);
	cairo_translate(cr, -self->radius - self->x, -self->radius - self->y);
	cairo_scale(cr, 1 / xscale, 1 / yscale);
	cairo_mask_surface(cr, self->cache.surface, 0, 0);
}

static void draw_rectangle_shadow_surface(CmkShadow *self, const float scale)
{
	/*
	 * Since rectangular shadows are symmetric, only one corner
	 * and a one-pixel sliver of a side needs to be rendered.
	 * This function creates a (alpha-only) bitmap looking like:
	 *
	 *        --------------------------
	 *        -                        -
	 *        -                        -
	 *        -                        -
	 *        -        *****************
	 * <----margin---->*      ^       **
	 * <-inv->-<------>*      |       **
	 *        - radius *<---margin--->**
	 *        -        *      |       **
	 *        -        *      |       **
	 *        -        *      v       **
	 *        ---------*****************
	 *            ^          ^         ^---- sliver, for edge-repeating
	 *      blur here  initally fully
	 *                 opaque, but blured
	 *                 into
	 *
	 * This would then be placed at the top-left corner, and painted
	 * such that the sliver is repeated along the x-axis. This is
	 * then rotated 90 degrees + translated so that it aligns with
	 * the top-right corner with edge along right side y-axis. And
	 * again for the bottom and left sides.
	 *
	 * radius can be anywhere between 0 and self->radius, depending on
	 * the percentage of blur currently being used (often for animation).
	 */

	const unsigned int margin = ceil(self->radius * scale);
	const unsigned int radius = ceil(self->radius * self->percent * scale);
	const unsigned int swidth = margin * 2 + 1; // +1 for edge sliver
	const unsigned int sheight = margin * 2;
	const unsigned int inv = margin - radius;

	bool clear;
	if(!check_surface(self, swidth, sheight, SHADOW_RECTANGLE, &clear))
		return; // Cached, no need to redraw

	const unsigned int stride = cairo_image_surface_get_stride(self->cache.surface);
	unsigned char *data = cairo_image_surface_get_data(self->cache.surface);

	if(clear)
		memset(data, 0, stride * sheight);

	fill_rect(data, stride, margin, margin, margin + 1, margin);

	if(radius > 0)
	{
		blurV(data, self->cache.tmp, stride,
			margin, inv,
			margin + 1, radius * 2,
			radius / 2);
		blurH(data, self->cache.tmp, stride,
			inv, inv,
			radius * 2, margin + radius,
			radius / 2);
	}

	cairo_surface_mark_dirty(self->cache.surface);
}

static void draw_rectangle_shadow(CmkShadow *self, cairo_t *cr, const float xscale, const float yscale)
{
	// 1/4th of the shadow is drawn and then rotated and mirrored
	// for the other sides, so only one scale value can be used.
	const float scale = MAX(xscale, yscale);

	// Draw (part of) the shadow to an image surface
	// This usually takes over 90% of the time
	// for this entire shadow-drawing process.
	draw_rectangle_shadow_surface(self, scale);

	// Convert surface to pattern
	// The extend PAD is important to get the edges
	cairo_pattern_t *p = cairo_pattern_create_for_surface(self->cache.surface);
	cairo_pattern_set_extend(p, CAIRO_EXTEND_PAD);
	cairo_pattern_set_filter(p, CAIRO_FILTER_FAST);

	/*
	 * Because of the piecewise nature of this shadow drawing,
	 * under certain transformations (rotations, some scaling)
	 * there will be noticable seams between each edge piece
	 * and between the edges and the center.
	 *
	 * To fix this, the edges and center are drawn one pixel
	 * further than they should be (the +/- pixel in the below
	 * cairo_rectangle calls).
	 *
	 * However, on the edges this causes some noticable overlap
	 * when the next edge is drawn slightly over the previous
	 * one. To fix this, draw to a group using the SOURCE op
	 * so that new pixels replace instead of merge. Finally,
	 * paint that group to the target surface.
	 */

	const float pixel = 1 / scale;
	const float margin = self->radius;

	// Fill in center
	cairo_rectangle(cr,
		margin - pixel,
		margin - pixel,
		self->width - margin * 2 + pixel * 2,
		self->height - margin * 2 + pixel * 2);
	cairo_fill(cr);

	// Draw edges
	cairo_push_group(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	for(int i = 0; i < 4; ++i)
	{
		float length = (i == 0 || i == 2) ? self->width : self->height;
		// Draw one corner + one edge
		cairo_save(cr);
		cairo_translate(cr, -margin, -margin);
		cairo_rectangle(cr,
			0, 0,
			length + pixel, margin * 2);
		cairo_scale(cr, pixel, pixel);
		cairo_set_source(cr, p);
		cairo_fill(cr);
		cairo_restore(cr);

		// Rotate + translate to next edge
		cairo_translate(cr, length, 0);
		cairo_rotate(cr, M_PI / 2);
	}

	cairo_pattern_destroy(p);

	// Use mask instead of paint, so that the color can be set
	p = cairo_pop_group(cr);
	cairo_mask(cr, p);
	cairo_pattern_destroy(p);
}

void cmk_shadow_draw(CmkShadow *self, cairo_t *cr)
{
	cairo_save(cr);

	// Set color
	// TODO: Make user-changeable
	cairo_set_source_rgba(cr, 0, 0, 0, 0.5);

	// If radius is 0, paint the solid shape
	if(self->percent == 0 || self->radius == 0)
	{
		cairo_new_path(cr);

		if(self->type == SHADOW_RECTANGLE)
			cairo_rectangle(cr, 0, 0, self->width, self->height);
		else if(self->type == SHADOW_ANY && self->path)
			cairo_append_path(cr, self->path);

		cairo_fill(cr);
		// match cairo_save at top of this function
		cairo_restore(cr);
		return;
	}

	/*
	 * Drawing a shadow requires bitmap-level work, so in order to
	 * get the proper resolution when working on a scaled device
	 * or with a user transformation (cairo_scale(), etc.), the
	 * actual pixel size of a cairo unit must be determined.
	 *
	 * Note that the x/yscale below don't affect the size of the
	 * rendered shadow, only the resolution.
	 */

	cairo_surface_t *target = cairo_get_target(cr);
	double xdscale, ydscale;
	cairo_surface_get_device_scale(target, &xdscale, &ydscale);

	cairo_matrix_t mat;
	cairo_get_matrix(cr, &mat);
	float xuscale = sqrt(mat.xx * mat.xx + mat.xy * mat.xy);
	float yuscale = sqrt(mat.yx * mat.yx + mat.yy * mat.yy);

	// After about 4x user scale, it doesn't really look any
	// better to keep increasing resolution, it just takes longer
	// to render.
	xuscale = MIN(xuscale, 4);
	yuscale = MIN(yuscale, 4);

	float xscale = xdscale * xuscale;
	float yscale = ydscale * yuscale;

	// Don't try to render anything at 0 scale.
	if(xscale < FLT_EPSILON || yscale < FLT_EPSILON)
		return;

	// There's a stuttering look at small scales, so clamp
	// it to a minimum of the device scale.
	// TODO: Better solution?
	xscale = MAX(xscale, xdscale);
	yscale = MAX(yscale, ydscale);

	// Render based on type of shadow
	if(self->type == SHADOW_RECTANGLE)
	{
		// Optimized rectangle algorithm doesn't work
		// if the radius is larger the half the size
		float r = self->percent * self->radius;
		if(r > self->width / 2 || r > self->height / 2)
			draw_any_shadow(self, cr, xscale, yscale);
		else
			draw_rectangle_shadow(self, cr, xscale, yscale);
	}
	else if(self->type == SHADOW_ANY)
	{
		draw_any_shadow(self, cr, xscale, yscale);
	}
	else
		g_warning("shadow type not implemented");

	cairo_restore(cr);
}

void cmk_shadow_set_percent(CmkShadow *self, float percent)
{
	g_return_if_fail(CMK_IS_SHADOW(self));
	if(self->percent != percent)
		self->percent = percent;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PERCENT]);
}

float cmk_shadow_get_percent(CmkShadow *self)
{
	g_return_val_if_fail(CMK_IS_SHADOW(self), 0);
	return self->percent;
}

void cmk_shadow_set_shape(CmkShadow *self, cairo_path_t *path, bool inner)
{
	g_return_if_fail(CMK_IS_SHADOW(self));

	g_clear_pointer(&self->path, cairo_path_destroy);
	self->path = path;
	self->type = inner ? SHADOW_ANY_INNER : SHADOW_ANY;

	if(path)
	{
		double x2, y2;
		get_path_extents(path, &self->x, &self->y, &x2, &y2);
		self->width = x2 - self->x;
		self->height = y2 - self->y;
	}
	else
	{
		self->x = self->y = self->width = self->height = 0;
	}
}

void cmk_shadow_set_rectangle(CmkShadow *self, float width, float height, bool inner)
{
	g_return_if_fail(CMK_IS_SHADOW(self));

	if(self->width == width
	&& self->height == height
	&& self->type == SHADOW_RECTANGLE)
		return;

	self->type = inner ? SHADOW_RECTANGLE_INNER : SHADOW_RECTANGLE;

	g_clear_pointer(&self->path, cairo_path_destroy);
	self->x = self->y = 0;
	self->width = width;
	self->height = height;
}

// cairo_path_extents() but for a cairo_path_t
// instead of a cairo_t's current path.
static void get_path_extents(cairo_path_t *path, double *ox1, double *oy1, double *ox2, double *oy2)
{
	cairo_path_data_t *data = &path->data[0];
	double x1, x2, y1, y2;
	int i = 0;

	if(path->num_data < 2
	|| data->header.type == CAIRO_PATH_CLOSE_PATH)
	{
		*ox1 = *oy1 = *ox2 = *oy2 = 0;
		return;
	}
	else if(data->header.type != CAIRO_PATH_CLOSE_PATH)
	{
		i += path->data[i].header.length;
	}

	// Start with initial point
	x1 = x2 = data[1].point.x;
	y1 = y2 = data[1].point.y;

	for(; i < path->num_data; )
	{
		data = &path->data[i];
		i += path->data[i].header.length;

		// If the last entry is a move to, don't include it
		if(data->header.type == CAIRO_PATH_MOVE_TO
		&& i > path->num_data)
			break;

		switch(data->header.type)
		{
			case CAIRO_PATH_CURVE_TO:
				x1 = MIN(x1, data[2].point.x);
				x2 = MAX(x2, data[2].point.x);
				y1 = MIN(y1, data[2].point.y);
				y2 = MAX(y2, data[2].point.y);
				x1 = MIN(x1, data[3].point.x);
				x2 = MAX(x2, data[3].point.x);
				y1 = MIN(y1, data[3].point.y);
				y2 = MAX(y2, data[3].point.y);
				// fallthrough
			case CAIRO_PATH_MOVE_TO:
			case CAIRO_PATH_LINE_TO:
				x1 = MIN(x1, data[1].point.x);
				x2 = MAX(x2, data[1].point.x);
				y1 = MIN(y1, data[1].point.y);
				y2 = MAX(y2, data[1].point.y);
			default:
				break;
		}
	}

	*ox1 = x1;
	*oy1 = y1;
	*ox2 = x2;
	*oy2 = y2;
}
