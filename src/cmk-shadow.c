/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "cmk-shadow.h"

struct _CmkShadow
{
	GObject parent;

	bool inner;
	float maxRadius;
	float radius;

	cairo_surface_t *surf;
	unsigned char *tmp;
	bool dirty;

	bool rectangle;
	double x, y, width, height;
	cairo_path_t *path;
};

enum
{
	PROP_INNER = 1,
	PROP_MAX_RADIUS,
	PROP_RADIUS,
	PROP_LAST,
};

static GParamSpec *properties[PROP_LAST];

G_DEFINE_TYPE(CmkShadow, cmk_shadow, G_TYPE_OBJECT);

static void on_dispose(GObject *self_);
static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec);
static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec);


CmkShadow * cmk_shadow_new(float maxRadius, bool inner)
{
	return CMK_SHADOW(g_object_new(CMK_TYPE_SHADOW,
		"max-radius", maxRadius,
		"inner", inner,
		NULL));
}

static void cmk_shadow_class_init(CmkShadowClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;
	base->get_property = get_property;
	base->set_property = set_property;

	/**
	 * CmkWidget:inner:
	 *
	 * Inner shadow or drop shadow.
	 */
	properties[PROP_INNER] =
		g_param_spec_boolean("inner", "inner", "inner",
		                     false,
		                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkWidget:max-radius:
	 *
	 * The maxiumum radius of the shadow, in cairo units.
	 */
	properties[PROP_MAX_RADIUS] =
		g_param_spec_float("max-radius", "max-radius", "max-radius",
		                   0, G_MAXFLOAT, 0,
		                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkWidget:radius:
	 *
	 * Radius of the shadow, as a percentage from 0 to 1 of
	 * the maxiumum radius.
	 */
	properties[PROP_RADIUS] =
		g_param_spec_float("radius", "radius", "radius",
		                   0, 1, 1,
		                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_shadow_init(CmkShadow *self)
{
}

static void on_dispose(GObject *self_)
{
	//TODO
	G_OBJECT_CLASS(cmk_shadow_parent_class)->dispose(self_);
}

static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec)
{
	CmkShadow *self = CMK_SHADOW(self_);
	
	switch(id)
	{
	//case PROP_TEXT:
	//	g_value_set_string(value, cmk_shadow_get_text(self));
	//	break;
	//case PROP_SINGLE_LINE:
	//	g_value_set_boolean(value, PRIV(self)->singleline);
	//	break;
	//case PROP_BOLD:
	//	g_value_set_boolean(value, cmk_shadow_get_bold(self));
	//	break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec)
{
	CmkShadow *self = CMK_SHADOW(self_);

	// TODO invalidate after sets
	
	switch(id)
	{
	case PROP_INNER:
		self->inner = g_value_get_boolean(value);
		break;
	case PROP_MAX_RADIUS:
		self->maxRadius = g_value_get_float(value);
		break;
	case PROP_RADIUS:
		self->radius = g_value_get_float(value);
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
 * The difference is that they can blur a specific region within the
 * buffer, which allows for edge-only blurring.
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



static bool ensure_drop_surface(CmkShadow *self)
{
	int width = self->width + self->maxRadius * 2;
	int height = self->height + self->maxRadius * 2;
	
	if(self->surf
	&& cairo_image_surface_get_width(self->surf) == width
	&& cairo_image_surface_get_height(self->surf) == height)
		return true;
	
	if(self->surf)
		cairo_surface_destroy(self->surf);
	//if(self->tmp)
	//	g_free(self->tmp);

	self->surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		width,
		height);
	//self->tmp = g_new(unsigned char, cairo_image_surface_get_stride(self->surf) * height);
	return false;
}

void slow_draw(CmkShadow *self, const float scale)
{
	if(!self->dirty)
		return;

	const unsigned int margin = ceil(self->maxRadius * scale);
	const float radius = self->maxRadius * scale * self->radius / 2;
	const unsigned int swidth = ceil((self->width + self->maxRadius * 2) * scale);
	const unsigned int sheight = ceil((self->height + self->maxRadius * 2) * scale);
	const unsigned int width = ceil(self->width * scale);
	const unsigned int height = ceil(self->height * scale);
	const unsigned int inv = margin - radius * 2;

	if(!self->surf
	|| cairo_image_surface_get_width(self->surf) != swidth
	|| cairo_image_surface_get_height(self->surf) != sheight)
	{
		g_clear_pointer(&self->surf, cairo_surface_destroy);
		g_clear_pointer(&self->tmp, g_free);
		self->surf = cairo_image_surface_create(CAIRO_FORMAT_A8, swidth, sheight);
	}

	unsigned int stride = cairo_image_surface_get_stride(self->surf);
	unsigned char *data = cairo_image_surface_get_data(self->surf);

	if(!self->tmp)
		self->tmp = g_new(unsigned char, stride * sheight);

	memset(data, 0, stride * sheight);
	fill_rect(data, stride, margin, margin, width, height);

	// Blur everything (very slow)
	//blurH(data, self->tmp, stride, 0, 0, swidth, sheight, radius);
	//blurV(data, self->tmp, stride, 0, 0, swidth, sheight, radius);

	// Blur only the parts actually needing blur (the edges,
	// with only the width of the radius.)
	blurH(data, self->tmp, stride, inv, margin, radius*4, height, radius);
	blurH(data, self->tmp, stride, width + inv, margin, radius*4, height, radius);
	blurV(data, self->tmp, stride, inv, inv, width + radius * 4, radius*4, radius);
	blurV(data, self->tmp, stride, inv, height + inv, width + radius * 4, radius*4, radius);

	cairo_surface_mark_dirty(self->surf);
	self->dirty = false;
}

static void draw_minimal(CmkShadow *self, const float scale)
{
	if(!self->dirty)
		return;
	
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
	 *      only blur  initally fully
	 *      here       opaque, but blured
	 *                 into
	 *
	 * This would then be placed at the top-left corner, and painted
	 * such that the sliver is repeated along the x-axis. This is
	 * then rotated 90 degrees + translated so that it aligns with
	 * the top-right corner with edge along right side y-axis. And
	 * again for the bottom and left sides.
	 *
	 * radius can be anywhere between 0 and maxRadius, depending on
	 * the percentage of blur currently being used (often for animation).
	 */

	const unsigned int margin = ceil(self->maxRadius * scale);
	const unsigned int radius = ceil(self->maxRadius * self->radius * scale);
	const unsigned int swidth = margin * 2 + 1; // +1 for edge sliver
	const unsigned int sheight = margin * 2;
	const unsigned int inv = margin - radius;

	if(!self->surf
	|| cairo_image_surface_get_width(self->surf) != swidth
	|| cairo_image_surface_get_height(self->surf) != sheight)
	{
		g_clear_pointer(&self->surf, cairo_surface_destroy);
		g_clear_pointer(&self->tmp, g_free);
		self->surf = cairo_image_surface_create(CAIRO_FORMAT_A8, swidth, sheight);
	}

	unsigned int stride = cairo_image_surface_get_stride(self->surf);
	unsigned char *data = cairo_image_surface_get_data(self->surf);

	if(!self->tmp)
		self->tmp = g_new(unsigned char, stride * sheight);

	memset(data, 0, stride * sheight);
	fill_rect(data, stride, margin, margin, margin + 1, margin);

	if(radius > 0)
	{
		blurV(data, self->tmp, stride,
			margin, inv,
			margin + 1, radius * 2,
			radius / 2);
		blurH(data, self->tmp, stride,
			inv, inv,
			radius * 2, margin + radius,
			radius / 2);
	}

	cairo_surface_mark_dirty(self->surf);
	self->dirty = false;
}

void cmk_shadow_drawx(CmkShadow *self, cairo_t *cr)
{
	const float scale = 2;
	slow_draw(self, scale);

	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_scale(cr, 1 / scale, 1 / scale);
	cairo_mask_surface(cr, self->surf, -self->maxRadius * scale, -self->maxRadius * scale);
}

void cmk_shadow_draw(CmkShadow *self, cairo_t *cr)
{
	cairo_save(cr);

	// If radius is 0, have a rectangle.
	if(self->radius == 0 || self->maxRadius == 0)
	{
		cairo_rectangle(cr, 0, 0, self->width, self->height);
		cairo_fill(cr);
		cairo_restore(cr);
		return;
	}

	// Increase the resolution of the shadow on scaled devices
	cairo_surface_t *target = cairo_get_target(cr);
	double xdscale, ydscale;
	cairo_surface_get_device_scale(target, &xdscale, &ydscale);
	float dscale = MAX(xdscale, ydscale);

	// Increase the resolution of the shadow for scaled
	// coordinates (e.g. user called cairo_scale()). At some
	// point, it doesn't really look any better to keep making
	// the shadow have higher resolution, it only makes
	// rendering slower. So just cap it at x4 user scale.
	cairo_matrix_t mat;
	cairo_get_matrix(cr, &mat);
	const float uxscale = sqrt(mat.xx * mat.xx + mat.xy * mat.xy);
	const float uyscale = sqrt(mat.yx * mat.yx + mat.yy * mat.yy);
	float uscale = MIN(MAX(uxscale, uyscale), 4);

	if(uscale < FLT_EPSILON)
		return;

	// Also clamp to >= 1 otherwise there's a stuttering
	// look at small scales. TODO: Better solution?
	//uscale = MAX(uscale, 1);

	// scale doesn't affect the size of the shadow, only the resolution
	float scale = fabs(dscale * uscale);
	const float pixel = 1 / scale;
	const float margin = self->maxRadius;

	// If rotation isn't a multiple of 90 degrees, or the
	// scale isn't an integer, it's not pixel-aligned
	const float angle90 = fmod(fabs(atan(mat.yx / mat.yy)), M_PI_2);
	const bool aligned = (angle90 < FLT_EPSILON || fabs(angle90 - M_PI_2) < FLT_EPSILON) && fabs(round(scale) - scale) < FLT_EPSILON;

	// Draw (part of) the shadow to an image surface
	draw_minimal(self, scale);

	cairo_t *crt = cr;
	cairo_surface_t *tmp;

	if(!aligned)
	{
		// If it's not pixel-aligned, redirect drawing
		// to an intermediate image surface. This removes
		// the seams of small alignment errors between
		// the four edge segments and the center. It
		// adds significant overhead, but not as bad as
		// doing real blur on all the edges.
		scale = ceil(scale);
		unsigned int fullw = (self->width + self->maxRadius * 2) * scale;
		unsigned int fullh = (self->height + self->maxRadius * 2) * scale;
		tmp = cairo_image_surface_create(CAIRO_FORMAT_A8, fullw, fullh);
		crt = cairo_create(tmp);
		cairo_scale(crt, scale, scale);
		cairo_translate(crt, margin, margin);
	}

	cairo_pattern_t *p = cairo_pattern_create_for_surface(self->surf);
	cairo_pattern_set_extend(p, CAIRO_EXTEND_PAD);
	cairo_pattern_set_filter(p, CAIRO_FILTER_FAST);

	cairo_set_source_rgba(crt, 0, 0, 0, 1);

	float ao = !aligned ? pixel : 0;
	//g_message("%f", ao);

	// Fill in center
	cairo_rectangle(crt,
		margin - ao,
		margin - ao,
		self->width - margin * 2 + ao * 2,
		self->height - margin * 2 + ao * 2);
	cairo_fill(crt);

	// Draw edges
	for(int i = 0; i < 4; ++i)
	{
		// Draw one corner + one edge
		cairo_save(crt);
		cairo_translate(crt, -margin, -margin);
		cairo_rectangle(crt,
			0, 0,
			self->width, margin * 2);
		cairo_clip(crt);
		cairo_scale(crt, 1 / scale, 1 / scale);
		cairo_mask(crt, p);
		cairo_restore(crt);

		// Rotate + translate to next edge
		cairo_translate(crt, self->width, 0);
		cairo_rotate(crt, M_PI / 2);
	}

	cairo_pattern_destroy(p);

	if(!aligned)
	{
		cairo_translate(cr, -margin, -margin);
		cairo_scale(cr, 1/scale, 1/scale);
		cairo_set_source_surface(cr, tmp, 0, 0);
		cairo_paint(cr);

		cairo_destroy(crt);
		cairo_surface_destroy(tmp);
	}

	cairo_restore(cr);
}

void cmk_shadow_set_radius(CmkShadow *self, float radius)
{
	g_return_if_fail(CMK_IS_SHADOW(self));
	if(self->radius != radius)
	{
		self->radius = radius;
		self->dirty = true;
	}
}

float cmk_shadow_get_radius(CmkShadow *self)
{
	g_return_val_if_fail(CMK_IS_SHADOW(self), 0);
	return self->radius;
}

// Gets the bounding box of the path, and also if the
// path approximately forms a rectangle. (May give
// false for some rectangles, but if it gives true
// it must be a rectangle.)
static void get_path_extents(cairo_path_t *path, double *ox1, double *oy1, double *ox2, double *oy2, bool *outIsRect)
{
	cairo_path_data_t *data = &path->data[0];
	double x1, x2, y1, y2;
	int i = 0;
	int pointi = 0;
	bool isRectangle = true;

	// Capture the first four points, to
	// see if the path is a rectangle
	struct {
		double x, y;
	} points[4];

	*outIsRect = false;

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

		if(data->header.type == CAIRO_PATH_MOVE_TO)
		{
			// If the last entry is a move to, don't include it
			if(i > path->num_data)
				break;

			x1 = MIN(x1, data[1].point.x);
			x2 = MAX(x2, data[1].point.x);
			y1 = MIN(y1, data[1].point.y);
			y2 = MAX(y2, data[1].point.y);
			// Set the first point to the last moveto before
			// any linetos, this way we can ignore if the path
			// ends with a lineto or an implicit "close path".
			// If any movetos occur after the first lineto,
			// assume it's not a rectangle.
			if(isRectangle && pointi == 0)
			{
				points[0].x = data[1].point.x;
				points[0].y = data[1].point.y;
			}
			else
				isRectangle = false;
		}
		else if(data->header.type == CAIRO_PATH_LINE_TO)
		{
			x1 = MIN(x1, data[1].point.x);
			x2 = MAX(x2, data[1].point.x);
			y1 = MIN(y1, data[1].point.y);
			y2 = MAX(y2, data[1].point.y);
			if(isRectangle)
			{
				++pointi;
				points[pointi].x = data[1].point.x;
				points[pointi].y = data[1].point.y;
			}
		}
		else if(data->header.type == CAIRO_PATH_CURVE_TO)
		{
			x1 = MIN(x1, data[1].point.x);
			x2 = MAX(x2, data[1].point.x);
			y1 = MIN(y1, data[1].point.y);
			y2 = MAX(y2, data[1].point.y);
			x1 = MIN(x1, data[2].point.x);
			x2 = MAX(x2, data[2].point.x);
			y1 = MIN(y1, data[2].point.y);
			y2 = MAX(y2, data[2].point.y);
			x1 = MIN(x1, data[3].point.x);
			x2 = MAX(x2, data[3].point.x);
			y1 = MIN(y1, data[3].point.y);
			y2 = MAX(y2, data[3].point.y);
			// technically still could be rectangle but unlikely
			isRectangle = false;
		}
	}

	*ox1 = x1;
	*oy1 = y1;
	*ox2 = x2;
	*oy2 = y2;

	// Detect if (approximately) a rectangle
	if(pointi != 3 || !isRectangle)
		return;

	// Check if all four points are equidistant from the average
	double cx = (points[0].x + points[1].x + points[2].x + points[3].x) / 4;
	double cy = (points[0].y + points[1].y + points[2].y + points[3].y) / 4;
	double d0 = (cx - points[0].x) * (cx - points[0].x)
	          + (cy - points[0].y) * (cy - points[0].y);
	double d1 = (cx - points[1].x) * (cx - points[1].x)
	          + (cy - points[1].y) * (cy - points[1].y);
	double d2 = (cx - points[2].x) * (cx - points[2].x)
	          + (cy - points[2].y) * (cy - points[2].y);
	double d3 = (cx - points[3].x) * (cx - points[3].x)
	          + (cy - points[3].y) * (cy - points[3].y);

	if(abs(d0 - d1) > FLT_EPSILON || abs(d0 - d2) > FLT_EPSILON || abs(d0 - d3) > FLT_EPSILON)
		return;

	// If any two consecutive points don't share a coordinate,
	// then the rectangle isn't axis aligned. Don't need to check
	// all sets of points, since if its not true for 0 and 1,
	// its not true for all due to the equidistant check.
	// This also accounts for four points in a hour-glass shaped
	// configuration.
	if(abs(points[0].x - points[1].x) > FLT_EPSILON
	|| abs(points[0].y - points[1].y) > FLT_EPSILON)
		return;

	*outIsRect = true;
}

void cmk_shadow_set_shape(CmkShadow *self, cairo_path_t *path)
{
	g_return_if_fail(CMK_IS_SHADOW(self));

	self->dirty = true;
	if(self->path)
		cairo_path_destroy(self->path);
	self->path = NULL;

	if(!path)
		return;

	double x2, y2;
	bool rectangle;
	get_path_extents(path, &self->x, &self->y, &x2, &y2, &rectangle);

	self->width = x2 - self->x;
	self->height = y2 - self->y;
	self->rectangle = rectangle;
}

void cmk_shadow_set_rectangle(CmkShadow *self, float width, float height)
{
	g_return_if_fail(CMK_IS_SHADOW(self));

	if(self->width == width && self->height == height)
		return;

	self->dirty = true;
	if(self->path)
		cairo_path_destroy(self->path);
	self->path = NULL;

	self->x = self->y = 0;
	self->width = width;
	self->height = height;
}
