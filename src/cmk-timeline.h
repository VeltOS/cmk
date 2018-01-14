/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-widget.h"

typedef struct CmkTimeline_ CmkTimeline;

/**
 * CmkTimelineMode:
 *
 * Easing modes, which affect the return value from
 * cmk_timeline_get_progress(). See
 * https://developer.gnome.org/clutter/stable/ClutterTimeline.html#ClutterAnimationMode
 * for graphs of the different types of modes.
 */
typedef enum
{
	CMK_TIMELINE_LINEAR,
	CMK_TIMELINE_QUAD_IN,
	CMK_TIMELINE_QUAD_OUT,
	CMK_TIMELINE_QUAD_IN_OUT,
	CMK_TIMELINE_CUBIC_IN,
	CMK_TIMELINE_CUBIC_OUT,
	CMK_TIMELINE_CUBIC_IN_OUT,
	CMK_TIMELINE_QUART_IN,
	CMK_TIMELINE_QUART_OUT,
	CMK_TIMELINE_QUART_IN_OUT,
	CMK_TIMELINE_QUINT_IN,
	CMK_TIMELINE_QUINT_OUT,
	CMK_TIMELINE_QUINT_IN_OUT,
	CMK_TIMELINE_SINE_IN,
	CMK_TIMELINE_SINE_OUT,
	CMK_TIMELINE_SINE_IN_OUT,
	CMK_TIMELINE_EXPO_IN,
	CMK_TIMELINE_EXPO_OUT,
	CMK_TIMELINE_EXPO_IN_OUT,
	CMK_TIMELINE_CIRC_IN,
	CMK_TIMELINE_CIRC_OUT,
	CMK_TIMELINE_CIRC_IN_OUT,
	CMK_TIMELINE_ELASTIC_IN,
	CMK_TIMELINE_ELASTIC_OUT,
	CMK_TIMELINE_ELASTIC_IN_OUT,
	CMK_TIMELINE_BACK_IN,
	CMK_TIMELINE_BACK_OUT,
	CMK_TIMELINE_BACK_IN_OUT,
	CMK_TIMELINE_BOUNCE_IN,
	CMK_TIMELINE_BOUNCE_OUT,
	CMK_TIMELINE_BOUNCE_IN_OUT
} CmkTimelineEasingMode;

/**
 * CmkTimelineLoopMode:
 *
 * @CMK_TIMELINE_ONESHOT: The timeline plays once, and when it
 *      finishes it stops.
 * @CMK_TIMELINE_LOOP: The timeline will repeat when it finishes,
 *      returning to the opposite end of the timeline and playing
 *      like a gif.
 * @CMK_TIMELINE_LOOP_REVERSE: When the timeline reaches either
 *      end, it will reverse direction and continue playing.
 */
typedef enum
{
	CMK_TIMELINE_ONESHOT,
	CMK_TIMELINE_LOOP,
	CMK_TIMELINE_LOOP_REVERSE,
} CmkTimelineLoopMode;

/**
 * CmkTimelineActionState:
 *
 * @CMK_TIMELINE_PLAYING: The timeline is playing.
 * @CMK_TIMELINE_END: The timeline completed and this
 *      is the last action it will emit until manually
 *      played again.
 * @CMK_TIMELINE_LOOPING: The timeline reached the end,
 *      but it has been set to loop, so it is still
 *      playing.
 */
typedef enum
{
	CMK_TIMELINE_PLAYING,
	CMK_TIMELINE_END,
	CMK_TIMELINE_LOOPING,
} CmkTimelineActionState;

/**
 * CmkTimelineActionCallback:
 *
 * See cmk_timeline_set_action().
 */
typedef void (*CmkTimelineActionCallback)(CmkWidget *widget, void *data, CmkTimelineActionState state);

/**
 * CmkTimelineCallback:
 *
 * See cmk_timeline_set_handler_callback().
 *
 * @start: To start or stop the update callbacks.
 * @time: Output the current timestamp, in microseconds.
 * @userdata: On start, NULL; on stop, the value returned by the call to start
 * Returns: Some userdata to store.
 */
typedef void * (*CmkTimelineHandlerCallback)(CmkTimeline *timeline, bool start, uint64_t *time, void *userdata);

/**
 * CmkTimeoutCallback:
 *
 * Callback for cmk_timeout_add().
 */
typedef bool (*CmkTimeoutCallback)(void *userdata);

typedef unsigned int (*CmkAddTimeoutHandler)(unsigned int ms, CmkTimeoutCallback callback, void *userdata);
typedef void (*CmkRemoveTimeoutHandler)(unsigned int id);


/**
 * cmk_timeline_new:
 *
 * Creates a new timeline, which allows
 * a CmkWidget to perform animations.
 *
 * @widget: The widget to run the timeline on. Each update,
 *          the CmkTimeline will invalidate the CmkWidget.
 * @ms: The duration of the timeline (animation), in milliseconds.
 */
CmkTimeline * cmk_timeline_new(CmkWidget *widget, unsigned long ms);

/**
 * cmk_timeline_ref:
 *
 * References the #CmkTimeline.
 */
CmkTimeline * cmk_timeline_ref(CmkTimeline *timeline);

/**
 * cmk_timeline_unref:
 *
 * Unref the #CmkTimeline.
 */
void cmk_timeline_unref(CmkTimeline *timeline);

/**
 * cmk_timeline_set_action:
 *
 * Sets the action to be performed when the timeline updates.
 * It is usually used in this way:
 *
 * cmk_timeline_set_action(timeline, cmk_widget_invalidate, regionToInvalidate);
 */ 
