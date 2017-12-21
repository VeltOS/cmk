#include "cmk-timeline-private.h"

static CmkTimelineCallback gTimelineCallback;

/*
 * Should be used by the controlling code to respond
 * to requests to start or stop a sequence on the timeline.
 */
void cmk_timeline_set_callback(CmkTimelineCallback callback)
{
	gTimelineCallback = callback;
}


/*
 * When a timeline is running, call this on
 * the animation clock / regular intervals.
 */
void cmk_timeline_update(CmkTimeline *timeline)
{
}
