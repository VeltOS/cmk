/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include "cmk-timeline.h"
#include <math.h>

static CmkTimelineHandlerCallback gTimelineHandlerCallback = NULL;
static CmkAddTimeoutHandler gAddTimeoutHandler = NULL;
static CmkRemoveTimeoutHandler gRemoveTimeoutHandler = NULL;

struct CmkTimeline_
{
	CmkWidget *widget;
	uint64_t length; // microseconds
	int refs;

	CmkTimelineActionCallback action;
	void *actiondata;
	void *handlerdata;

	CmkTimelineEasingMode forwardEasingMode;
	CmkTimelineEasingMode backwardEasingMode;
	CmkTimelineEasingMode currentEasingMode;
	CmkTimelineLoopMode loopMode;
	float forwardMultiplier, backwardMultiplier;

	uint64_t lastUpdateTime; // microseconds
	uint64_t progress; // microseconds
	float easedProgress; // normalized [0,1], except some easing modes
	bool playing, reverse;
};


static void update_percent_progress(CmkTimeline *self);


CmkTimeline * cmk_timeline_new(CmkWidget *widget, unsigned long ms)
{
	CmkTimeline *self = calloc(sizeof(CmkTimeline), 1);
	self->widget = widget;
	self->length = ms * 1000;
	self->refs = 1;
	self->forwardEasingMode = self->backwardEasingMode = self->currentEasingMode = CMK_TIMELINE_LINEAR;
	self->forwardMultiplier = self->backwardMultiplier = 1;
	self->loopMode = CMK_TIMELINE_ONESHOT;
	return self;
}

CmkTimeline * cmk_timeline_ref(CmkTimeline *self)
{
	g_return_val_if_fail(self, NULL);

	++self->refs;
	return self;
}

void cmk_timeline_unref(CmkTimeline *self)
{
	g_return_if_fail(self);

	--self->refs;

	if(self->refs <= 0)
	{
		cmk_timeline_pause(self);
		free(self);
	}
}

void cmk_timeline_set_action(CmkTimeline *self, CmkTimelineActionCallback action, void *userdata)
{
	g_return_if_fail(self);

	self->action = action;
	self->actiondata = userdata;
}

void cmk_timeline_set_easing_mode_full(CmkTimeline *self, CmkTimelineEasingMode forward, CmkTimelineEasingMode backward)
{
	g_return_if_fail(self);
	self->forwardEasingMode = forward;
	self->backwardEasingMode = backward;
}

void cmk_timeline_set_easing_mode(CmkTimeline *self, CmkTimelineEasingMode mode)
{
	cmk_timeline_set_easing_mode_full(self, mode, mode);
}

void cmk_timeline_set_multipliers(CmkTimeline *self, float forward, float backward)
{
	g_return_if_fail(self);
	if(forward < 0) forward = 0;
	if(backward < 0) backward = 0;
	self->forwardMultiplier = forward;
	self->backwardMultiplier = backward;
}

void cmk_timeline_set_loop_mode(CmkTimeline *self, CmkTimelineLoopMode mode)
{
	g_return_if_fail(self);
	self->loopMode = mode;
}

static void cmk_timeline_start_internal(CmkTimeline *self)
{
	if(!self->playing)
	{
		// Don't start if its already finished
		if((self->reverse && self->progress == 0)
		|| (!self->reverse && self->progress >= self->length))
			return;

		if(self->reverse && self->progress >= self->length)
			self->currentEasingMode = self->backwardEasingMode;
		else if(!self->reverse && self->progress == 0)
			self->currentEasingMode = self->forwardEasingMode;

		self->handlerdata = gTimelineHandlerCallback(self, true, &self->lastUpdateTime, NULL);
		self->playing = true;
	}
}

void cmk_timeline_start(CmkTimeline *self)
{
	g_return_if_fail(self);
	g_return_if_fail(gTimelineHandlerCallback);

	self->reverse = false;
	cmk_timeline_start_internal(self);
}

void cmk_timeline_start_reverse(CmkTimeline *self)
{
	g_return_if_fail(self);
	g_return_if_fail(gTimelineHandlerCallback);

	self->reverse = true;
	cmk_timeline_start_internal(self);
}

void cmk_timeline_continue(CmkTimeline *self)
{
	g_return_if_fail(self);
	g_return_if_fail(gTimelineHandlerCallback);
	cmk_timeline_start_internal(self);
}

void cmk_timeline_pause(CmkTimeline *self)
{
	g_return_if_fail(self);
	g_return_if_fail(gTimelineHandlerCallback);

	if(self->playing)
	{
		self->playing = false;
		gTimelineHandlerCallback(self, false, NULL, self->handlerdata);
	}
}

