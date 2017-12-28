/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <string.h>
#include "cmk-widget.h"
#include "array.h"

#define PRIV(self) ((CmkWidgetPrivate *)self->priv)
#define CLASS(self) ((CmkWidgetClass *)self->class)

CmkWidgetClass *cmk_widget_class = NULL;

static uint32_t listenIdCount = 1;

typedef struct
{
	uint32_t id;
	char *signalName;
	CmkCallback callback;
	void *userdata;
} Listener;

typedef struct
{
	unsigned int ref;
	void *wrapper;
	Listener *listeners;

	bool disabled;
	bool rtl;
	unsigned int eventMask;
	float setWidth, setHeight;

	// Preferred width / height cache
	float prefWforH, prefWmin, prefWnat;
	float prefHforW, prefHmin, prefHnat;
	bool prefWvalid, prefHvalid;

	// Draw rect cache
	CmkRect drawRect;
	bool drawRectValid;
} CmkWidgetPrivate;

static void on_destroy(CmkWidget *self);
static void on_draw(CmkWidget *self, cairo_t *cr);
static bool on_event(CmkWidget *self, const CmkEvent *event);
static void on_disable(CmkWidget *self, bool disabled);
static void get_preferred_width(CmkWidget *self, float forHeight, float *min, float *nat);
static void get_preferred_height(CmkWidget *self, float forWidth, float *min, float *nat);
static void get_draw_rect(CmkWidget *self, CmkRect *rect);
static void text_direction_changed(CmkWidget *self, bool rtl);
static void free_listener(void *listener);


CmkWidget * cmk_widget_new(void)
{
	CmkWidget *self = calloc(sizeof(CmkWidget), 1);
	cmk_widget_init(self);
	return self;
}

void cmk_widget_init(CmkWidget *self)
{
	if(cmk_widget_class == NULL)
	{
		cmk_widget_class = calloc(sizeof(CmkWidgetClass), 1);
		cmk_widget_class->destroy = on_destroy;
		cmk_widget_class->draw = on_draw;
		cmk_widget_class->disable = on_disable;
		cmk_widget_class->event = on_event;
		cmk_widget_class->get_preferred_width = get_preferred_width;
		cmk_widget_class->get_preferred_height = get_preferred_height;
		cmk_widget_class->get_draw_rect = get_draw_rect;
		cmk_widget_class->text_direction_changed = text_direction_changed;
	}

	self->class = cmk_widget_class;
	self->priv = calloc(sizeof(CmkWidgetPrivate), 1);
	PRIV(self)->ref = 1;
	PRIV(self)->setWidth = PRIV(self)->setHeight = -1;
	PRIV(self)->listeners = array_new(sizeof(Listener), free_listener);

	// Most widgets probably want these events by default.
	PRIV(self)->eventMask = CMK_EVENT_BUTTON | CMK_EVENT_CROSSING | CMK_EVENT_KEY | CMK_EVENT_FOCUS;
}

CmkWidget * cmk_widget_ref(CmkWidget *self)
{
	cmk_return_if_fail(self, NULL)

	++PRIV(self)->ref;
	return self;
}

void cmk_widget_unref(CmkWidget *self)
{
	cmk_return_if_fail(self, )
	
	if(PRIV(self)->ref > 0)
		--PRIV(self)->ref;

	if(PRIV(self)->ref == 0)
	{
		// Destroy object
		if(CLASS(self)->destroy)
			CLASS(self)->destroy(self);
		else
			on_destroy(self);
	}
}

static void on_destroy(CmkWidget *self)
{
	array_free(PRIV(self)->listeners);
	free(self->priv);
	free(self);
}

static void on_draw(UNUSED CmkWidget *self, UNUSED cairo_t *cr)
{
	// Do nothing
}

static bool on_event(UNUSED CmkWidget *self, UNUSED const CmkEvent *event)
{
	return false;
}

static void on_disable(CmkWidget *self, UNUSED bool disabled)
{
	// TODO: Fade the widget out to gray or in to color
	cmk_widget_invalidate(self, NULL);
}

static void get_preferred_width(UNUSED CmkWidget *self, UNUSED float forHeight, float *min, float *nat)
{
	*min = *nat = 0;
}

