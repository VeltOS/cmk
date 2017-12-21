#include <glib-object.h>

typedef struct CmkTimeline_ CmkTimeline;

/*
 * Called when a timeline requests a start or stop.
 * See cmk_timeline_update()
 */
typedef void (*CmkTimelineCallback)(CmkTimeline *timeline, gboolean start);

/*
 * Should be used by the controlling code to respond
 * to requests to start or stop a sequence on the timeline.
 */
void cmk_timeline_set_callback(CmkTimelineCallback callback);


/*
 * When a timeline is running, call this on
 * the animation clock / regular intervals.
 */
void cmk_timeline_update(CmkTimeline *timeline);
