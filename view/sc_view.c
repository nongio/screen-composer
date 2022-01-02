#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>

#include "log.h"
#include "sc_output.h"
#include "sc_view.h"
#include <wlr/types/wlr_surface.h>

static void
view_surface_commit_handler(struct wl_listener *listener, void *data)
{
	DLOG("view_surface_commit\n");
	struct sc_view *view = wl_container_of(listener, view, on_surface_commit);

	if (view->output == NULL) {
		return;
	}
	if (view->parent != NULL) {
		sc_output_damage_view(view->output, view->parent, false);
	} else {
		sc_output_damage_view(view->output, view, false);
	}
}

static void
subview_destroy_handler(struct wl_listener *listener, void *data)
{
	struct sc_view *subview =
		wl_container_of(listener, subview, on_subview_destroy);

	if (subview->output == NULL) {
		return;
	}

	if (subview->parent != NULL) {
		sc_output_damage_view(subview->output, subview->parent, false);
	} else {
		sc_output_damage_view(subview->output, subview, false);
	}

	sc_view_destroy(subview);
	free(subview);
}

static struct sc_view *
view_subsurface_create(struct wlr_subsurface *subsurface)
{
	struct sc_view *subview = calloc(1, sizeof(struct sc_view));
	sc_view_init(subview, subsurface->surface);

	subview->on_subview_destroy.notify = subview_destroy_handler;
	wl_signal_add(&subsurface->events.destroy, &subview->on_subview_destroy);

	return subview;
}

static void
view_subsurface_new(struct wl_listener *listener, void *data)
{
	struct sc_view *view = wl_container_of(listener, view, on_subsurface_new);

	struct wlr_subsurface *subsurface = data;

	struct sc_view *subview = view_subsurface_create(subsurface);

	subview->parent = view;
	wl_list_insert(&view->children, &subview->link);
}

void
sc_view_init(struct sc_view *view, struct wlr_surface *surface)
{
	DLOG("sc_view_init\n");
	view->surface = surface;
	wl_list_init(&view->children);

	view->on_surface_commit.notify = view_surface_commit_handler;
	wl_signal_add(&view->surface->events.commit, &view->on_surface_commit);

	view->on_subsurface_new.notify = view_subsurface_new;
	wl_signal_add(&view->surface->events.new_subsurface,
				  &view->on_subsurface_new);
}

void
sc_view_destroy(struct sc_view *view)
{
	wl_list_remove(&view->link);
	wl_list_remove(&view->on_subsurface_new.link);
	wl_list_remove(&view->on_surface_commit.link);
	wl_list_remove(&view->on_subview_destroy.link);
}

void
sc_view_damage_part(struct sc_view *view)
{
	if (view->output != NULL) {
	}
}

void
sc_view_damage_whole(struct sc_view *view)
{
	if (view->output != NULL) {
		sc_output_damage_view(view->output, view, true);
	}
}

void
sc_view_map(struct sc_view *view)
{
	view->mapped = true;
	struct wlr_subsurface *sub;
	wl_list_for_each (sub, &view->surface->current.subsurfaces_below,
					  current.link) {
		struct sc_view *subview = view_subsurface_create(sub);
		subview->parent = view;
		wl_list_insert(&view->children, &subview->link);
	}
	wl_list_for_each (sub, &view->surface->current.subsurfaces_above,
					  current.link) {
		struct sc_view *subview = view_subsurface_create(sub);
		subview->parent = view;
		wl_list_insert(&view->children, &subview->link);
	}
}