void cmk_timeline_goto(CmkTimeline *self, float percent)
{
	g_return_if_fail(self);
	if(percent > 1) percent = 1;
	if(percent < 0) percent = 0;
	self->progress = percent * self->length;
	update_percent_progress(self);
}

bool cmk_timeline_is_playing(CmkTimeline *self)
{
	g_return_val_if_fail(self, false);
	return self->playing;
}

float cmk_timeline_get_progress(CmkTimeline *self)
{
	g_return_val_if_fail(self, 0);
	return self->easedProgress;
}

CmkWidget * cmk_timeline_get_widget(CmkTimeline *self)
{
	g_return_val_if_fail(self, NULL);
	return self->widget;
}

void cmk_timeline_update(CmkTimeline *self, uint64_t time)
{
	if(!self->playing)
		return;
	
	if(self->lastUpdateTime >= time)
		return; // Time went backwards!? or didn't change

	// Change since last update
	uint64_t delta = time - self->lastUpdateTime;
	self->lastUpdateTime = time;

	// Update progress
	if(self->reverse)
	{
		delta *= self->backwardMultiplier;
		if(delta > self->progress)
			self->progress = 0; // Avoid overflows
		else
			self->progress -= delta;
	}
	else
	{
		delta *= self->forwardMultiplier;
		self->progress += delta;
	}

	CmkTimelineActionState state;

	// Check if the end has been reached
	if((self->reverse && self->progress == 0)
	|| (!self->reverse && self->progress >= self->length))
	{
		if(!self->reverse)
			self->progress = self->length;
		update_percent_progress(self);

		if(self->loopMode == CMK_TIMELINE_LOOP)
		{
			// Send out action at endpoint, before reseting
			if(self->action)
				self->action(self->widget, self->actiondata, CMK_TIMELINE_LOOPING);
			self->progress = self->reverse ? self->length : 0;
			return;
		}
		else if(self->loopMode == CMK_TIMELINE_LOOP_REVERSE)
		{
			self->reverse = !self->reverse;
			state = CMK_TIMELINE_LOOPING;

			if(self->reverse)
				self->currentEasingMode = self->backwardEasingMode;
			else
				self->currentEasingMode = self->forwardEasingMode;
		}
		else
		{
			self->playing = false;
			state = CMK_TIMELINE_END;
			gTimelineHandlerCallback(self, false, NULL, self->handlerdata);
		}
	}
	else
	{
		state = CMK_TIMELINE_PLAYING;
		update_percent_progress(self);
	}

	if(self->action)
		self->action(self->widget, self->actiondata, state);
}

void cmk_timeline_set_handler_callback(CmkTimelineHandlerCallback callback, bool overwrite)
{
	if(overwrite || !gTimelineHandlerCallback)
		gTimelineHandlerCallback = callback;
}

CmkTimelineHandlerCallback cmk_timeline_get_handler_callback(void)
{
	return gTimelineHandlerCallback;
}

static void update_percent_progress(CmkTimeline *self)
{
	float p = (float)self->progress / self->length;
	self->easedProgress = cmk_timeline_ease(self->currentEasingMode, p);
}

