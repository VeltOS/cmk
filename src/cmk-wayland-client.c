/*
 * libcmk
 * Copyright (C) 2017 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef CMK_NO_WAYLAND

#include <glib-object.h>
#include <wayland-client.h>
#include <clutter/wayland/clutter-wayland.h>
#include "cmk-widget.h"

static void cwl_registry_on_add(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void cwl_registry_on_remove(void *data, struct wl_registry *registry, uint32_t id);

static void cwl_output_release(struct wl_output *output);
static void cwl_output_on_geometry(void *data, struct wl_output *output, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel, const char *make, const char *model, int32_t transform);
static void cwl_output_on_mode(void *data, struct wl_output *output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
static void cwl_output_on_done(void *data, struct wl_output *output);
static void cwl_output_on_scale(void *data, struct wl_output *output, int32_t scale);

static void cwl_seat_release(struct wl_seat *seat);
static void cwl_seat_on_update(void *data, struct wl_seat *seat, uint32_t capabilities);

static void cwl_pointer_on_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y);
static void cwl_pointer_on_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);
static void cwl_pointer_on_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
static void cwl_pointer_on_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
static void cwl_pointer_on_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value);

static void cwl_keyboard_on_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size); 
static void cwl_keyboard_on_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
static void cwl_keyboard_on_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface);
static void cwl_keyboard_on_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
static void cwl_keyboard_on_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched, uint32_t modsLocked, uint32_t group);

static void cwl_surface_release(struct wl_surface *surface);

typedef struct
{
	int32_t scale;
	// There are other properties but they're not important to us
} CwlOutputProperties;

struct _ClutterInputDeviceClass
{
	GObjectClass gobjectClass;
	gboolean (* keycode_to_evdev) (ClutterInputDevice *device,
		guint hardware_keycode,
		guint *evdev_keycode);
};

#define CWL_TYPE_INPUT_DEVICE cwl_input_device_get_type()
G_DECLARE_FINAL_TYPE(CwlInputDevice, cwl_input_device, CWL, INPUT_DEVICE, ClutterInputDevice);

struct _CwlInputDevice
{
	GObject gobjectParent;
	// ClutterInputDevice is somewhere around 200 bytes excluding
	// its GObject parent. So give some extra room for expansion.
	char parent[400]; 
	
	struct wl_pointer *pointer;
	struct wl_surface *pointerSurface;
	gfloat x, y;
	
	struct wl_keyboard *keyboard;
	struct wl_surface *keyboardSurface;
	
	guint modifiers;
};

typedef struct
{
	ClutterStage *stage;
	int32_t scale;
} CwlSurfaceProperties;


static const struct wl_registry_listener cwl_registry_cbs = { // Version 1
	cwl_registry_on_add,
	cwl_registry_on_remove
};
const struct wl_output_listener cwl_output_cbs = { // Version 2
	cwl_output_on_geometry,
	cwl_output_on_mode,
	cwl_output_on_done, // since 2
	cwl_output_on_scale // since 2
};
const struct wl_seat_listener cwl_seat_cbs = { // Version 1
	cwl_seat_on_update
};
const struct wl_pointer_listener cwl_pointer_cbs = { // Version 1
	cwl_pointer_on_enter,
	cwl_pointer_on_leave,
	cwl_pointer_on_motion,
	cwl_pointer_on_button,
	cwl_pointer_on_axis,
	// TODO: Version 5 has a 'frame' event that allows combining multiple
	// pointer events into one if they happened simulateously (ex: diagonal
	// scrolling on a touch pad). Not necessary, but could be useful.
};
const struct wl_keyboard_listener cwl_keyboard_cbs = { // Version 1
	cwl_keyboard_on_keymap,
	cwl_keyboard_on_enter,
	cwl_keyboard_on_leave,
	cwl_keyboard_on_key,
	cwl_keyboard_on_modifiers,
};

G_DEFINE_TYPE(CwlInputDevice, cwl_input_device, CLUTTER_TYPE_INPUT_DEVICE);

static struct wl_display *cwlDisplay = NULL;
static struct wl_registry *cwlRegistry = NULL;
static struct wl_compositor *cwlCompositor = NULL;
static uint32_t cwlCompositorId = 0;
static struct wl_shell *cwlShell = NULL;
static uint32_t cwlShellId = 0;
static struct wl_shm *cwlShm = NULL;
static uint32_t cwlShmId = 0;
static struct wl_cursor_theme *cwlCursorTheme = NULL;
static GHashTable *cwlSeats = NULL; // id to struct wl_seat *
static GHashTable *cwlOutputs = NULL; // id to struct wl_output *
static GPtrArray *cwlSurfaces = NULL; // array of struct wl_surface * for windows

gboolean cwl_client_init(void)
{
	cwlDisplay = wl_display_connect(NULL);
	if(!cwlDisplay)
		return FALSE;
	
	// Destroy the proxy objects when the entry from the table is removed
	cwlSeats = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)cwl_seat_release);
	cwlOutputs = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)cwl_output_release);
	cwlSurfaces = g_ptr_array_new_with_free_func((GDestroyNotify)cwl_surface_release);
	
	cwlRegistry = wl_display_get_registry(cwlDisplay);
	wl_registry_add_listener(cwlRegistry, &cwl_registry_cbs, NULL);
	
	wl_display_dispatch(cwlDisplay);
	wl_display_roundtrip(cwlDisplay);
	
	// try again
	if(!cwlCompositor || !cwlShell)
		wl_display_roundtrip(cwlDisplay);
	
	if(!cwlCompositor || !cwlShell)
	{
		g_warning("Cannot find wl_compositor or wl_shell");
		wl_display_disconnect(cwlDisplay);
		g_clear_pointer(&cwlSeats, g_hash_table_destroy);
		g_clear_pointer(&cwlOutputs, g_hash_table_destroy);
		g_clear_pointer(&cwlSurfaces, g_ptr_array_unref);
		return FALSE;
	}
	
	clutter_set_windowing_backend("wayland");
	clutter_wayland_set_display(cwlDisplay);
	//clutter_wayland_disable_event_retrieval();
	
	if(clutter_init(NULL, NULL) != CLUTTER_INIT_SUCCESS)
	{
		wl_display_disconnect(cwlDisplay);
		g_clear_pointer(&cwlSeats, g_hash_table_destroy);
		g_clear_pointer(&cwlOutputs, g_hash_table_destroy);
		g_clear_pointer(&cwlSurfaces, g_ptr_array_unref);
		return FALSE;
	}
	
	return TRUE;
}

void cwl_client_deinit(void)
{
	g_clear_pointer(&cwlRegistry, wl_registry_destroy);
	g_clear_pointer(&cwlCompositor, wl_compositor_destroy);
	g_clear_pointer(&cwlShell, wl_shell_destroy);
	g_clear_pointer(&cwlShm, wl_shm_destroy);
	g_clear_pointer(&cwlSeats, g_hash_table_destroy);
	g_clear_pointer(&cwlOutputs, g_hash_table_destroy);
	g_clear_pointer(&cwlDisplay, wl_display_disconnect);
	g_clear_pointer(&cwlSurfaces, g_ptr_array_unref);
	cwlCompositorId = 0;
	cwlShellId = 0;
	cwlShmId = 0;
}

static void cwl_registry_on_add(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	g_message("registry add: %i, %i, %s", id, version, interface);
	if(g_strcmp0(interface, "wl_compositor") == 0)
	{
		g_clear_pointer(&cwlCompositor, wl_compositor_destroy);
		cwlCompositorId = id;
		cwlCompositor = wl_registry_bind(registry, id, &wl_compositor_interface, 3);
	}
	else if(g_strcmp0(interface, "wl_shell") == 0)
	{
		g_clear_pointer(&cwlShell, wl_shell_destroy);
		cwlShellId = id;
		cwlShell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
	}
	else if(g_strcmp0(interface, "wl_shm") == 0)
	{
		g_clear_pointer(&cwlShm, wl_shm_destroy);
		cwlShmId = id;
		cwlShm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
	}
	else if(g_strcmp0(interface, "wl_seat") == 0)
	{
		struct wl_seat *seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
		
		CwlInputDevice *device = CWL_INPUT_DEVICE(
			g_object_new(cwl_input_device_get_type(),
				"id", id,
				"device-type", CLUTTER_POINTER_DEVICE,
				"name", "wayland device",
				"enabled", TRUE,
				NULL));

		wl_seat_set_user_data(seat, device);
		
		g_hash_table_insert(cwlSeats, GINT_TO_POINTER(id), device);
		wl_seat_add_listener(seat, &cwl_seat_cbs, device);
	}
	else if(g_strcmp0(interface, "wl_output") == 0)
	{
		struct wl_output *output = wl_registry_bind(registry, id, &wl_output_interface, 2);
		
		CwlOutputProperties *props = g_new0(CwlOutputProperties, 1);
		props->scale = 1;
		wl_output_set_user_data(output, props);
		
		g_hash_table_insert(cwlOutputs, GINT_TO_POINTER(id), output);
		wl_output_add_listener(output, &cwl_output_cbs, props);
	}
}

static void cwl_registry_on_remove(void *data, struct wl_registry *registry, uint32_t id)
{
	g_message("registry remove: %i", id);
	// Try to find the proxy that was removed and dispose of it
	
	if(id == cwlCompositorId)
	{
		g_clear_pointer(&cwlCompositor, wl_compositor_destroy);
		cwlCompositorId = 0;
	}
	else if(id == cwlShellId)
	{
		g_clear_pointer(&cwlShell, wl_shell_destroy);
		cwlShellId = 0;
	}
	else if(id == cwlShmId)
	{
		g_clear_pointer(&cwlShm, wl_shm_destroy);
		cwlShmId = 0;
	}
	// Removing from table will release the proxy (g_hash_table_remove returns TRUE if it was found a removed)
	else if(cwlOutputs && g_hash_table_remove(cwlOutputs, GINT_TO_POINTER(id)))
	{ }
	else if(cwlSeats && g_hash_table_remove(cwlSeats, GINT_TO_POINTER(id)))
	{ }
}


static void cwl_output_release(struct wl_output *output)
{
	CwlOutputProperties *props = wl_output_get_user_data(output);
	g_free(props);
	wl_output_release(output);
}

static void cwl_output_on_geometry(void *data, struct wl_output *output, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel, const char *make, const char *model, int32_t transform)
{ } // Not important

static void cwl_output_on_mode(void *data, struct wl_output *output, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{ } // Not important

static void cwl_output_on_done(void *data, struct wl_output *output)
{ } // Not important

static void cwl_output_on_scale(void *data, struct wl_output *output, int32_t scale)
{
	CwlOutputProperties *props = wl_output_get_user_data(output);
	if(!props)
		return;
	props->scale = scale;
	// TODO: Update all surfaces
}


static gboolean cwl_input_device_keycode_to_evdev(ClutterInputDevice *device, guint hardwareKeycode, guint *evdevKeycode)
{
	*evdevKeycode = hardwareKeycode;
	return TRUE;
}
static void cwl_input_device_class_init(CwlInputDeviceClass *class)
{
	((ClutterInputDeviceClass *)class)->keycode_to_evdev = cwl_input_device_keycode_to_evdev;
}
static void cwl_input_device_init(CwlInputDevice *self)
{ }

static void cwl_seat_release(struct wl_seat *seat)
{
	CwlInputDevice *device = wl_seat_get_user_data(seat);
	g_clear_pointer(&device->pointer, wl_pointer_release);
	g_clear_pointer(&device->keyboard, wl_keyboard_release);
	g_object_unref(device);
	wl_seat_set_user_data(seat, NULL);
	wl_seat_release(seat);
}

static void cwl_seat_on_update(void *data, struct wl_seat *seat, uint32_t capabilities)
{
	CwlInputDevice *device = wl_seat_get_user_data(seat);
	if(!device)
		return;
	
	if(!device->pointer && (capabilities & WL_SEAT_CAPABILITY_POINTER))
	{
		device->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(device->pointer, &cwl_pointer_cbs, device);
	}
	else if(device->pointer && !(capabilities & WL_SEAT_CAPABILITY_POINTER))
	{
		g_clear_pointer(&device->pointer, wl_pointer_release);
	}
	
	if(!device->keyboard && (capabilities & WL_SEAT_CAPABILITY_KEYBOARD))
	{
		device->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(device->keyboard, &cwl_keyboard_cbs, device);
	}
	else if(device->pointer && !(capabilities & WL_SEAT_CAPABILITY_KEYBOARD))
	{
		g_clear_pointer(&device->keyboard, wl_keyboard_release);
	}
}

// Make sure that cmk-wayland-client owns the given surface
// before trying to use its userdata
static gboolean cwl_owns_surface(struct wl_surface *surface)
{
	if(surface && cwlSurfaces)
		for(guint i=0; i<cwlSurfaces->len; ++i)
			if(g_ptr_array_index(cwlSurfaces, i) == surface)
				return TRUE;
	return FALSE;
}

static guint32 monotonic_ms(void)
{
	return g_get_monotonic_time() / 1000;
}

// TODO: Pointer & keyboard events
static void cwl_pointer_on_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
	g_message("enter");
	CwlInputDevice *device = data;
	CwlSurfaceProperties *surfaceProps = wl_surface_get_user_data(surface);
	if(!device || !surfaceProps || !cwl_owns_surface(surface))
		return;
	
	device->pointerSurface = surface;
	device->x = wl_fixed_to_double(x);
	device->y = wl_fixed_to_double(y);
	
	ClutterCrossingEvent *event = (ClutterCrossingEvent *)clutter_event_new(CLUTTER_ENTER);
	event->stage = surfaceProps->stage;
	event->time = monotonic_ms();
	event->x = device->x;
	event->y = device->y;
	event->source = CLUTTER_ACTOR(surfaceProps->stage);
	event->device = CLUTTER_INPUT_DEVICE(device);
	clutter_event_put((ClutterEvent *)event);
	clutter_event_free((ClutterEvent *)event);
}

static void cwl_pointer_on_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
	CwlInputDevice *device = data;
	if(!device || !device->pointerSurface)
		return;
	
	CwlSurfaceProperties *surfaceProps = wl_surface_get_user_data(device->pointerSurface);
	device->pointerSurface = NULL;
	if(!surfaceProps)
		return;
	
	ClutterCrossingEvent *event = (ClutterCrossingEvent *)clutter_event_new(CLUTTER_LEAVE);
	event->stage = surfaceProps->stage;
	event->time = monotonic_ms();
	event->x = device->x;
	event->y = device->y; 
	event->source = CLUTTER_ACTOR(surfaceProps->stage);
	event->device = CLUTTER_INPUT_DEVICE(device);
	clutter_event_put((ClutterEvent *)event);
	clutter_event_free((ClutterEvent *)event);
	device->x = 0;
	device->y = 0;
}

static void cwl_pointer_on_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	CwlInputDevice *device = data;
	if(!device || !device->pointerSurface)
		return;
		
	CwlSurfaceProperties *surfaceProps = wl_surface_get_user_data(device->pointerSurface);
	if(!surfaceProps)
		return;
	
	device->x = wl_fixed_to_double(x);
	device->y = wl_fixed_to_double(y);
	
	ClutterMotionEvent *event = (ClutterMotionEvent *)clutter_event_new(CLUTTER_MOTION);
	event->stage = surfaceProps->stage;
	event->time = monotonic_ms();
	event->x = device->x;
	event->y = device->y;
	event->source = CLUTTER_ACTOR(surfaceProps->stage);
	event->device = CLUTTER_INPUT_DEVICE(device);
	event->modifier_state = device->modifiers;
	clutter_event_put((ClutterEvent *)event);
	clutter_event_free((ClutterEvent *)event);
}

static void cwl_pointer_on_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	CwlInputDevice *device = data;
	if(!device || !device->pointerSurface)
		return;
		
	CwlSurfaceProperties *surfaceProps = wl_surface_get_user_data(device->pointerSurface);
	if(!surfaceProps)
		return;
	
	ClutterEventType type = state ? CLUTTER_BUTTON_PRESS : CLUTTER_BUTTON_RELEASE;
	ClutterButtonEvent *event = (ClutterButtonEvent *)clutter_event_new(type);
	event->stage = surfaceProps->stage;
	event->time = monotonic_ms();
	event->x = device->x;
	event->y = device->y;
	event->source = CLUTTER_ACTOR(surfaceProps->stage);
	event->device = CLUTTER_INPUT_DEVICE(device);
	
	ClutterModifierType mask = 0;
	switch(button)
	{
	case 272:
		event->button = 1;
		mask = CLUTTER_BUTTON1_MASK;
		break;
	case 273:
		event->button = 3;
		mask = CLUTTER_BUTTON2_MASK;
		break;
	case 274:
		event->button = 2;
		mask = CLUTTER_BUTTON3_MASK;
		break;
	}
	
	if(mask)
	{
		if(state)
			device->modifiers |= mask;
		else
			device->modifiers &= ~mask;
	}
	
	event->modifier_state = device->modifiers;
	
	clutter_event_put((ClutterEvent *)event);
	clutter_event_free((ClutterEvent *)event);
}

static void cwl_pointer_on_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	// TODO
}

static void cwl_keyboard_on_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) 
{
	// TODO
}

static void cwl_keyboard_on_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
	// TODO
}

static void cwl_keyboard_on_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
	// TODO
}

static void cwl_keyboard_on_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	// TODO
}

static void cwl_keyboard_on_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched, uint32_t modsLocked, uint32_t group)
{
	// TODO
}


static void cwl_surface_release(struct wl_surface *surface)
{
	CwlSurfaceProperties *props = wl_surface_get_user_data(surface);
	clutter_actor_destroy(CLUTTER_ACTOR(props->stage));
	g_free(props);
	wl_surface_destroy(surface);
}

ClutterStage * cwl_window_new(void)
{
	if(!cwlCompositor || !cwlShell)
		return NULL;
	
	struct wl_surface *surface = wl_compositor_create_surface(cwlCompositor);
	if(!surface)
		return NULL;
	
	struct wl_shell_surface *shellSurface = wl_shell_get_shell_surface(cwlShell, surface);
	if(!shellSurface)
	{
		wl_surface_destroy(surface);
		return NULL;
	}
	
	CwlSurfaceProperties *props = g_new0(CwlSurfaceProperties, 1);
	wl_surface_set_user_data(surface, props);
	
	props->stage = CLUTTER_STAGE(clutter_stage_new());
	clutter_wayland_stage_set_wl_surface(props->stage, surface);
	clutter_actor_show(CLUTTER_ACTOR(props->stage));
	
	return props->stage;
}

#endif
