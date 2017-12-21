#include "cmk-widget.h"

typedef struct _CmkWidgetPrivate CmkWidgetPrivate;
struct _CmkWidgetPrivate
{
	void *wrapper;

	// Preferred width / height cache
	float prefWforH, prefWmin, prefWnat;
	float prefHforW, prefHmin, prefHnat;
	gboolean prefWvalid, prefHvalid;

	float setWidth, setHeight;

	gboolean disabled;
};

enum
{
	PROP_DISABLED = 1,
	PROP_LAST
};

enum
{
	SIGNAL_INVALIDATE = 1,
	SIGNAL_RELAYOUT,
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

static void set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void on_dispose(GObject *self_);
static void on_invalidate(CmkWidget *self, const cairo_region_t *region);
static void on_relayout(CmkWidget *self);
static void on_draw(CmkWidget *self, cairo_t *cr);
static gboolean on_event(CmkWidget *self, const CmkEvent *event);
static void on_disable(CmkWidget *self, gboolean disabled);
static void get_preferred_width(CmkWidget *self, float forHeight, float *min, float *nat);
static void get_preferred_height(CmkWidget *self, float forWidth, float *min, float *nat);
static void set_disabled_internal(CmkWidget *self, gboolean disabled);


G_DEFINE_TYPE_WITH_PRIVATE(CmkWidget, cmk_widget, G_TYPE_INITIALLY_UNOWNED);
#define PRIVATE(widget) ((CmkWidgetPrivate *)cmk_widget_get_instance_private(widget))


CmkWidget * cmk_widget_new(void)
{
	return CMK_WIDGET(g_object_new(CMK_TYPE_WIDGET, NULL));
}

static void cmk_widget_class_init(CmkWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;
	base->get_property = get_property;
	base->set_property = set_property;

	class->invalidate = on_invalidate;
	class->relayout = on_relayout;
	class->draw = on_draw;
	class->event = on_event;
	class->disable = on_disable;
	class->get_preferred_width = get_preferred_width;
	class->get_preferred_height = get_preferred_height;

	properties[PROP_DISABLED] =
		g_param_spec_boolean("disabled",
		                     "Disabled",
		                     "TRUE if this widget is disabled",
		                     FALSE,
		                     G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);

	signals[SIGNAL_INVALIDATE] =
		g_signal_new("invalidate",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             G_STRUCT_OFFSET(CmkWidgetClass, invalidate),
		             NULL, NULL, NULL,
		             G_TYPE_NONE,
		             4, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT);

	signals[SIGNAL_RELAYOUT] =
		g_signal_new("relayout",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             G_STRUCT_OFFSET(CmkWidgetClass, relayout),
		             NULL, NULL, NULL,
		             G_TYPE_NONE,
		             0);
}

static void cmk_widget_init(CmkWidget *self)
{
	CmkWidgetPrivate *private = PRIVATE(self);
	private->setWidth = private->setHeight = -1;

	//private->colors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)cmk_color_free);
}

static void on_dispose(GObject *self_)
{
	//CmkWidgetPrivate *private = PRIVATE(CMK_WIDGET(self_));
	G_OBJECT_CLASS(cmk_widget_parent_class)->dispose(self_);
}

static void set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_WIDGET(self_));
	CmkWidget *self = CMK_WIDGET(self_);
	
	switch(propertyId)
	{
	case PROP_DISABLED:
		set_disabled_internal(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_WIDGET(self_));
	CmkWidget *self = CMK_WIDGET(self_);
	
	switch(propertyId)
	{
	case PROP_DISABLED:
		g_value_set_boolean(value, PRIVATE(self)->disabled);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void on_invalidate(UNUSED CmkWidget *self, UNUSED const cairo_region_t *region)
{
	// Do nothing
}

static void on_relayout(CmkWidget *self)
{
	CmkWidgetPrivate *private = PRIVATE(self);
	private->prefWvalid = private->prefHvalid = FALSE;
}

static void on_draw(UNUSED CmkWidget *self, UNUSED cairo_t *cr)
{
	float w, h;
	cmk_widget_get_size(self, &w, &h);
	// Do nothing
	g_message("draw");
	cairo_set_source_rgb(cr, 1, 0, 0);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_fill(cr);
}

static gboolean on_event(UNUSED CmkWidget *self, UNUSED const CmkEvent *event)
{
	g_message("event %i", event->type);
	return FALSE;
}

static void on_disable(CmkWidget *self, UNUSED gboolean disabled)
{
	// TODO: Fade the widget out to gray or in to color
	cmk_widget_invalidate(self, NULL);
}

static void get_preferred_width(UNUSED CmkWidget *self, UNUSED float forHeight, float *min, float *nat)
{
	g_message("get pref width");
	*min = *nat = 100;
}

static void get_preferred_height(UNUSED CmkWidget *self, UNUSED float forWidth, float *min, float *nat)
{
	*min = *nat = 100;
}

void cmk_widget_set_wrapper(CmkWidget *self, void *wrapper)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(PRIVATE(self)->wrapper == NULL);
	PRIVATE(self)->wrapper = wrapper;
}

void * cmk_widget_get_wrapper(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->wrapper;
}

void cmk_widget_invalidate(CmkWidget *self, const cairo_region_t *region)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	g_signal_emit(self, signals[SIGNAL_INVALIDATE], 0, region);
}

