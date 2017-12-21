#include <glib.h>

/**
 * CmkEventType:
 * @CMK_EVENT_BUTTON: Mouse button event.
 * @CMK_EVENT_CROSSING: Mouse enter/leave event.
 * @CMK_EVENT_MOTION: Mouse motion event.
 * @CMK_EVENT_KEY: Keyboard keypress event.
 * @CMK_EVENT_TEXT: Unicode text input event. Use this for inserting user-typed text.
 * @CMK_EVENT_FOCUS: Key focus / unfocus event. Usually caused by tabbing.
 * @CMK_EVENT_SCROLL: Scroll event.
 */
typedef enum
{
	CMK_EVENT_BUTTON = 1,
	CMK_EVENT_CROSSING,
	CMK_EVENT_MOTION,
	CMK_EVENT_KEY,
	CMK_EVENT_TEXT,
	CMK_EVENT_FOCUS,
	CMK_EVENT_SCROLL,
} CmkEventType;

/**
 * CmkKeyModifiers:
 *
 * A set of key modifier masks. These may not be accurate to
 * the hardware keyboard, as the user may have remapped keys.
 *
 * @CMK_MOD_ACCEL: Set if CMK_MOD_CONTROL is set and the
 *     application should consider "control" the main modifier
 *     key for acceleraton shortcuts (Windows, Linux), or
 *     CMK_MOD_META is set and the application should use
 *     "meta"/"command" for shortcuts (macOS).
 */
typedef enum
{
	CMK_MOD_SHIFT     = 1 << 0,
	CMK_MOD_CAPS_LOCK = 1 << 1,
	CMK_MOD_CONTROL   = 1 << 2,
	CMK_MOD_ALT       = 1 << 3,
	CMK_MOD_BUTTON1   = 1 << 4,
	CMK_MOD_BUTTON2   = 1 << 5,
	CMK_MOD_BUTTON3   = 1 << 6,
	CMK_MOD_SUPER     = 1 << 7,
	CMK_MOD_HYPER     = 1 << 8,
	CMK_MOD_META      = 1 << 9,
	CMK_MOD_ACCEL     = 1 << 10,
	CMK_MOD_ALL       = (1 << 11) - 1
} CmkKeyModifiers;

/**
 * CmkEvent:
 *
 * The base event structure. This can be downcasted to from
 * any other event type.
 *
 * @type: #CmkEventType type of event.
 * @time: Time of event, in milliseconds.
 */
typedef struct
{
	CmkEventType type;
	guint32 time;
} CmkEvent;

/**
 * CmkEventButton:
 *
 * Mouse button event data.
 *
 * @type: #CmkEventType type of event.
 * @time: Time of event, in milliseconds.
 * @modifiers: A mask of #CmkKeyModifiers.
 * @x: X coordinate of the mouse, in widget-relative coordinates.
 * @y: Y coordinate of the mouse, in widget-relative coordinates.
 * @press: #TRUE if the button is pressed, #FALSE if the button is
 *          released.
 * @button: The mouse button that was pressed. 1 is primary click,
 *          2 is middle, 3 is secondary, and not well-defined after
 *          that.
 * @clickCount: The number of times the button was clicked in
 *          quick succession. 1 on the first click, 2 on the
 *          second click, etc. Time between clicks may be platform
 *          dependent.
 */
typedef struct
{
	CmkEventType type;
	guint32 time;
	guint modifiers;
	double x;
	double y;
	gboolean press;
	guint button;
	guint clickCount;
} CmkEventButton;

/**
 * CmkEventCrossing:
 *
 * Mouse crossing event data.
 *
 * @type: #CmkEventType type of event.
 * @time: Time of event, in milliseconds.
 * @x: X coordinate of the mouse, in widget-relative coordinates.
 * @y: Y coordinate of the mouse, in widget-relative coordinates.
 * @enter: #TRUE on mouse enter, #FALSE on mouse exit.
 */
typedef struct
{
	CmkEventType type;
	guint32 time;
	double x;
	double y;
	gboolean enter;
} CmkEventCrossing;

/**
 * CmkEventMotion:
 *
 * Mouse motion event data.
 *
 * @type: #CmkEventType type of event.
 * @time: Time of event, in milliseconds.
 * @modifiers: A mask of #CmkKeyModifiers.
 * @x: X coordinate of the mouse, in widget-relative coordinates.
 * @y: Y coordinate of the mouse, in widget-relative coordinates.
 */
typedef struct
{
	CmkEventType type;
	guint32 time;
	guint modifiers;
	double x;
	double y;
} CmkEventMotion;

/**
 * CmkEventScroll:
 *
 * Scroll event data.
 *
 * A value of 1 or -1 is used for dx or dy in the event
 * non-smooth scrolling.
 *
 * @type: #CmkEventType type of event.
 * @time: Time of event, in milliseconds.
 * @modifiers: A mask of #CmkKeyModifiers.
 * @x: X coordinate of the mouse, in widget-relative coordinates.
 * @y: Y coordinate of the mouse, in widget-relative coordinates.
 * @dx: Scroll amount in the X direction.
 * #dy: Scroll amount in the Y direction.
 */
typedef struct
{
	CmkEventType type;
	guint32 time;
	guint modifiers;
	double x;
	double y;
	double dx;
	double dy;
} CmkEventScroll;

/**
 * CmkEventKey:
 *
 * Keyboard event data. This event should not be used for text
 * input. Instead, see CMK_EVENT_TEXT and #CmkEventText.
 *
 * @type: #CmkEventType type of event.
 * @time: Time of event, in milliseconds.
 * @modifiers: A mask of #CmkKeyModifiers.
 * @keyval: The raw key value.
 */
typedef struct
{
	CmkEventType type;
	guint32 time;
	guint modifiers;
	guint keyval;
} CmkEventKey;

/**
 * Text input event data.
 *
 * @type: #CmkEventType type of event.
 * @time: Time of event, in milliseconds.
 * @string: A NULL-terminated UTF-8 string of text input by the user.
 */
typedef struct
{
	CmkEventType type;
	guint32 time;
	gchar *string;
} CmkEventText;

/**
 * Keyboard focus event data.
 *
 * @type: #CmkEventType type of event.
 * @time: Time of event, in milliseconds.
 * @in: #TRUE if there was a gain of keyboard focus, or #FALSE on loss.
 */
typedef struct
{
	CmkEventType type;
	guint32 time;
	gboolean in;
} CmkEventFocus;