void cmk_timeline_set_action(CmkTimeline *timeline, CmkTimelineActionCallback action, void *userdata);

/**
 * cmk_timeline_set_easing_mode_full:
 *
 * See #CmkTimelineEasingMode enumeration.
 *
 * @forward and @backward affect the mode to use when starting
 * at the beginning or end of the timeline, but if the timeline
 * is started after being paused part way through, it will
 * continue using the easing mode it was using.
 *
 * @forward: Sets the easing mode to use when starting
 *     the timeline at the beginning and going forward.
 * @backward: Sets the easing mode to use when starting
 *     the timeline at the end and going backward.
 */
void cmk_timeline_set_easing_mode_full(CmkTimeline *timeline, CmkTimelineEasingMode forward, CmkTimelineEasingMode backward);

/**
 * cmk_timeline_set_easing_mode:
 *
 * cmk_timeline_set_easing_mode_full() with @mode as both
 * forward and backward.
 */
void cmk_timeline_set_easing_mode(CmkTimeline *timeline, CmkTimelineEasingMode mode);

/**
 * cmk_timeline_set_multipliers:
 *
 * It can be useful to have the one direction of the timeline
 * play faster than the other. The @forward and @backward values
 * scale the speed at which each direction plays.
 */
void cmk_timeline_set_multipliers(CmkTimeline *timeline, float forward, float backward);

/**
 * cmk_timeline_set_loop_mode:
 *
 * Sets how the timeline should or should not loop. In this mode,
 * the timeline never completes, but it will always send an action
 * at exactly 1 and 0 progress when it hits the end or beginning,
 * respectively.
 */
void cmk_timeline_set_loop_mode(CmkTimeline *timeline, CmkTimelineLoopMode mode);

/**
 * cmk_timeline_start:
 *
 * Starts or continues a timeline in the forward direction.
 * The timeline will play until it completes and then stop.
 * If the timeline is already completed, this does nothing;
 * the timeline must be reversed or reset first.
 *
 * See cmk_timeline_start_reverse().
 */
void cmk_timeline_start(CmkTimeline *timeline);

/**
 * cmk_timeline_start_reverse:
 *
 * Starts or continues a timeline in the reverse direction.
 *
 * Together with cmk_timeline_start(), this can be used
 * to play animations that responed to user events such as
 * changing shadow size on mouse-over.
 */
void cmk_timeline_start_reverse(CmkTimeline *timeline);

/**
 * cmk_timeline_pause:
 *
 * Pause a timeline. Continue with cmk_timeline_continue().
 */
void cmk_timeline_pause(CmkTimeline *timeline);

/**
 * cmk_timeline_continue:
 *
 * Starts a timeline without forcing a direction;
 * opposite of cmk_timeline_pause().
 */
void cmk_timeline_continue(CmkTimeline *timeline);

/**
 * cmk_timeline_goto:
 *
 * Go to a position on the timeline, from 0 to 1 (0 is
 * start, 1 is end). This does not directly set the progress
 * of the timeline, which depends on the easing mode.
 */
void cmk_timeline_goto(CmkTimeline *timeline, float percent);

/**
 * cmk_timeline_is_playing:
 *
 * Returns true if the timeline is playing.
 */
bool cmk_timeline_is_playing(CmkTimeline *timeline);

/**
 * cmk_timeline_get_progress:
 *
 * Gets the progress as a percentage from 0 to 1.
 */
float cmk_timeline_get_progress(CmkTimeline *timeline);

/**
 * cmk_timeline_get_widget:
 *
 * Gets the #CmkWidget associated with the timeline.
 */
CmkWidget * cmk_timeline_get_widget(CmkTimeline *timeline);

/**
 * cmk_timeline_update:
 *
 * When a timeline is running, call this on
 * the animation clock / regular intervals.
 *
 * @time: Current time in microseconds.
 */
void cmk_timeline_update(CmkTimeline *timeline, uint64_t time);

/**
 * cmk_timeline_set_callback:
 *
 * Global value. Should be used by the controlling code to
 * respond to requests to start or stop a sequence on the
 * timeline. See cmk_timeline_update().
 *
 * This is automatically called by wrapper classes upon
 * their initialization (with overwrite set to false), so
 * normally don't worry about this function.
 *
 * @overwrite: true to overwrite an existing callback, if set
 */
void cmk_timeline_set_handler_callback(CmkTimelineHandlerCallback callback, bool overwrite);

/**
 * cmk_timeline_get_handler_callback:
 *
 * Gets the global value set in cmk_timeline_set_handler_callback().
 * Can be used to implement a custom version of CmkTimeline.
 */
CmkTimelineHandlerCallback cmk_timeline_get_handler_callback(void);

/**
 * cmk_timeline_ease:
 *
 * Takes a progress from 0 to 1 and applys the easing mode to it.
 * This is not needed for standard #CmkTimeline usage, but may be
 * useful for getting multiple easing modes out of a one timeline.
 */
float cmk_timeline_ease(CmkTimelineEasingMode mode, float progress);

void cmk_set_timeout_handlers(CmkAddTimeoutHandler add, CmkRemoveTimeoutHandler remove);

void cmk_get_timeout_handlers(CmkAddTimeoutHandler *add, CmkRemoveTimeoutHandler *remove);

/**
 * cmk_add_timeout
 *
 * Invokes wrapper's timeout method.
 */
unsigned int cmk_add_timeout(unsigned int ms, CmkTimeoutCallback callback, void *userdata);

void cmk_remove_timeout(unsigned int id);