static void get_preferred_height(UNUSED CmkWidget *self, UNUSED float forWidth, float *min, float *nat)
{
	*min = *nat = 0;
}

static void get_draw_rect(CmkWidget *self, CmkRect *rect)
{
	rect->x = rect->y = 0;
	rect->width = PRIV(self)->setWidth;
	rect->height = PRIV(self)->setHeight;
}

static void text_direction_changed(CmkWidget *self, bool rtl)
{
	cmk_widget_invalidate(self, NULL);
}

void cmk_widget_set_wrapper(CmkWidget *self, void *wrapper)
{
	cmk_return_if_fail(self, )
	PRIV(self)->wrapper = wrapper;
}

void * cmk_widget_get_wrapper(CmkWidget *self)
{
	cmk_return_if_fail(self, NULL)
	return PRIV(self)->wrapper;
}

uint32_t cmk_widget_listen(CmkWidget *self, const char *signalName, CmkCallback callback, void *userdata)
{
	cmk_return_if_fail(self, 0)
	cmk_return_if_fail(signalName, 0)
	cmk_return_if_fail(callback, 0)

	Listener l = {
		.id = (listenIdCount ++),
		.signalName = strdup(signalName),
		.callback = callback,
		.userdata = userdata
	};

	PRIV(self)->listeners = array_append(PRIV(self)->listeners, &l);
	return l.id;
}

void cmk_widget_unlisten(CmkWidget *self, uint32_t id)
{
	cmk_return_if_fail(self, )

	if(id == 0)
		return;

	size_t size = array_size(PRIV(self)->listeners);
	for(size_t i = 0; i < size; ++i)
	{
		if(PRIV(self)->listeners[i].id == id)
		{
			array_remove(PRIV(self)->listeners, i, true);
			break;
		}
	}
}

void cmk_widget_unlisten_by_userdata(CmkWidget *self, const void *userdata)
{
	cmk_return_if_fail(self, )

	size_t size = array_size(PRIV(self)->listeners);
	for(size_t i = size + 1; i != 0; --i)
		if(PRIV(self)->listeners[i - 1].userdata == userdata)
			array_remove(PRIV(self)->listeners, i - 1, true);
}

void cmk_widget_emit(CmkWidget *self, const char *signalName, const void *data)
{
	cmk_return_if_fail(self, )

	size_t size = array_size(PRIV(self)->listeners);
	for(size_t i = 0; i < size; ++i)
	{
		Listener *l = &(PRIV(self)->listeners[i]);
		if(l->callback && strcmp(signalName, l->signalName) == 0)
			l->callback(self, data, l->userdata);
	}
}

void cmk_widget_invalidate(CmkWidget *self, const cairo_region_t *region)
{
	cmk_return_if_fail(self, );
	cmk_widget_emit(self, "invalidate", region);
}

void cmk_widget_relayout(CmkWidget *self)
{
	cmk_return_if_fail(self, );

	PRIV(self)->prefWvalid = PRIV(self)->prefHvalid = false;
	PRIV(self)->drawRectValid = false;

	cmk_widget_emit(self, "relayout", NULL);
}

void cmk_widget_draw(CmkWidget *self, cairo_t *cr)
{
	cmk_return_if_fail(self, );
	cmk_return_if_fail(cr != NULL, );

	if(CLASS(self)->draw)
		CLASS(self)->draw(self, cr);
}

bool cmk_widget_event(CmkWidget *self, const CmkEvent *event)
{
	cmk_return_if_fail(self, false);
	cmk_return_if_fail(event != NULL, false);

	if((PRIV(self)->eventMask & event->type) == 0)
		return false;

	if(CLASS(self)->event)
		return CLASS(self)->event(self, event);
	return false;
}

void cmk_widget_set_event_mask(CmkWidget *self, unsigned int mask)
{
	cmk_return_if_fail(self, );
	PRIV(self)->eventMask = mask;
	cmk_widget_emit(self, "event-mask", NULL);
}

unsigned int cmk_widget_get_event_mask(CmkWidget *self)
{
	cmk_return_if_fail(self, 0);
	return PRIV(self)->eventMask;
}

