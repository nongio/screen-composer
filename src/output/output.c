#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <wlr/interfaces/wlr_output.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output_damage.h>

#include "log.h"
#include "sc_compositor_rendering.h"
#include "sc_config.h"
#include "sc_output.h"
#include "sc_output_repaintdelay.h"
#include "sc_view.h"
#include "sc_skia.h"

extern struct sc_configuration configuration;

static int output_repaint_timer_handler(void *data);

static void output_on_frame(struct wl_listener *listener, void *data);

static void output_on_present(struct wl_listener *listener, void *data);

static void output_on_mode(struct wl_listener *listener, void *data);

static void output_update_matrix(struct sc_output *output);

struct sc_output *
sc_output_create(struct wlr_output *wlr_output,
				 struct sc_compositor *compositor)
{
	struct sc_output *output = calloc(1, sizeof(struct sc_output));

	/* TODO should check in the config if can be enabled */
	output->wlr_output = wlr_output;
	output->compositor = compositor;
	output->layout = compositor->output_layout;
	output->damage = wlr_output_damage_create(wlr_output);
	output->max_render_time = configuration.max_render_time;
	wlr_output_init_render(output->wlr_output, compositor->wlr_allocator,
						   compositor->wlr_renderer);
	wlr_output_set_custom_mode(output->wlr_output, configuration.display_width,
	configuration.display_height, configuration.display_refresh);
	wlr_output_set_scale(output->wlr_output, configuration.display_scale);

	wlr_output_commit(output->wlr_output);
	
	if (!wl_list_empty(&output->wlr_output->modes)) {
		struct wlr_output_mode *mode =
			wlr_output_preferred_mode(output->wlr_output);
		wlr_output_set_mode(output->wlr_output, mode);
		wlr_output_enable(output->wlr_output, true);

		if (!wlr_output_commit(output->wlr_output)) {
			return NULL;
		}
	}
	output->enabled = true;

	output->on_mode.notify = output_on_mode;
	wl_signal_add(&output->wlr_output->events.mode, &output->on_mode);

	// TODO add listener for layout changes
	output->output_box =
		wlr_output_layout_get_box(output->layout, output->wlr_output);

	// TODO calculate projection matrix
	output->projection_matrix = calloc(1, sizeof(float) * 9);
	output_update_matrix(output);

	output->repaint_timer = wl_event_loop_add_timer(
		compositor->wl_event_loop, output_repaint_timer_handler, output);

	/* sets up a listener for the frame notify event. */
	output->on_frame.notify = output_on_frame;
	wl_signal_add(&output->damage->events.frame, &output->on_frame);

	output->on_present.notify = output_on_present;
	wl_signal_add(&output->wlr_output->events.present, &output->on_present);
	wlr_output_damage_whole(output->wlr_output);

	int width, height;
  	wlr_output_transformed_resolution(output->wlr_output, &width, &height);
	
	
	output->fbo = fbo_create(width, height);
	output->skia = skia_context_create_for_view(output->fbo);

	return output;
}

static int
output_repaint_timer_handler(void *data)
{
	struct sc_output *output = (struct sc_output *) data;
	/* Checks if there is a need to render or skip */

	pixman_region32_t damage;
	bool needs_frame;
	pixman_region32_init(&damage);

	if (output->wlr_output == NULL) {
		goto repaint_end;
	}

	output->wlr_output->frame_pending = false;

	if (!wlr_output_damage_attach_render(output->damage, &needs_frame,
										 &damage)) {
		goto repaint_end;
	}

	if (needs_frame) {
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);

		sc_render_output(output, &now, &damage);
	} else {
		wlr_output_rollback(output->wlr_output);
	}

repaint_end:
	pixman_region32_fini(&damage);

	// Send frame done to all visible surfaces in the output
	struct timespec when;
	clock_gettime(CLOCK_MONOTONIC, &when);

	sc_output_send_frame_done(output, &when);
	return 0;
}

static void
output_on_present(struct wl_listener *listener, void *data)
{
	struct sc_output *output = wl_container_of(listener, output, on_frame);
	struct wlr_output_event_present *output_event = data;

	if (!output->enabled || !output_event->presented) {
		return;
	}
	sc_output_update_presentation(output, output_event);
}

static void
output_on_frame(struct wl_listener *listener, void *data)
{
	struct sc_output *output = wl_container_of(listener, output, on_frame);

	if (!output->enabled || !output->wlr_output->enabled) {
		return;
	}

	int delay = sc_output_get_ms_until_refresh(output);
	//	int msec_until_refresh = delay + output->max_render_time;

	// If the delay is less than 1 millisecond (which is the least we can wait)
	// then just render right away.
	if (delay < 1) {
		output_repaint_timer_handler(output);
	} else {
		DLOG("output_on_frame delay: %d\n", delay);
		output->wlr_output->frame_pending = true;
		wl_event_source_timer_update(output->repaint_timer, delay);
	}
}

static void
output_on_mode(struct wl_listener *listener, void *data)
{
	struct sc_output *output = wl_container_of(listener, output, on_mode);
	output_update_matrix(output);
}

static void
send_frame_done_iterator(struct wlr_surface *surface, int sx, int sy,
						 void *user_data)
{
	struct timespec *when = user_data;
	wlr_surface_send_frame_done(surface, when);
}

void
sc_output_send_frame_done(struct sc_output *output, struct timespec *when)
{
	sc_output_for_each_view_surface(output, send_frame_done_iterator, when);
}
static void
output_update_matrix(struct sc_output *output)
{
	wlr_output_effective_resolution(output->wlr_output, &output->width,
									&output->height);

	wlr_matrix_projection(output->projection_matrix, output->width,
						  output->height, WL_OUTPUT_TRANSFORM_NORMAL);
}
