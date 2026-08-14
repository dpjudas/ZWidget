#pragma once

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include <dlfcn.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <cstdio>

extern "C" {
extern void* gp_wl_proxy_marshal;
extern void* gp_wl_proxy_marshal_constructor;
extern void* gp_wl_proxy_marshal_constructor_versioned;
extern void* gp_wl_proxy_marshal_flags;
}

struct WaylandDynamic
{
	typedef struct wl_display* (*PFN_wl_display_connect)(const char*);
	typedef void (*PFN_wl_display_disconnect)(struct wl_display*);
	typedef int (*PFN_wl_display_dispatch)(struct wl_display*);
	typedef int (*PFN_wl_display_dispatch_pending)(struct wl_display*);
	typedef int (*PFN_wl_display_flush)(struct wl_display*);
	typedef int (*PFN_wl_display_roundtrip)(struct wl_display*);
	typedef int (*PFN_wl_display_get_fd)(struct wl_display*);
	typedef int (*PFN_wl_display_prepare_read)(struct wl_display*);
	typedef int (*PFN_wl_display_read_events)(struct wl_display*);
	typedef void (*PFN_wl_display_cancel_read)(struct wl_display*);
	typedef int (*PFN_wl_display_get_error)(struct wl_display*);

	typedef void (*PFN_wl_proxy_marshal)(struct wl_proxy*, uint32_t, ...);
	typedef struct wl_proxy* (*PFN_wl_proxy_marshal_constructor)(struct wl_proxy*, uint32_t, const struct wl_interface*, ...);
	typedef struct wl_proxy* (*PFN_wl_proxy_marshal_constructor_versioned)(struct wl_proxy*, uint32_t, const struct wl_interface*, uint32_t, ...);
	typedef struct wl_proxy* (*PFN_wl_proxy_marshal_flags)(struct wl_proxy*, uint32_t, const struct wl_interface*, uint32_t, uint32_t, ...);
	typedef struct wl_proxy* (*PFN_wl_proxy_marshal_array_flags)(struct wl_proxy*, uint32_t, const struct wl_interface*, uint32_t, uint32_t, union wl_argument*);
	typedef int (*PFN_wl_proxy_add_listener)(struct wl_proxy*, void (**)(void), void*);
	typedef void (*PFN_wl_proxy_destroy)(struct wl_proxy*);
	typedef void (*PFN_wl_proxy_set_queue)(struct wl_proxy*, struct wl_event_queue*);
	typedef void (*PFN_wl_proxy_set_user_data)(struct wl_proxy*, void*);
	typedef void* (*PFN_wl_proxy_get_user_data)(struct wl_proxy*);
	typedef uint32_t (*PFN_wl_proxy_get_version)(struct wl_proxy*);

	typedef struct wl_event_queue* (*PFN_wl_display_create_queue)(struct wl_display*);
	typedef void (*PFN_wl_event_queue_destroy)(struct wl_event_queue*);

	typedef struct wl_cursor_theme* (*PFN_wl_cursor_theme_load)(const char*, int, struct wl_shm*);
	typedef void (*PFN_wl_cursor_theme_destroy)(struct wl_cursor_theme*);
	typedef struct wl_cursor* (*PFN_wl_cursor_theme_get_cursor)(struct wl_cursor_theme*, const char*);
	typedef struct wl_buffer* (*PFN_wl_cursor_image_get_buffer)(struct wl_cursor_image*);

	typedef struct xkb_context* (*PFN_xkb_context_new)(enum xkb_context_flags);
	typedef void (*PFN_xkb_context_unref)(struct xkb_context*);
	typedef struct xkb_keymap* (*PFN_xkb_keymap_new_from_string)(struct xkb_context*, const char*, enum xkb_keymap_format, enum xkb_keymap_compile_flags);
	typedef void (*PFN_xkb_keymap_unref)(struct xkb_keymap*);
	typedef struct xkb_state* (*PFN_xkb_state_new)(struct xkb_keymap*);
	typedef void (*PFN_xkb_state_unref)(struct xkb_state*);
	typedef enum xkb_state_component (*PFN_xkb_state_update_mask)(struct xkb_state*, xkb_mod_mask_t, xkb_mod_mask_t, xkb_mod_mask_t, xkb_layout_index_t, xkb_layout_index_t, xkb_layout_index_t);
	typedef xkb_keysym_t (*PFN_xkb_state_key_get_one_sym)(struct xkb_state*, xkb_keycode_t);
	typedef int (*PFN_xkb_state_key_get_utf8)(struct xkb_state*, xkb_keycode_t, char*, size_t);

