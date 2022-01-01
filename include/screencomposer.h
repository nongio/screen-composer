#ifndef _SCREENCOMPOSER_H
#define _SCREENCOMPOSER_H

#include <wlr/backend.h>
#include <wlr/render/gles2.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>

struct sc_compositor {
	struct wl_display *wl_display;
	struct wl_event_loop *wl_event_loop;
	struct wlr_backend *wlr_backend;
	struct wlr_renderer *wlr_renderer;
	struct wlr_allocator *wlr_allocator;
	struct wlr_compositor *wlr_compositor;
	struct wlr_presentation *wlr_presentation;
	struct wlr_egl *egl;

	/* seat */
	struct wlr_seat *seat;
	struct wlr_cursor *cursor;
	struct wlr_xcursor_manager *cursor_mgr;

	/* protocols */
	struct wlr_xdg_shell *xdg_shell;
	struct wlr_layer_shell_v1 *layer_shell;

	/* output */
	struct wl_list outputs;
	struct wlr_output_layout *output_layout;
};

struct sc_compositor *sc_compositor_init();
void sc_compositor_start();
void sc_compositor_start_eventloop();
char *sc_compositor_get_socket();
#endif
