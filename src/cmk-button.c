/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <string.h>
#include "cmk-button.h"
#include "cmk-timeline.h"

#define PRIV(self) ((CmkButtonPrivate *)((self)->priv))

CmkButtonClass *cmk_button_class = NULL;

typedef struct
{
	CmkButtonType type;
	float x;
	CmkTimeline *timeline;
} CmkButtonPrivate;

static void on_destroy(CmkWidget *self_);
static void on_draw(CmkWidget *self_, cairo_t *cr);
static bool on_event(CmkWidget *self_, const CmkEvent *event);
static void get_preferred_width(CmkWidget *self_, float forHeight, float *min, float *nat);
static void get_preferred_height(CmkWidget *self_, float forWidth, float *min, float *nat);
static void get_draw_rect(CmkWidget *self_, CmkRect *rect);


CmkButton * cmk_button_new(CmkButtonType type)
{
	CmkButton *self = calloc(sizeof(CmkButton), 1);
	cmk_button_init(self, type);
	return self;
}

CmkButton * cmk_button_new_with_label(CmkButtonType type, const char *label)
{
	CmkButton *button = cmk_button_new(type);
	cmk_button_set_label(button, label);
	return button;
}

void cmk_button_init(CmkButton *self, CmkButtonType type)
{
	cmk_widget_init((CmkWidget *)self);

	if(cmk_button_class == NULL)
	{
		cmk_button_class = calloc(sizeof(CmkButtonClass), 1);
		CmkWidgetClass *widgetClass = &cmk_button_class->parentClass;
		*widgetClass = *cmk_widget_class;
		widgetClass->destroy = on_destroy;
		widgetClass->draw = on_draw;
		widgetClass->event = on_event;
		widgetClass->get_preferred_width = get_preferred_width;
		widgetClass->get_preferred_height = get_preferred_height;
		widgetClass->get_draw_rect = get_draw_rect;
	}

	((CmkWidget *)self)->class = cmk_button_class;
	self->priv = calloc(sizeof(CmkButtonPrivate), 1);
	PRIV(self)->type = type;

	PRIV(self)->timeline = cmk_timeline_new((CmkWidget *)self, 3000);
	cmk_timeline_set_easing_mode(PRIV(self)->timeline, CMK_TIMELINE_QUAD_IN_OUT);
	cmk_timeline_set_action(PRIV(self)->timeline, (CmkTimelineActionCallback)cmk_widget_invalidate, NULL);
	cmk_timeline_set_multipliers(PRIV(self)->timeline, 1, 1);
	cmk_timeline_set_loop_mode(PRIV(self)->timeline, CMK_TIMELINE_LOOP_REVERSE);
	cmk_widget_set_event_mask((CmkWidget *)self, cmk_widget_get_event_mask((CmkWidget *)self) | CMK_EVENT_MOTION);
}

void cmk_button_set_label(CmkButton *button, const char *label)
{
	// TODO
}

static void on_destroy(CmkWidget *self_)
{
	free(((CmkButton *)self_)->priv);
	cmk_widget_class->destroy(self_);
}

static void on_draw(CmkWidget *self_, cairo_t *cr)
{
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	float w, h;
	cmk_widget_get_size(self_, &w, &h);

	float p = cmk_timeline_get_progress(PRIV((CmkButton *)self_)->timeline);
	//cairo_set_source_rgba(cr, 0, 0, 1, 1);
	//cairo_rectangle(cr, -100, -100, w + 200, h + 200);
	//cairo_fill(cr);

	cairo_set_source_rgb(cr, 1, 0, 0);
	cairo_rectangle(cr, 0, 0, PRIV((CmkButton *)self_)->x, h);
	cairo_fill(cr);

	cairo_set_source_rgba(cr, 0, 1, 1, 0.9);
	cairo_rectangle(cr, 0, 0, p * w, h);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0, 1, 0);
	cairo_rectangle(cr, 0, 0, 20, 20);
	cairo_fill(cr);
}

static bool on_event(CmkWidget *self_, const CmkEvent *event)
{
	if(event->type == CMK_EVENT_MOTION)
	{
		float w, h;
		cmk_widget_get_size(self_, &w, &h);

		CmkEventMotion *motion = (CmkEventMotion *)event;
		int lesser = PRIV((CmkButton *)self_)->x;
		if(motion->x < lesser)
			lesser = motion->x;
		int greater = PRIV((CmkButton *)self_)->x;
		if(motion->x > greater)
			greater = motion->x;
		cairo_rectangle_int_t rect = {lesser, 0, greater - lesser, h};
		rect.width += 1;
		cairo_region_t *reg = cairo_region_create_rectangle(&rect);
		cmk_widget_invalidate(self_, reg);
		cairo_region_destroy(reg);
		PRIV((CmkButton *)self_)->x = motion->x;
		//printf("motion %f, %f\n", motion->x, motion->y);
	}
	else if(event->type == CMK_EVENT_CROSSING)
	{
		CmkEventCrossing *crossing = (CmkEventCrossing *)event;
		CmkTimeline *timeline = PRIV((CmkButton *)self_)->timeline;
		if(crossing->enter)
			cmk_timeline_start(timeline);
		else
			cmk_timeline_start_reverse(timeline);
	}
	return true;
}

static void get_preferred_width(CmkWidget *self_, float forHeight, float *min, float *nat)
{
	*min = *nat = 1000;
}

static void get_preferred_height(CmkWidget *self_, float forWidth, float *min, float *nat)
{
	*min = *nat = 1000;
}

static void get_draw_rect(CmkWidget *self_, CmkRect *rect)
{
	float width, height;
	cmk_widget_get_size(self_, &width, &height);

	rect->x = rect->y = 0;
	rect->width = width;
	rect->height = height;
	//rect->x = -20;
	//rect->y = -20;
	//rect->width = width + 40;
	//rect->height = height + 40;
}