	PFN_wl_display_connect p_display_connect;
	PFN_wl_display_disconnect p_display_disconnect;
	PFN_wl_display_dispatch p_display_dispatch;
	PFN_wl_display_dispatch_pending p_display_dispatch_pending;
	PFN_wl_display_flush p_display_flush;
	PFN_wl_display_roundtrip p_display_roundtrip;
	PFN_wl_display_get_fd p_display_get_fd;
	PFN_wl_display_prepare_read p_display_prepare_read;
	PFN_wl_display_read_events p_display_read_events;
	PFN_wl_display_cancel_read p_display_cancel_read;
	PFN_wl_display_get_error p_display_get_error;

	PFN_wl_proxy_marshal p_proxy_marshal;
	PFN_wl_proxy_marshal_constructor p_proxy_marshal_constructor;
	PFN_wl_proxy_marshal_constructor_versioned p_proxy_marshal_constructor_versioned;
	PFN_wl_proxy_marshal_flags p_proxy_marshal_flags;
	PFN_wl_proxy_marshal_array_flags p_proxy_marshal_array_flags;
	PFN_wl_proxy_add_listener p_proxy_add_listener;
	PFN_wl_proxy_destroy p_proxy_destroy;
	PFN_wl_proxy_set_queue p_proxy_set_queue;
	PFN_wl_proxy_set_user_data p_proxy_set_user_data;
	PFN_wl_proxy_get_user_data p_proxy_get_user_data;
	PFN_wl_proxy_get_version p_proxy_get_version;

	PFN_wl_display_create_queue p_display_create_queue;
	PFN_wl_event_queue_destroy p_event_queue_destroy;

	PFN_wl_cursor_theme_load p_cursor_theme_load;
	PFN_wl_cursor_theme_destroy p_cursor_theme_destroy;
	PFN_wl_cursor_theme_get_cursor p_cursor_theme_get_cursor;
	PFN_wl_cursor_image_get_buffer p_cursor_image_get_buffer;

	PFN_xkb_context_new p_xkb_context_new;
	PFN_xkb_context_unref p_xkb_context_unref;
	PFN_xkb_keymap_new_from_string p_xkb_keymap_new_from_string;
	PFN_xkb_keymap_unref p_xkb_keymap_unref;
	PFN_xkb_state_new p_xkb_state_new;
	PFN_xkb_state_unref p_xkb_state_unref;
	PFN_xkb_state_update_mask p_xkb_state_update_mask;
	PFN_xkb_state_key_get_one_sym p_xkb_state_key_get_one_sym;
	PFN_xkb_state_key_get_utf8 p_xkb_state_key_get_utf8;

	const struct wl_interface* p_registry_interface;
	const struct wl_interface* p_compositor_interface;
	const struct wl_interface* p_shm_interface;
	const struct wl_interface* p_seat_interface;
	const struct wl_interface* p_output_interface;
	const struct wl_interface* p_data_device_manager_interface;

	    // Interface getter functions
    inline const struct wl_interface* GetRegistryInterface() const { return p_registry_interface; }
    inline const struct wl_interface* GetCompositorInterface() const { return p_compositor_interface; }
    inline const struct wl_interface* GetShmInterface() const { return p_shm_interface; }
    inline const struct wl_interface* GetSeatInterface() const { return p_seat_interface; }
    inline const struct wl_interface* GetOutputInterface() const { return p_output_interface; }
    inline const struct wl_interface* GetDataDeviceManagerInterface() const { return p_data_device_manager_interface; }

