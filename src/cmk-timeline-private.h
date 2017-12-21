#include "cmk-timeline.h"

CmkTimeline * cmk_timeline_new(int ms);
void cmk_timeline_start(CmkTimeline *timeline);
void cmk_timeline_stop(CmkTimeline *timeline);
void cmk_timeline_reset(CmkTimeline *timeline);
float cmk_timeline_get_progress(CmkTimeline *timeline);