void cmk_widget_relayout(CmkWidget *self)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_signal_emit(self, signals[SIGNAL_RELAYOUT], 0);
}

void cmk_widget_draw(CmkWidget *self, cairo_t *cr)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(cr != NULL);

	CmkWidgetClass *class = CMK_WIDGET_GET_CLASS(self);
	if(G_LIKELY(class->draw))
		class->draw(self, cr);
}

gboolean cmk_widget_event(CmkWidget *self, const CmkEvent *event)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	CmkWidgetClass *class = CMK_WIDGET_GET_CLASS(self);
	if(G_LIKELY(class->event))
		return class->event(self, event);
	return FALSE;
}

void cmk_widget_get_preferred_width(CmkWidget *self, float forHeight, float *min, float *nat)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	CmkWidgetPrivate *private = PRIVATE(self);
	if(private->prefWvalid && private->prefWforH == forHeight)
	{
		// Return cached value
		if(min) *min = private->prefWmin;
		if(nat) *nat = private->prefWnat;
		return;
	}

	float m = 0, n = 0;
	if(min == NULL) min = &m;
	if(nat == NULL) nat = &n;

	CmkWidgetClass *class = CMK_WIDGET_GET_CLASS(self);
	if(G_LIKELY(class->get_preferred_width))
		class->get_preferred_width(self, forHeight, min, nat);

	private->prefWforH = forHeight;
	private->prefWmin = *min;
	private->prefWnat = *nat;
	private->prefWvalid = TRUE;
}

void cmk_widget_get_preferred_height(CmkWidget *self, float forWidth, float *min, float *nat)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	CmkWidgetPrivate *private = PRIVATE(self);
	if(private->prefHvalid && private->prefHforW == forWidth)
	{
		// Return cached value
		if(min) *min = private->prefHmin;
		if(nat) *nat = private->prefHnat;
		return;
	}

	float m = 0, n = 0;
	if(min == NULL) min = &m;
	if(nat == NULL) nat = &n;

	CmkWidgetClass *class = CMK_WIDGET_GET_CLASS(self);
	if(G_LIKELY(class->get_preferred_height))
		class->get_preferred_height(self, forWidth, min, nat);

	private->prefHforW = forWidth;
	private->prefHmin = *min;
	private->prefHnat = *nat;
	private->prefHvalid = TRUE;
}

void cmk_widget_set_size(CmkWidget *self, float width, float height)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	CmkWidgetPrivate *private = PRIVATE(self);
	private->setWidth = width;
	private->setHeight = height;
}

void cmk_widget_get_size(CmkWidget *self, float *width, float *height)
{
	*width = *height = 0;

	g_return_if_fail(CMK_IS_WIDGET(self));
	CmkWidgetPrivate *private = PRIVATE(self);

	if(private->setWidth < 0)
		cmk_widget_get_preferred_width(self, -1, NULL, width);
	else
		*width = private->setWidth;

	if(private->setHeight < 0)
		cmk_widget_get_preferred_height(self, -1, NULL, height);
	else
		*height = private->setHeight;
}

static void set_disabled_internal(CmkWidget *self, gboolean disabled)
{
	CmkWidgetPrivate *private = PRIVATE(self);
	if(private->disabled != disabled)
	{
		private->disabled = disabled;

		CmkWidgetClass *class = CMK_WIDGET_GET_CLASS(self);
		if(G_LIKELY(class->disable))
			class->disable(self, disabled);
	}
}

void cmk_widget_set_disabled(CmkWidget *self, gboolean disabled)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	gboolean prev = PRIVATE(self)->disabled;

	set_disabled_internal(self, disabled);

	if(prev != disabled)
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DISABLED]);
}

gboolean cmk_widget_get_disabled(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), FALSE);
	return PRIVATE(self)->disabled;
}