	static WaylandDynamic* Get()
	{
		static WaylandDynamic instance;
		return &instance;
	}

private:
	WaylandDynamic()
	{
		void* wl = dlopen("libwayland-client.so.0", RTLD_NOW | RTLD_GLOBAL);
		if (!wl) {
			fprintf(stderr, "WaylandDynamic: Failed to load libwayland-client.so.0: %s\n", dlerror());
			throw std::runtime_error("Could not load libwayland-client.so.0");
		}
		fprintf(stderr, "WaylandDynamic: Successfully loaded libwayland-client.so.0\n");

		void* wlc = dlopen("libwayland-cursor.so.0", RTLD_NOW | RTLD_GLOBAL);

		#undef LOAD_SYM
		#define LOAD_SYM(lib, name) \
			p_##name = (PFN_wl_##name)dlsym(lib, "wl_" #name); \
			if (!p_##name) fprintf(stderr, "WaylandDynamic: Failed to load symbol wl_%s\n", #name);

		LOAD_SYM(wl, display_connect);
		LOAD_SYM(wl, display_disconnect);
		LOAD_SYM(wl, display_dispatch);
		LOAD_SYM(wl, display_dispatch_pending);
		LOAD_SYM(wl, display_flush);
		LOAD_SYM(wl, display_roundtrip);
		LOAD_SYM(wl, display_get_fd);
		LOAD_SYM(wl, display_prepare_read);
		LOAD_SYM(wl, display_read_events);
		LOAD_SYM(wl, display_cancel_read);
		LOAD_SYM(wl, display_get_error);

		LOAD_SYM(wl, proxy_marshal);
		LOAD_SYM(wl, proxy_marshal_constructor);
		LOAD_SYM(wl, proxy_marshal_constructor_versioned);
		LOAD_SYM(wl, proxy_marshal_flags);
		LOAD_SYM(wl, proxy_marshal_array_flags);
		LOAD_SYM(wl, proxy_add_listener);
		LOAD_SYM(wl, proxy_destroy);
		LOAD_SYM(wl, proxy_set_queue);
		LOAD_SYM(wl, proxy_set_user_data);
		LOAD_SYM(wl, proxy_get_user_data);
		LOAD_SYM(wl, proxy_get_version);

		gp_wl_proxy_marshal = (void*)p_proxy_marshal;
		gp_wl_proxy_marshal_constructor = (void*)p_proxy_marshal_constructor;
		gp_wl_proxy_marshal_constructor_versioned = (void*)p_proxy_marshal_constructor_versioned;
		gp_wl_proxy_marshal_flags = (void*)p_proxy_marshal_flags;

		LOAD_SYM(wl, display_create_queue);
		LOAD_SYM(wl, event_queue_destroy);

		p_registry_interface = (const struct wl_interface*)dlsym(wl, "wl_registry_interface");
		if (!p_registry_interface) fprintf(stderr, "WaylandDynamic: Failed to load wl_registry_interface\n");
		p_compositor_interface = (const struct wl_interface*)dlsym(wl, "wl_compositor_interface");
		p_shm_interface = (const struct wl_interface*)dlsym(wl, "wl_shm_interface");
		p_seat_interface = (const struct wl_interface*)dlsym(wl, "wl_seat_interface");
		p_output_interface = (const struct wl_interface*)dlsym(wl, "wl_output_interface");
		p_data_device_manager_interface = (const struct wl_interface*)dlsym(wl, "wl_data_device_manager_interface");

		if (wlc)
		{
			#undef LOAD_SYM
			#define LOAD_SYM(lib, name) \
				p_##name = (PFN_wl_##name)dlsym(lib, "wl_" #name); \
				if (!p_##name) fprintf(stderr, "WaylandDynamic: Failed to load symbol wl_%s\n", #name);
			LOAD_SYM(wlc, cursor_theme_load);
			LOAD_SYM(wlc, cursor_theme_destroy);
			LOAD_SYM(wlc, cursor_theme_get_cursor);
			LOAD_SYM(wlc, cursor_image_get_buffer);
		}

		void* xkb = dlopen("libxkbcommon.so.0", RTLD_NOW | RTLD_GLOBAL);
		if (xkb)
		{
			#undef LOAD_SYM
			#define LOAD_SYM(lib, name) \
				p_##name = (PFN_##name)dlsym(lib, #name); \
				if (!p_##name) fprintf(stderr, "WaylandDynamic: Failed to load symbol %s\n", #name);
			LOAD_SYM(xkb, xkb_context_new);
			LOAD_SYM(xkb, xkb_context_unref);
			LOAD_SYM(xkb, xkb_keymap_new_from_string);
			LOAD_SYM(xkb, xkb_keymap_unref);
			LOAD_SYM(xkb, xkb_state_new);
			LOAD_SYM(xkb, xkb_state_unref);
			LOAD_SYM(xkb, xkb_state_update_mask);
			LOAD_SYM(xkb, xkb_state_key_get_one_sym);
			LOAD_SYM(xkb, xkb_state_key_get_utf8);
			fprintf(stderr, "WaylandDynamic: Successfully loaded libxkbcommon.so.0\n");
		}
		else
		{
			fprintf(stderr, "WaylandDynamic: Failed to load libxkbcommon.so.0: %s\n", dlerror());
		}
	}
};