void cmk_widget_get_preferred_width(CmkWidget *self, float forHeight, float *min, float *nat)
{
	cmk_return_if_fail(self, );

	if(PRIV(self)->prefWvalid && PRIV(self)->prefWforH == forHeight)
	{
		// Return cached value
		if(min) *min = PRIV(self)->prefWmin;
		if(nat) *nat = PRIV(self)->prefWnat;
		return;
	}

	float m = 0, n = 0;
	if(min == NULL) min = &m;
	if(nat == NULL) nat = &n;

	if(CLASS(self)->get_preferred_width)
		CLASS(self)->get_preferred_width(self, forHeight, min, nat);

	PRIV(self)->prefWforH = forHeight;
	PRIV(self)->prefWmin = *min;
	PRIV(self)->prefWnat = *nat;
	PRIV(self)->prefWvalid = true;
}

void cmk_widget_get_preferred_height(CmkWidget *self, float forWidth, float *min, float *nat)
{
	cmk_return_if_fail(self, );

	if(PRIV(self)->prefHvalid && PRIV(self)->prefHforW == forWidth)
	{
		// Return cached value
		if(min) *min = PRIV(self)->prefHmin;
		if(nat) *nat = PRIV(self)->prefHnat;
		return;
	}

	float m = 0, n = 0;
	if(min == NULL) min = &m;
	if(nat == NULL) nat = &n;

	if(CLASS(self)->get_preferred_height)
		CLASS(self)->get_preferred_height(self, forWidth, min, nat);

	PRIV(self)->prefHforW = forWidth;
	PRIV(self)->prefHmin = *min;
	PRIV(self)->prefHnat = *nat;
	PRIV(self)->prefHvalid = true;
}

void cmk_widget_get_draw_rect(CmkWidget *self, CmkRect *rect)
{
	cmk_return_if_fail(rect, );

	rect->x = 0;
	rect->y = 0;
	rect->width = 0;
	rect->height = 0;

	cmk_return_if_fail(self, );

	if(PRIV(self)->drawRectValid)
	{
		*rect = PRIV(self)->drawRect;
		return;
	}

	if(CLASS(self)->get_draw_rect)
		CLASS(self)->get_draw_rect(self, rect);
	
	PRIV(self)->drawRect = *rect;
}

void cmk_widget_set_size(CmkWidget *self, float width, float height)
{
	cmk_return_if_fail(self, );

	if(PRIV(self)->setWidth != width || PRIV(self)->setHeight != height)
		PRIV(self)->drawRectValid = false;

	PRIV(self)->setWidth = width;
	PRIV(self)->setHeight = height;
}

void cmk_widget_get_size(CmkWidget *self, float *width, float *height)
{
	*width = *height = 0;

	cmk_return_if_fail(self, );

	if(PRIV(self)->setWidth < 0)
		cmk_widget_get_preferred_width(self, -1, NULL, width);
	else
		*width = PRIV(self)->setWidth;

	if(PRIV(self)->setHeight < 0)
		cmk_widget_get_preferred_height(self, -1, NULL, height);
	else
		*height = PRIV(self)->setHeight;
}

void cmk_widget_set_disabled(CmkWidget *self, bool disabled)
{
	cmk_return_if_fail(self, );

	if(PRIV(self)->disabled != disabled)
	{
		PRIV(self)->disabled = disabled;

		if(CLASS(self)->disable)
			CLASS(self)->disable(self, disabled);
	}
}

bool cmk_widget_get_disabled(CmkWidget *self)
{
	cmk_return_if_fail(self, false);
	return PRIV(self)->disabled;
}

void cmk_widget_set_text_direction(CmkWidget *self, bool rtl)
{
	cmk_return_if_fail(self, );
	if(PRIV(self)->rtl != rtl)
	{
		PRIV(self)->rtl = rtl;
		if(CLASS(self)->text_direction_changed)
			CLASS(self)->text_direction_changed(self, rtl);
	}
}

bool cmk_widget_get_text_direction(CmkWidget *self)
{
	cmk_return_if_fail(self, false);
	return PRIV(self)->rtl;
}

static void free_listener(void *listener)
{
	cmk_return_if_fail(listener, );
	free(((Listener *)listener)->signalName);
}