float cmk_timeline_ease(CmkTimelineEasingMode mode, float p)
{
	/*
	 * Modified from AHEasing
	 * by Auerhaus Development LLC + Warren Moore, 2011
	 * https://github.com/warrenm/AHEasing/blob/master/AHEasing/easing.c
	 */

	switch(mode)
	{
	default:
	case CMK_TIMELINE_LINEAR:
		return p;

	case CMK_TIMELINE_QUAD_IN:
		return p * p;
	case CMK_TIMELINE_QUAD_OUT:
		return -(p * (p - 2));
	case CMK_TIMELINE_QUAD_IN_OUT:
		if(p < 0.5f)
			return 2 * p * p;
		return (-2 * p * p) + (4 * p) - 1;

	case CMK_TIMELINE_CUBIC_IN:
		return p * p * p;
	case CMK_TIMELINE_CUBIC_OUT:
		--p;
		return p * p * p + 1;
	case CMK_TIMELINE_CUBIC_IN_OUT:
		if(p < 0.5f)
			return 4 * p * p * p;
		p = (p * 2) - 2;
		return 0.5f * p * p * p + 1;

	case CMK_TIMELINE_QUART_IN:
		return p * p * p * p;
	case CMK_TIMELINE_QUART_OUT:
		--p;
		return 1 - p * p * p * p;
	case CMK_TIMELINE_QUART_IN_OUT:
		if(p < 0.5f)
			return 8 * p * p * p * p;
		--p;
		return -8 * p * p * p * p + 1;

	case CMK_TIMELINE_QUINT_IN:
		return p * p * p * p * p;
	case CMK_TIMELINE_QUINT_OUT:
		--p;
		return p * p * p * p * p + 1;
	case CMK_TIMELINE_QUINT_IN_OUT:
		if(p < 0.5f)
			return 16 * p * p * p * p * p;
		p = (p * 2) - 2;
		return 0.5f * p * p * p * p * p + 1;

	case CMK_TIMELINE_SINE_IN:
		return sinf((p - 1) * M_PI_2) + 1;
	case CMK_TIMELINE_SINE_OUT:
		return sinf(p * M_PI_2);
	case CMK_TIMELINE_SINE_IN_OUT:
		return 0.5f * (1 - cosf(p * M_PI));

	case CMK_TIMELINE_EXPO_IN:
		return (p == 0) ? p : powf(2, 10 * (p - 1));
	case CMK_TIMELINE_EXPO_OUT:
		return (p == 1) ? p : 1 - powf(2, -10 * p);
	case CMK_TIMELINE_EXPO_IN_OUT:
		if(p == 0 || p == 1)
			return p;
		if(p < 0.5f)
			return 0.5f * powf(2, (20 * p) - 10);
		return -0.5f * powf(2, (-20 * p) + 10) + 1;

	case CMK_TIMELINE_CIRC_IN:
		return 1 - sqrtf(1 - (p * p));
	case CMK_TIMELINE_CIRC_OUT:
		return sqrtf((2 - p) * p);
	case CMK_TIMELINE_CIRC_IN_OUT:
		if(p < 0.5f)
			return 0.5f * (1 - sqrtf(1 - 4 * (p * p)));
		return 0.5f * (sqrtf(-((2 * p) - 3) * ((2 * p) - 1)) + 1);

	case CMK_TIMELINE_ELASTIC_IN:
		return sinf(13 * M_PI_2 * p) * powf(2, 10 * (p - 1));
	case CMK_TIMELINE_ELASTIC_OUT:
		return sinf(-13 * M_PI_2 * (p + 1)) * powf(2, -10 * p) + 1;
	case CMK_TIMELINE_ELASTIC_IN_OUT:
		if(p < 0.5f)
			return 0.5f * sinf(13 * M_PI_2 * (2 * p)) * powf(2, 10 * ((2 * p) - 1));
		return 0.5f * (sinf(-13 * M_PI_2 * ((2 * p - 1) + 1)) * powf(2, -10 * (2 * p - 1)) + 2);

	case CMK_TIMELINE_BACK_IN:
		return p * p * p - p * sinf(p * M_PI);
	case CMK_TIMELINE_BACK_OUT:
		p = 1 - p;
		return 1 - (p * p * p - p * sinf(p * M_PI));
	case CMK_TIMELINE_BACK_IN_OUT:
		if(p < 0.5f)
		{
			p *= 2;
			return 0.5f * (p * p * p - p * sinf(p * M_PI));
		}
		else
		{
			p = (1 - (2 * p - 1));
			return 0.5f * (1 - (p * p * p - p * sinf(p * M_PI))) + 0.5f;
		}

	case CMK_TIMELINE_BOUNCE_IN:
		return 1 - cmk_timeline_ease(CMK_TIMELINE_BOUNCE_OUT, 1 - p);
	case CMK_TIMELINE_BOUNCE_OUT:
		if(p < 4 / 11.0f)
			return (121 * p * p) / 16.0f;
		else if(p < 8 / 11.0f)
			return (363 / 40.0f * p * p) - (99 / 10.0f * p) + 17 / 5.0f;
		else if(p < 9/10.0)
			return (4356 / 361.0f * p * p) - (35442 / 1805.0f * p) + 16061 / 1805.0f;
		return (54 / 5.0f * p * p) - (513 / 25.0f * p) + 268 / 25.0f;
	case CMK_TIMELINE_BOUNCE_IN_OUT:
		if(p < 0.5f)
			return 0.5f * (1 - cmk_timeline_ease(CMK_TIMELINE_BOUNCE_OUT, 1 - (p * 2)));
		return 0.5f * cmk_timeline_ease(CMK_TIMELINE_BOUNCE_OUT, (p * 2) - 1) + 0.5f;
	}
}


void cmk_set_timeout_handlers(CmkAddTimeoutHandler add, CmkRemoveTimeoutHandler remove)
{
	gAddTimeoutHandler = add;
	gRemoveTimeoutHandler = remove;
}

void cmk_get_timeout_handlers(CmkAddTimeoutHandler *add, CmkRemoveTimeoutHandler *remove)
{
	*add = gAddTimeoutHandler;
	*remove = gRemoveTimeoutHandler;
}

unsigned int cmk_add_timeout(unsigned int ms, CmkTimeoutCallback callback, void *userdata)
{
	if(gAddTimeoutHandler)
		return gAddTimeoutHandler(ms, callback, userdata);
	return 0;
}

void cmk_remove_timeout(unsigned int id)
{
	if(gRemoveTimeoutHandler)
		gRemoveTimeoutHandler(id);
}
