/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* nemo-pathbar.c
 * Copyright (C) 2004  Red Hat, Inc.,  Jonathan Blandford <jrb@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
 * Boston, MA 02110-1335, USA.
 */

extern "C"
{
#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <math.h>

#include "nemo-pathbar.h"

#include <eel/eel-vfs-extensions.h>

#include <libnemo-private/nemo-file.h>
#include <libnemo-private/nemo-file-utilities.h>
#include <libnemo-private/nemo-global-preferences.h>
#include <libnemo-private/nemo-icon-names.h>
#include <libnemo-private/nemo-icon-dnd.h>

#include "nemo-window.h"
#include "nemo-window-private.h"
#include "nemo-window-slot.h"
#include "nemo-window-slot-dnd.h"
}

#include "gtk_container.hpp"
#include "gtk_button.hpp"
#include "gtk_image.hpp"
#include <cstdint>
#include "nemo-pathbar-button.hpp"

namespace nemo
{
    struct PathBar : public gtk::Container
    {
        PathBar(GtkContainer* cClass) : gtk::Container(cClass){}
    };
}

enum {
    PATH_CLICKED,
    PATH_SET,
    LAST_SIGNAL
};

#define SCROLL_TIMEOUT           150
#define INITIAL_SCROLL_TIMEOUT   300

static guint path_bar_signals [LAST_SIGNAL] = { 0 };

static gboolean desktop_is_home;

#define NEMO_PATH_BAR_ICON_SIZE 16
#define NEMO_PATH_BAR_BUTTON_MAX_WIDTH 250

/*
 * Content of pathbar->button_list:
 *       <- next                      previous ->
 * ---------------------------------------------------------------------
 * | /   |   home  |   user      | downloads    | folder   | sub folder
 * ---------------------------------------------------------------------
 *  last             fake_root                              button_list
 *                                scrolled_root_button
 */

struct _NemoPathBarDetails {
	GtkContainer parent;

	GdkWindow *event_window;

	GFile *root_path;
	GFile *home_path;
	GFile *desktop_path;

	GFile *current_path;
	gpointer current_button_data;

	GList *button_list;
	GList *scrolled_root_button;
	GList *fake_root;
	gtk::Button *up_slider_button;
	gtk::Button *down_slider_button;
	unsigned int settings_signal_id;
	int slider_width;
	int16_t spacing;
	int16_t button_offset;
	unsigned int timer;
	unsigned int slider_visible : 1;
	unsigned int need_timer : 1;
	unsigned int ignore_click : 1;

	unsigned int drag_slider_timeout;
	bool drag_slider_timeout_for_up_button;
};

G_DEFINE_TYPE (NemoPathBar, nemo_path_bar, GTK_TYPE_CONTAINER);

static void     nemo_path_bar_scroll_up                (NemoPathBar *path_bar);
static void     nemo_path_bar_scroll_down              (NemoPathBar *path_bar);
static void     nemo_path_bar_stop_scrolling           (NemoPathBar *path_bar);
static gboolean nemo_path_bar_slider_button_press      (GtkWidget       *widget,
                                GdkEventButton  *event,
                                NemoPathBar *path_bar);
static gboolean nemo_path_bar_slider_button_release    (GtkWidget       *widget,
                                GdkEventButton  *event,
                                NemoPathBar *path_bar);
static void     nemo_path_bar_check_icon_theme         (NemoPathBar *path_bar);

static gboolean nemo_path_bar_update_path              (NemoPathBar *path_bar,
                                GFile           *file_path,
                                gboolean         emit_signal);

static gtk::Button* get_slider_button (NemoPathBar *path_bar, GtkPositionType position)
{
    gtk_widget_push_composite_child();

    gtk::Button* button = new gtk::Button{};
    button->get_style_context()->add_class("slider-button");
    button->set_focus_on_click(false);
    button->add_events(GDK_SCROLL_MASK);
    
    gtk::Image* image;
    if (position == GTK_POS_LEFT)
        image = new gtk::Image{"pan-start-symbolic", GTK_ICON_SIZE_MENU};
    else
        image = new gtk::Image{"pan-end-symbolic", GTK_ICON_SIZE_MENU};
    button->takeOnOwnership(image);
    button->add(*image);
    gtk_container_add (GTK_CONTAINER (path_bar), button->getPtr());
    button->show_all();

    gtk_widget_pop_composite_child();

    return button;
}

static void update_button_types (NemoPathBar *path_bar)
{
    GList *list;
    GFile *path = NULL;

    for (list = path_bar->priv->button_list; list; list = list->next) {
        nemo::PathBarButton *button_data;
        button_data = (nemo::PathBarButton*) (list->data);
        if (button_data->get_active()) {
            path = (GFile*)g_object_ref (button_data->path);
            break;
        }
    }
    if (path != NULL) {
        nemo_path_bar_update_path (path_bar, path, TRUE);
        g_object_unref (path);
    }
}

static void desktop_location_changed_callback (gpointer user_data)
{
    NemoPathBar *path_bar;

    path_bar = NEMO_PATH_BAR (user_data);

    g_object_unref (path_bar->priv->desktop_path);
    g_object_unref (path_bar->priv->home_path);
    path_bar->priv->desktop_path = nemo_get_desktop_location ();
    path_bar->priv->home_path = g_file_new_for_path (g_get_home_dir ());
    desktop_is_home = g_file_equal (path_bar->priv->home_path, path_bar->priv->desktop_path);

    update_button_types (path_bar);
}

static gboolean slider_timeout (gpointer user_data)
{
    NemoPathBar *path_bar;

    path_bar = NEMO_PATH_BAR (user_data);

    path_bar->priv->drag_slider_timeout = 0;

    if (gtk_widget_get_visible (GTK_WIDGET (path_bar))) {
        if (path_bar->priv->drag_slider_timeout_for_up_button) {
            nemo_path_bar_scroll_up (path_bar);
        } else {
            nemo_path_bar_scroll_down (path_bar);
        }
    }

    return FALSE;
}

static void nemo_path_bar_slider_drag_motion (GtkWidget      *widget,
                      GdkDragContext *context,
                      int             x,
                      int             y,
                      unsigned int    time,
                      gpointer        user_data)
{
    NemoPathBar *path_bar;
    GtkSettings *settings;
    unsigned int timeout;

    path_bar = NEMO_PATH_BAR (user_data);

    if (path_bar->priv->drag_slider_timeout == 0) {
        settings = gtk_widget_get_settings (widget);

        g_object_get (settings, "gtk-timeout-expand", &timeout, NULL);
        path_bar->priv->drag_slider_timeout =
            g_timeout_add (timeout,
                       slider_timeout,
                       path_bar);

        path_bar->priv->drag_slider_timeout_for_up_button =
            widget == path_bar->priv->up_slider_button->getPtr();
    }
}

static void
nemo_path_bar_slider_drag_leave (GtkWidget      *widget,
                     GdkDragContext *context,
                     unsigned int    time,
                     gpointer        user_data)
{
    NemoPathBar *path_bar;

    path_bar = NEMO_PATH_BAR (user_data);

    if (path_bar->priv->drag_slider_timeout != 0) {
        g_source_remove (path_bar->priv->drag_slider_timeout);
        path_bar->priv->drag_slider_timeout = 0;
    }
}



static void
nemo_path_bar_init (NemoPathBar *path_bar)
{
    char *p;

    nemo::PathBar* cppClass = new nemo::PathBar((GtkContainer*)path_bar);
    path_bar->cppParent = cppClass;

    path_bar->priv = G_TYPE_INSTANCE_GET_PRIVATE (path_bar, NEMO_TYPE_PATH_BAR, NemoPathBarDetails);

    gtk_widget_set_has_window (GTK_WIDGET (path_bar), FALSE);
    gtk_widget_set_redraw_on_allocate (GTK_WIDGET (path_bar), FALSE);

    path_bar->priv->up_slider_button = get_slider_button (path_bar, GTK_POS_LEFT);
    path_bar->priv->down_slider_button = get_slider_button (path_bar, GTK_POS_RIGHT);

    p = nemo_get_desktop_directory ();
    path_bar->priv->desktop_path = g_file_new_for_path (p);
    g_free (p);
    path_bar->priv->home_path = g_file_new_for_path (g_get_home_dir ());
    path_bar->priv->root_path = g_file_new_for_path ("/");

    desktop_is_home = g_file_equal (path_bar->priv->home_path, path_bar->priv->desktop_path);

    g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_DESKTOP_IS_HOME_DIR,
                  G_CALLBACK(desktop_location_changed_callback),
                  path_bar);

    g_signal_connect_swapped (path_bar->priv->up_slider_button->getPtr(), "clicked", G_CALLBACK (nemo_path_bar_scroll_up), path_bar);
    g_signal_connect_swapped (path_bar->priv->down_slider_button->getPtr(), "clicked", G_CALLBACK (nemo_path_bar_scroll_down), path_bar);

    g_signal_connect (path_bar->priv->up_slider_button->getPtr(), "button_press_event", G_CALLBACK (nemo_path_bar_slider_button_press), path_bar);
    g_signal_connect (path_bar->priv->up_slider_button->getPtr(), "button_release_event", G_CALLBACK (nemo_path_bar_slider_button_release), path_bar);
    g_signal_connect (path_bar->priv->down_slider_button->getPtr(), "button_press_event", G_CALLBACK (nemo_path_bar_slider_button_press), path_bar);
    g_signal_connect (path_bar->priv->down_slider_button->getPtr(), "button_release_event", G_CALLBACK (nemo_path_bar_slider_button_release), path_bar);

    gtk_drag_dest_set (GTK_WIDGET (path_bar->priv->up_slider_button->getPtr()),
               (GtkDestDefaults)0, NULL, 0, (GdkDragAction)0);
    gtk_drag_dest_set_track_motion (GTK_WIDGET (path_bar->priv->up_slider_button->getPtr()), TRUE);
    g_signal_connect (path_bar->priv->up_slider_button->getPtr(),
              "drag-motion",
              G_CALLBACK (nemo_path_bar_slider_drag_motion),
              path_bar);
    g_signal_connect (path_bar->priv->up_slider_button->getPtr(),
              "drag-leave",
              G_CALLBACK (nemo_path_bar_slider_drag_leave),
              path_bar);

    gtk_drag_dest_set (GTK_WIDGET (path_bar->priv->down_slider_button->getPtr()),
               (GtkDestDefaults)0, NULL, 0, (GdkDragAction)0);
    gtk_drag_dest_set_track_motion (GTK_WIDGET (path_bar->priv->up_slider_button->getPtr()), TRUE);
    g_signal_connect (path_bar->priv->down_slider_button->getPtr(),
              "drag-motion",
              G_CALLBACK (nemo_path_bar_slider_drag_motion),
              path_bar);
    g_signal_connect (path_bar->priv->down_slider_button->getPtr(),
              "drag-leave",
              G_CALLBACK (nemo_path_bar_slider_drag_leave),
              path_bar);

    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (path_bar)), GTK_STYLE_CLASS_LINKED);
    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (path_bar)), "path-bar");
}

static void
nemo_path_bar_finalize (GObject *object)
{
    NemoPathBar *path_bar;

    path_bar = NEMO_PATH_BAR (object);

    nemo_path_bar_stop_scrolling (path_bar);

    if (path_bar->priv->drag_slider_timeout != 0) {
        g_source_remove (path_bar->priv->drag_slider_timeout);
        path_bar->priv->drag_slider_timeout = 0;
    }

    g_list_free (path_bar->priv->button_list);
    g_clear_object (&path_bar->priv->desktop_path);

    g_signal_handlers_disconnect_by_func (nemo_preferences,
                          (gpointer)desktop_location_changed_callback,
                          path_bar);

    G_OBJECT_CLASS (nemo_path_bar_parent_class)->finalize (object);
}

/* Removes the settings signal handler.  It's safe to call multiple times */
static void
remove_settings_signal (NemoPathBar *path_bar,
            GdkScreen  *screen)
{
    if (path_bar->priv->settings_signal_id) {
          GtkSettings *settings;

         settings = gtk_settings_get_for_screen (screen);
         g_signal_handler_disconnect (settings,
                            path_bar->priv->settings_signal_id);
        path_bar->priv->settings_signal_id = 0;
    }
}

static void
nemo_path_bar_dispose (GObject *object)
{
    remove_settings_signal (NEMO_PATH_BAR (object), gtk_widget_get_screen (GTK_WIDGET (object)));

    G_OBJECT_CLASS (nemo_path_bar_parent_class)->dispose (object);
}

/* Size requisition:
 *
 * Ideally, our size is determined by another widget, and we are just filling
 * available space.
 */
static void
nemo_path_bar_get_preferred_width (GtkWidget *widget,
                       gint      *minimum,
                       gint      *natural)
{
    nemo::PathBarButton *button_data;
    NemoPathBar *path_bar;
    GList *list;
    int child_min, child_nat;

    path_bar = NEMO_PATH_BAR (widget);

    *minimum = *natural = 0;

    for (list = path_bar->priv->button_list; list; list = list->next) {
        button_data = (nemo::PathBarButton*) (list->data);
        button_data->get_preferred_width(child_min, child_nat);
        *minimum = MAX (*minimum, child_min);
        *natural = MAX (*natural, child_nat);
    }

    path_bar->priv->down_slider_button->get_preferred_width(path_bar->priv->slider_width);

    *minimum += path_bar->priv->slider_width * 2;
    *natural += path_bar->priv->slider_width * 2;
}

GtkWidget* nemo_path_bar_new()
{
    NemoPathBar* cClass = (NemoPathBar*)g_object_new (NEMO_TYPE_PATH_BAR, nullptr); //calls init
    return (GtkWidget*)cClass;
}

static void button_data_file_changed (NemoFile *file,
              nemo::PathBarButton *button_data)
{
    GFile *location, *current_location, *parent, *button_parent;
    nemo::PathBarButton *current_button_data;
    char *display_name;
    NemoPathBar *path_bar;
    gboolean renamed, child;

    path_bar = (NemoPathBar *) gtk_widget_get_ancestor (button_data->getPtr(),
                                NEMO_TYPE_PATH_BAR);
    if (path_bar == NULL) {
        return;
    }

    g_assert (path_bar->priv->current_path != NULL);
    g_assert (path_bar->priv->current_button_data != NULL);

    current_button_data = (nemo::PathBarButton*)path_bar->priv->current_button_data;

    location = nemo_file_get_location (file);
    if (!g_file_equal (button_data->path, location)) {
        parent = g_file_get_parent (location);
        button_parent = g_file_get_parent (button_data->path);

        renamed = (parent != NULL && button_parent != NULL) &&
               g_file_equal (parent, button_parent);

        if (parent != NULL) {
            g_object_unref (parent);
        }
        if (button_parent != NULL) {
            g_object_unref (button_parent);
        }

        if (renamed) {
            button_data->path = (GFile*)g_object_ref (location);
        } else {
            /* the file has been moved.
             * If it was below the currently displayed location, remove it.
             * If it was not below the currently displayed location, update the path bar
             */
            child = g_file_has_prefix (button_data->path,
                           path_bar->priv->current_path);

            if (child) {
                /* moved file inside current path hierarchy */
                g_object_unref (location);
                location = g_file_get_parent (button_data->path);
                current_location = (GFile*)g_object_ref (path_bar->priv->current_path);
            } else {
                /* moved current path, or file outside current path hierarchy.
                 * Update path bar to new locations.
                 */
                current_location = nemo_file_get_location (current_button_data->file);
            }

                nemo_path_bar_update_path (path_bar, location, FALSE);
                nemo_path_bar_set_path (path_bar, current_location);
            g_object_unref (location);
            g_object_unref (current_location);
            return;
        }
    } else if (nemo_file_is_gone (file)) {
        gint idx, position;

        /* if the current or a parent location are gone, don't do anything, as the view
         * will get the event too and call us back.
         */
        current_location = nemo_file_get_location (current_button_data->file);

        if (g_file_has_prefix (location, current_location)) {
            /* remove this and the following buttons */
            position = g_list_position (path_bar->priv->button_list,
                    g_list_find (path_bar->priv->button_list, button_data));

            if (position != -1) {
                for (idx = 0; idx <= position; idx++) {
                    gtk_container_remove (GTK_CONTAINER (path_bar),
                                  ((nemo::PathBarButton*) (path_bar->priv->button_list->data))->getPtr());
                }
            }
        }

        g_object_unref (current_location);
        g_object_unref (location);
        return;
    }
    g_object_unref (location);

    /* MOUNTs use the GMount as the name, so don't update for those */
    if (button_data->type != nemo::ButtonType::Mount) {
        display_name = nemo_file_get_display_name (file);
        if (g_strcmp0 (display_name, button_data->dir_name) != 0) {
            g_free (button_data->dir_name);
            button_data->dir_name = g_strdup (display_name);
        }

        g_free (display_name);
    }
    button_data->update_button_appearance();
}


static void nemo_path_bar_get_preferred_height(GtkWidget *widget, int *minimum, int *natural)
{
    nemo::PathBarButton *button_data;
    NemoPathBar *path_bar;
    GList *list;
    gint child_min, child_nat;

    path_bar = NEMO_PATH_BAR (widget);

    *minimum = *natural = 0;

    for (list = path_bar->priv->button_list; list; list = list->next) {
        button_data = (nemo::PathBarButton*) (list->data);
       gtk_widget_get_preferred_height (button_data->getPtr(), &child_min, &child_nat);

        *minimum = MAX (*minimum, child_min);
        *natural = MAX (*natural, child_nat);
    }
}

static void nemo_path_bar_update_slider_buttons(NemoPathBar *path_bar)
{
    if (path_bar->priv->button_list) {

        GtkWidget *button;

        button = ((nemo::PathBarButton*) (path_bar->priv->button_list->data))->getPtr();
        if (gtk_widget_get_child_visible (button))
        {
            path_bar->priv->down_slider_button->set_sensitive(false);
            nemo_path_bar_stop_scrolling (path_bar);
        } 
        else
            path_bar->priv->down_slider_button->set_sensitive(true);

        button = ((nemo::PathBarButton*) (g_list_last (path_bar->priv->button_list)->data))->getPtr();
        if (gtk_widget_get_child_visible (button))
        {
            path_bar->priv->up_slider_button->set_sensitive(false);
            nemo_path_bar_stop_scrolling (path_bar);
        }
        else
            path_bar->priv->up_slider_button->set_sensitive(true);
    }
}

static void nemo_path_bar_unmap (GtkWidget *widget)
{
    nemo_path_bar_stop_scrolling (NEMO_PATH_BAR (widget));
    gdk_window_hide (NEMO_PATH_BAR (widget)->priv->event_window);

    GTK_WIDGET_CLASS (nemo_path_bar_parent_class)->unmap (widget);
}

static void nemo_path_bar_map (GtkWidget *widget)
{
    gdk_window_show (NEMO_PATH_BAR (widget)->priv->event_window);

    GTK_WIDGET_CLASS (nemo_path_bar_parent_class)->map (widget);
}

static void child_ordering_changed (NemoPathBar *path_bar)
{
    GList *l;

    if (path_bar->priv->up_slider_button->getPtr())
        path_bar->priv->up_slider_button->get_style_context()->invalidate();

    if (path_bar->priv->down_slider_button->getPtr())
        path_bar->priv->down_slider_button->get_style_context()->invalidate();


    for (l = path_bar->priv->button_list; l; l = l->next) {
        nemo::PathBarButton *data = (nemo::PathBarButton*)l->data;
        gtk_style_context_invalidate (gtk_widget_get_style_context (data->getPtr()));
    }
}

/* This is a tad complicated */
static void nemo_path_bar_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    GtkWidget *child;
    NemoPathBar *path_bar;
    GtkTextDirection direction;
    GtkAllocation child_allocation;
    GList *list, *pathbar_root_button;
    gint width;
    gint width_min;
    gint largest_width;
    gboolean need_sliders;
    gint up_slider_offset;
    gint down_slider_offset;
    GtkRequisition child_requisition;
    GtkRequisition child_requisition_min;
    gboolean needs_reorder = FALSE;
    gint button_count = 0;

    need_sliders = TRUE;
    up_slider_offset = 0;
    down_slider_offset = 0;
    path_bar = NEMO_PATH_BAR (widget);

    gtk_widget_set_allocation (widget, allocation);

    if (gtk_widget_get_realized (widget)) {
        gdk_window_move_resize (path_bar->priv->event_window,
                    allocation->x, allocation->y,
                    allocation->width, allocation->height);
    }

    /* No path is set so we don't have to allocate anything. */
    if (path_bar->priv->button_list == NULL) {
        return;
    }
    direction = gtk_widget_get_direction (widget);

    path_bar->priv->up_slider_button->get_preferred_width(path_bar->priv->slider_width);

    gtk_widget_get_preferred_size (((nemo::PathBarButton*) (path_bar->priv->button_list->data))->getPtr(),
                       NULL, &child_requisition);
    width = child_requisition.width;

    for (list = path_bar->priv->button_list->next; list; list = list->next) {
        child = ((nemo::PathBarButton*) (list->data))->getPtr();
        gtk_widget_get_preferred_size (child,
                       NULL, &child_requisition);
        width += child_requisition.width;

        if (list == path_bar->priv->fake_root) {
            width += path_bar->priv->slider_width;
            break;
        }
    }

    largest_width = allocation->width;

    if (width <= allocation->width && !need_sliders) {
        if (path_bar->priv->fake_root) {
            pathbar_root_button = path_bar->priv->fake_root;
        } else {
            pathbar_root_button = g_list_last (path_bar->priv->button_list);
        }
    } else {
        gboolean reached_end;
        gint slider_space;
        reached_end = FALSE;
		need_sliders = TRUE;
        slider_space = 2 * path_bar->priv->slider_width;
        largest_width -= slider_space;

        /* To see how much space we have, and how many buttons we can display.
         * We start at the first button, count forward until hit the new
         * button, then count backwards.
         */

        /* First assume, we can only display one button */
        if (path_bar->priv->scrolled_root_button) {
            pathbar_root_button = path_bar->priv->scrolled_root_button;
        } else {
            pathbar_root_button = path_bar->priv->button_list;
        }

        /* Count down the path chain towards the end. */
        gtk_widget_get_preferred_size (((nemo::PathBarButton*) (pathbar_root_button->data))->getPtr(),
                                       NULL, &child_requisition);
        button_count = 1;
        width = child_requisition.width;

        /* Count down the path chain towards the end. */
        list = pathbar_root_button->prev;
        while (list && !reached_end) {
            if (list == path_bar->priv->fake_root) {
                break;
            }
            child = ((nemo::PathBarButton*) (list->data))->getPtr();
            gtk_widget_get_preferred_size (child, NULL, &child_requisition);

            if (width + child_requisition.width + slider_space > allocation->width) {
                reached_end = TRUE;
                if (button_count == 1) {
                    /* Display two Buttons if they fit shrinked */
                    gtk_widget_get_preferred_size (child, &child_requisition_min, NULL);
                    width_min = child_requisition_min.width;
                    gtk_widget_get_preferred_size (((nemo::PathBarButton*) (pathbar_root_button->data))->getPtr(), &child_requisition_min, NULL);
                    width_min += child_requisition_min.width;
                    if (width_min <= largest_width) {
                        button_count++;
                        largest_width /= 2;
                        if (width < largest_width) {
                            /* unused space for second button */
                            largest_width += largest_width - width;
                        } else if (child_requisition.width < largest_width) {
                            /* unused space for first button */
                            largest_width += largest_width - child_requisition.width;
                        }
                    }
                }
            } else {
                width += child_requisition.width;
            }
            list = list->prev;
        }

        /* Finally, we walk up, seeing how many of the previous buttons we can add*/
        while (pathbar_root_button->next && ! reached_end) {
            if (pathbar_root_button == path_bar->priv->fake_root) {
                break;
            }
            child = ((nemo::PathBarButton*) (pathbar_root_button->next->data))->getPtr();
            gtk_widget_get_preferred_size (child, NULL, &child_requisition);

            if (width + child_requisition.width + slider_space > allocation->width) {
                reached_end = TRUE;
                if (button_count == 1) {
            	    gtk_widget_get_preferred_size (child, &child_requisition_min, NULL);
            	    width_min = child_requisition_min.width;
            	    gtk_widget_get_preferred_size (((nemo::PathBarButton*) (pathbar_root_button->data))->getPtr(), &child_requisition_min, NULL);
            	    width_min += child_requisition_min.width;
            	    if (width_min <= largest_width) {
                        // Two shinked buttons fit
                        pathbar_root_button = pathbar_root_button->next;
                        button_count++;
                        largest_width /= 2;
                        if (width < largest_width) {
                            /* unused space for second button */
                            largest_width += largest_width - width;
                        } else if (child_requisition.width < largest_width) {
                            /* unused space for first button */
                            largest_width += largest_width - child_requisition.width;
                        }
            	    }
                }
            } else {
                width += child_requisition.width;
                pathbar_root_button = pathbar_root_button->next;
                button_count++;
            }
        }
    }

    /* Now, we allocate space to the buttons */
    child_allocation.y = allocation->y;
    child_allocation.height = allocation->height;

    if (direction == GTK_TEXT_DIR_RTL) {
        child_allocation.x = allocation->x + allocation->width;
        if (need_sliders || path_bar->priv->fake_root) {
            child_allocation.x -= (path_bar->priv->slider_width);
            up_slider_offset = allocation->width - path_bar->priv->slider_width;
        }
    } else {
        child_allocation.x = allocation->x;
        if (need_sliders || path_bar->priv->fake_root) {
            up_slider_offset = 0;
            child_allocation.x += path_bar->priv->slider_width;
        }
    }

    for (list = pathbar_root_button; list; list = list->prev) {
        child = ((nemo::PathBarButton*) (list->data))->getPtr();
        gtk_widget_get_preferred_size (child,
                                        NULL, &child_requisition);


        child_allocation.width = MIN (child_requisition.width, largest_width);
        if (direction == GTK_TEXT_DIR_RTL) {
            child_allocation.x -= child_allocation.width;
        }
    	/* Check to see if we've don't have any more space to allocate buttons */
        if (need_sliders && direction == GTK_TEXT_DIR_RTL) {
            if (child_allocation.x - path_bar->priv->slider_width < allocation->x) {
                break;
            }
        } else {
            if (need_sliders && direction == GTK_TEXT_DIR_LTR) {
                if (child_allocation.x + child_allocation.width + path_bar->priv->slider_width > allocation->x + allocation->width) {
                    break;
                }
            }
        }

        needs_reorder |= gtk_widget_get_child_visible (child) == FALSE;
        gtk_widget_set_child_visible (child, TRUE);
        gtk_widget_size_allocate (child, &child_allocation);

        if (direction == GTK_TEXT_DIR_RTL) {
            down_slider_offset = child_allocation.x - allocation->x - path_bar->priv->slider_width;
        } else {
            down_slider_offset += child_allocation.width;
            child_allocation.x += child_allocation.width;
        }
    }
    /* Now we go hide all the widgets that don't fit */
    while (list) {
        child = ((nemo::PathBarButton*) (list->data))->getPtr();
        needs_reorder |= gtk_widget_get_child_visible (child) == TRUE;
        gtk_widget_set_child_visible (child, FALSE);
        list = list->prev;
    }

    if (((nemo::PathBarButton*) (pathbar_root_button->data))->fake_root) {
        path_bar->priv->fake_root = pathbar_root_button;
    }

    for (list = pathbar_root_button->next; list; list = list->next) {
        child = ((nemo::PathBarButton*) (list->data))->getPtr();
        needs_reorder |= gtk_widget_get_child_visible (child) == TRUE;
        gtk_widget_set_child_visible (child, FALSE);
        if (((nemo::PathBarButton*) (list->data))->fake_root) {
            path_bar->priv->fake_root = list;
        }
    }

    if (need_sliders || path_bar->priv->fake_root) {
        child_allocation.width = path_bar->priv->slider_width;
        child_allocation.x = up_slider_offset + allocation->x;
        path_bar->priv->up_slider_button->size_allocate(&child_allocation);

        needs_reorder |= path_bar->priv->up_slider_button->get_child_visible() == false;
        path_bar->priv->up_slider_button->set_child_visible(true);
        path_bar->priv->up_slider_button->show_all();

        if (direction == GTK_TEXT_DIR_LTR) {
            down_slider_offset += path_bar->priv->slider_width;
        }
    } else {
        needs_reorder |= path_bar->priv->up_slider_button->get_child_visible() == true;
        path_bar->priv->up_slider_button->set_child_visible(false);
    }

    if (need_sliders) {
        child_allocation.width = path_bar->priv->slider_width;
        child_allocation.x = down_slider_offset + allocation->x;
        path_bar->priv->down_slider_button->size_allocate(&child_allocation);

        needs_reorder |= path_bar->priv->up_slider_button->get_child_visible() == false;
        path_bar->priv->down_slider_button->set_child_visible(true);
        path_bar->priv->down_slider_button->show_all();
        nemo_path_bar_update_slider_buttons (path_bar);
    } else {
        needs_reorder |= path_bar->priv->up_slider_button->get_child_visible() == true;
        path_bar->priv->down_slider_button->set_child_visible(false);
        /* Reset Scrolling to have the left most folder in focus when resizing again */
        path_bar->priv->scrolled_root_button = NULL;
    }

    if (needs_reorder) {
        child_ordering_changed (path_bar);
    }
}

static void nemo_path_bar_style_updated (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (nemo_path_bar_parent_class)->style_updated (widget);

    nemo_path_bar_check_icon_theme (NEMO_PATH_BAR (widget));
}

static void nemo_path_bar_screen_changed (GtkWidget *widget, GdkScreen *previous_screen)
{
    if (GTK_WIDGET_CLASS (nemo_path_bar_parent_class)->screen_changed) {
        GTK_WIDGET_CLASS (nemo_path_bar_parent_class)->screen_changed (widget, previous_screen);
    }
        /* We might nave a new settings, so we remove the old one */
    if (previous_screen) {
        remove_settings_signal (NEMO_PATH_BAR (widget), previous_screen);
    }
    nemo_path_bar_check_icon_theme (NEMO_PATH_BAR (widget));
}

static gboolean nemo_path_bar_scroll(GtkWidget *widget, GdkEventScroll *event)
{
    NemoPathBar *path_bar;

    path_bar = NEMO_PATH_BAR (widget);

    switch (event->direction) {
        case GDK_SCROLL_RIGHT:
        case GDK_SCROLL_DOWN:
            nemo_path_bar_scroll_down (path_bar);
            return TRUE;

        case GDK_SCROLL_LEFT:
        case GDK_SCROLL_UP:
            nemo_path_bar_scroll_up (path_bar);
            return TRUE;
        case GDK_SCROLL_SMOOTH:
            break;
        default:
            break;
    }

    return FALSE;
}

static void nemo_path_bar_realize(GtkWidget *widget)
{
    NemoPathBar *path_bar;
    GtkAllocation allocation;
    GdkWindow *window;
    GdkWindowAttr attributes;
    gint attributes_mask;

    gtk_widget_set_realized (widget, TRUE);

    path_bar = NEMO_PATH_BAR (widget);
    window = gtk_widget_get_parent_window (widget);
    gtk_widget_set_window (widget, window);
    g_object_ref (window);

    gtk_widget_get_allocation (widget, &allocation);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_ONLY;
    attributes.event_mask = gtk_widget_get_events (widget);
    attributes.event_mask |=
        GDK_SCROLL_MASK |
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK;
    attributes_mask = GDK_WA_X | GDK_WA_Y;

    path_bar->priv->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                         &attributes, attributes_mask);
    gdk_window_set_user_data (path_bar->priv->event_window, widget);
}

static void nemo_path_bar_unrealize(GtkWidget *widget)
{
    NemoPathBar *path_bar;

    path_bar = NEMO_PATH_BAR (widget);

    gdk_window_set_user_data (path_bar->priv->event_window, NULL);
    gdk_window_destroy (path_bar->priv->event_window);
    path_bar->priv->event_window = NULL;

    GTK_WIDGET_CLASS (nemo_path_bar_parent_class)->unrealize (widget);
}

static void nemo_path_bar_add (GtkContainer *container, GtkWidget *widget)
{
    gtk_widget_set_parent (widget, GTK_WIDGET (container));
}

static void nemo_path_bar_remove_1 (GtkContainer *container, GtkWidget *widget)
{
    gboolean was_visible = gtk_widget_get_visible (widget);
    gtk_widget_unparent (widget);
    if (was_visible) {
        gtk_widget_queue_resize (GTK_WIDGET (container));
    }
}

static void nemo_path_bar_remove (GtkContainer *container, GtkWidget *widget)
{
    NemoPathBar *path_bar;
    GList *children;

    path_bar = NEMO_PATH_BAR (container);

    if (widget == path_bar->priv->up_slider_button->getPtr()) {
            nemo_path_bar_remove_1 (container, widget);
            path_bar->priv->up_slider_button->setPtr(nullptr);
            return;
    }

    if (widget == path_bar->priv->down_slider_button->getPtr()) {
            nemo_path_bar_remove_1 (container, widget);
            path_bar->priv->down_slider_button->setPtr(nullptr);
            return;
    }

    children = path_bar->priv->button_list;
    while (children) {
        if (widget == ((nemo::PathBarButton*) (children->data))->getPtr()) {
          nemo_path_bar_remove_1 (container, widget);
            path_bar->priv->button_list = g_list_remove_link (path_bar->priv->button_list, children);
            g_list_free_1 (children);
            return;
        }
        children = children->next;
    }
}

static void nemo_path_bar_forall (GtkContainer *container,
                  gboolean      include_internals,
                  GtkCallback   callback,
                  gpointer      callback_data)
{
    NemoPathBar *path_bar;
    GList *children;

    g_return_if_fail (callback != NULL);
    path_bar = NEMO_PATH_BAR (container);

    children = path_bar->priv->button_list;
    while (children) {
        GtkWidget *child;
        child = ((nemo::PathBarButton*) (children->data))->getPtr();
        children = children->next;
        (* callback) (child, callback_data);
    }

    if (path_bar->priv->up_slider_button->getPtr())
        (* callback) (path_bar->priv->up_slider_button->getPtr(), callback_data);

    if (path_bar->priv->down_slider_button->getPtr())
        (* callback) (path_bar->priv->down_slider_button->getPtr(), callback_data);
}

static void nemo_path_bar_grab_notify (GtkWidget *widget, gboolean was_grabbed)
{
    if (!was_grabbed)
        nemo_path_bar_stop_scrolling (NEMO_PATH_BAR (widget));
}

static void nemo_path_bar_state_changed (GtkWidget *widget, GtkStateType previous_state)
{
    if (!gtk_widget_get_sensitive (widget))
        nemo_path_bar_stop_scrolling (NEMO_PATH_BAR (widget));
}

static GtkWidgetPath *
nemo_path_bar_get_path_for_child (GtkContainer *container,
                    GtkWidget *child)
{
    NemoPathBar *path_bar = NEMO_PATH_BAR (container);
    GtkWidgetPath *path;

    path = gtk_widget_path_copy (gtk_widget_get_path (GTK_WIDGET (path_bar)));

    if (gtk_widget_get_visible (child) && gtk_widget_get_child_visible (child)) {
        GtkWidgetPath *sibling_path;
        GList *visible_children;
        GList *l;
        int pos;

        /* 1. Build the list of visible children, in visually left-to-right order
         * (i.e. independently of the widget's direction).  Note that our
         * button_list is stored in innermost-to-outermost path order!
         */

        visible_children = NULL;

        if (path_bar->priv->down_slider_button->get_visible() &&
            path_bar->priv->down_slider_button->get_child_visible()) {
            visible_children = g_list_prepend (visible_children, path_bar->priv->down_slider_button->getPtr());
        }

        for (l = path_bar->priv->button_list; l; l = l->next) {
            nemo::PathBarButton *data = (nemo::PathBarButton*)l->data;

            if (gtk_widget_get_visible (data->getPtr()) &&
                gtk_widget_get_child_visible (data->getPtr()))
                visible_children = g_list_prepend (visible_children, data->getPtr());
        }

        if (path_bar->priv->up_slider_button->get_visible() &&
            path_bar->priv->up_slider_button->get_child_visible()) {
            visible_children = g_list_prepend (visible_children, path_bar->priv->up_slider_button->getPtr());
        }

        if (gtk_widget_get_direction (GTK_WIDGET (path_bar)) == GTK_TEXT_DIR_RTL) {
            visible_children = g_list_reverse (visible_children);
        }

        /* 2. Find the index of the child within that list */

        pos = 0;

        for (l = visible_children; l; l = l->next) {
            GtkWidget *button = (GtkWidget*)l->data;

            if (button == child) {
                break;
            }

            pos++;
        }

        /* 3. Build the path */

        sibling_path = gtk_widget_path_new ();

        for (l = visible_children; l; l = l->next) {
            gtk_widget_path_append_for_widget (sibling_path, (GtkWidget*)l->data);
        }

        gtk_widget_path_append_with_siblings (path, sibling_path, pos);

        g_list_free (visible_children);
        gtk_widget_path_unref (sibling_path);
    } else {
        gtk_widget_path_append_for_widget (path, child);
    }

    return path;
}

static void nemo_path_bar_class_init(NemoPathBarClass *path_bar_class)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    gobject_class = (GObjectClass *) path_bar_class;
    widget_class = (GtkWidgetClass *) path_bar_class;
    container_class = (GtkContainerClass *) path_bar_class;

    gobject_class->finalize = nemo_path_bar_finalize;
    gobject_class->dispose = nemo_path_bar_dispose;

    widget_class->get_preferred_height = nemo_path_bar_get_preferred_height;
    widget_class->get_preferred_width = nemo_path_bar_get_preferred_width;
    widget_class->realize = nemo_path_bar_realize;
    widget_class->unrealize = nemo_path_bar_unrealize;
    widget_class->unmap = nemo_path_bar_unmap;
    widget_class->map = nemo_path_bar_map;
    widget_class->size_allocate = nemo_path_bar_size_allocate;
    widget_class->style_updated = nemo_path_bar_style_updated;
    widget_class->screen_changed = nemo_path_bar_screen_changed;
    widget_class->grab_notify = nemo_path_bar_grab_notify;
    widget_class->state_changed = nemo_path_bar_state_changed;
    widget_class->scroll_event = nemo_path_bar_scroll;

    container_class->add = nemo_path_bar_add;
    container_class->forall = nemo_path_bar_forall;
    container_class->remove = nemo_path_bar_remove;
    container_class->get_path_for_child = nemo_path_bar_get_path_for_child;

    path_bar_signals [PATH_CLICKED] =
        g_signal_new ("path-clicked",
        G_OBJECT_CLASS_TYPE (path_bar_class),
        G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET (NemoPathBarClass, path_clicked),
        NULL, NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        G_TYPE_FILE);
    path_bar_signals [PATH_SET] =
        g_signal_new ("path-set",
        G_OBJECT_CLASS_TYPE (path_bar_class),
        G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET (NemoPathBarClass, path_set),
        NULL, NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        G_TYPE_FILE);

     gtk_container_class_handle_border_width (container_class);
     g_type_class_add_private (path_bar_class, sizeof (NemoPathBarDetails));
}

static void nemo_path_bar_scroll_down (NemoPathBar *path_bar)
{
    GList *list;
    GList *down_button;
    GList *up_button;
    gint space_available;
    gint space_needed;
    GtkTextDirection direction;
    GtkAllocation allocation, down_button_allocation, up_button_allocation,
                  slider_allocation;

    down_button = NULL;
    up_button = NULL;

    if (path_bar->priv->ignore_click) {
        path_bar->priv->ignore_click = FALSE;
        return;
    }

    gtk_widget_queue_resize (GTK_WIDGET (path_bar));

    direction = gtk_widget_get_direction (GTK_WIDGET (path_bar));

    /* We find the button at the 'down' end, the non visible subfolder of
     * a visible folder, that we have to make visible */
    for (list = path_bar->priv->button_list; list; list = list->next) {
        if (list->next && gtk_widget_get_child_visible (((nemo::PathBarButton*) (list->next->data))->getPtr())) {
            down_button = list;
            break;
        }
    }

    if (down_button == NULL || down_button == path_bar->priv->button_list) {
	/* No Button visible or we scroll back to curren folder reset scrolling */
	path_bar->priv->scrolled_root_button = NULL;
        return;
    }

    /* Find the last visible button on the 'up' end */
    for (list = g_list_last (path_bar->priv->button_list); list; list = list->prev)
    {
        if (((nemo::PathBarButton*)list->data)->get_child_visible())
        {
            up_button = list;
            break;
        }
    }

    gtk_widget_get_allocation (((nemo::PathBarButton*) (down_button->data))->getPtr(), &down_button_allocation);
    gtk_widget_get_allocation (GTK_WIDGET (path_bar), &allocation);
    path_bar->priv->down_slider_button->get_allocation(&slider_allocation);

    space_needed = down_button_allocation.width;
    if (direction == GTK_TEXT_DIR_RTL) {
        space_available = slider_allocation.x - allocation.x;
    } else {
        space_available = (allocation.x + allocation.width) - (slider_allocation.x + slider_allocation.width);
    }

    /* We have space_available extra space that's not being used.  We
     * need space_needed space to make the button fit.  So we walk down
     * from the end, removing buttons until we get all the space we
     * need */
    while ((space_available < space_needed) && (up_button != down_button)) {
        gtk_widget_get_allocation (((nemo::PathBarButton*) (up_button->data))->getPtr(), &up_button_allocation);
        space_available += up_button_allocation.width;
        up_button = up_button->prev;
        path_bar->priv->scrolled_root_button = up_button;
    }
}

static void nemo_path_bar_scroll_up (NemoPathBar *path_bar)
{
    GList *list;

    if (path_bar->priv->ignore_click) {
        path_bar->priv->ignore_click = FALSE;
        return;
    }

    gtk_widget_queue_resize (GTK_WIDGET (path_bar));

    /* scroll in parent folder direction */
    for (list = g_list_last (path_bar->priv->button_list); list; list = list->prev) {
        if (list->prev && gtk_widget_get_child_visible (((nemo::PathBarButton*) (list->prev->data))->getPtr())) {
            if (list->prev == path_bar->priv->fake_root) {
                path_bar->priv->fake_root = NULL;
            }
            path_bar->priv->scrolled_root_button = list;
            return;
        }
    }
}

static gboolean nemo_path_bar_scroll_timeout (NemoPathBar *path_bar)
{
    gboolean retval = FALSE;

    if (path_bar->priv->timer) 
    {
        if (path_bar->priv->up_slider_button->has_focus())
            nemo_path_bar_scroll_up (path_bar);
        else if (path_bar->priv->down_slider_button->has_focus())
            nemo_path_bar_scroll_down (path_bar);

        if (path_bar->priv->need_timer) {
            path_bar->priv->need_timer = FALSE;
            g_source_remove (path_bar->priv->timer);

            path_bar->priv->timer = g_timeout_add (SCROLL_TIMEOUT,
                                   (GSourceFunc)nemo_path_bar_scroll_timeout,
                                   path_bar);

        } 
        else
            retval = TRUE;
    }

    return retval;
}

static void nemo_path_bar_stop_scrolling (NemoPathBar *path_bar)
{
    if (path_bar->priv->timer) {
        g_source_remove (path_bar->priv->timer);
        path_bar->priv->timer = 0;
        path_bar->priv->need_timer = FALSE;
    }
}

static gboolean nemo_path_bar_slider_button_press (GtkWidget       *widget,
                       GdkEventButton  *event,
                       NemoPathBar *path_bar)
{
    if (!gtk_widget_has_focus (widget)) {
        gtk_widget_grab_focus (widget);
    }

    if (event->type != GDK_BUTTON_PRESS || event->button != 1) {
        return FALSE;
    }

    path_bar->priv->ignore_click = FALSE;

    if (widget == path_bar->priv->up_slider_button->getPtr()) {
        nemo_path_bar_scroll_up (path_bar);
    } else if (widget == path_bar->priv->down_slider_button->getPtr()) {
        nemo_path_bar_scroll_down (path_bar);
    }

    if (!path_bar->priv->timer) {
        path_bar->priv->need_timer = TRUE;
        path_bar->priv->timer = g_timeout_add (INITIAL_SCROLL_TIMEOUT,
                         (GSourceFunc)nemo_path_bar_scroll_timeout,
                             path_bar);
    }

    return FALSE;
}

static gboolean nemo_path_bar_slider_button_release (GtkWidget      *widget,
                         GdkEventButton *event,
                         NemoPathBar     *path_bar)
{
    if (event->type != GDK_BUTTON_RELEASE) {
        return FALSE;
    }

    path_bar->priv->ignore_click = TRUE;
    nemo_path_bar_stop_scrolling (path_bar);

    return FALSE;
}


/* Changes the icons wherever it is needed */
static void reload_icons (NemoPathBar *path_bar)
{
    GList *list;

    for (list = path_bar->priv->button_list; list; list = list->next) {
        nemo::PathBarButton *button_data;
        button_data = (nemo::PathBarButton*) (list->data);
        if (button_data->type != nemo::ButtonType::Normal || button_data->is_base_dir) {
            button_data->update_button_appearance ();
        }
    }
}

/* Callback used when a GtkSettings value changes */
static void settings_notify_cb (GObject    *object,
            GParamSpec *pspec,
            NemoPathBar *path_bar)
{
    const char *name;

    name = g_param_spec_get_name (pspec);

     if (! strcmp (name, "gtk-icon-theme-name") || ! strcmp (name, "gtk-icon-sizes")) {
        reload_icons (path_bar);
    }
}

static void nemo_path_bar_check_icon_theme (NemoPathBar *path_bar)
{
    GtkSettings *settings;

    if (path_bar->priv->settings_signal_id) {
        return;
    }

    settings = gtk_settings_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (path_bar)));
    path_bar->priv->settings_signal_id = g_signal_connect (settings, "notify", G_CALLBACK (settings_notify_cb), path_bar);

    reload_icons (path_bar);
}

/* Public functions and their helpers */
void nemo_path_bar_clear_buttons (NemoPathBar *path_bar)
{
    while (path_bar->priv->button_list != NULL)
    {
        nemo::PathBarButton* button = (nemo::PathBarButton*) (path_bar->priv->button_list->data);
        gtk_container_remove (GTK_CONTAINER (path_bar), button->getPtr());
        delete button;
    }
    path_bar->priv->scrolled_root_button = NULL;
    path_bar->priv->fake_root = NULL;
}

static void button_clicked_cb (GtkWidget *button, gpointer data)
{
    nemo::PathBarButton *button_data;
    NemoPathBar *path_bar;
    GList *button_list;

    button_data = (nemo::PathBarButton*) (data);
    if (button_data->ignore_changes) {
        return;
    }

    path_bar = NEMO_PATH_BAR (gtk_widget_get_parent (button));

    button_list = g_list_find (path_bar->priv->button_list, button_data);
    g_assert (button_list != NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

    g_signal_emit (path_bar, path_bar_signals [PATH_CLICKED], 0, button_data->path);
}

void button_data_free(nemo::PathBarButton *button_data)
{
    delete button_data;
}

static nemo::PathBarButton * make_directory_button (NemoPathBar *path_bar, NemoFile *file,  gboolean current_dir, gboolean base_dir)
{
     nemo::PathBarButton* button_data = new nemo::PathBarButton(file, current_dir, base_dir, desktop_is_home);
     g_signal_connect (button_data->getPtr(), "clicked", G_CALLBACK (button_clicked_cb), button_data);
     //g_object_weak_ref (G_OBJECT (button_data->getPtr()), (GWeakNotify) button_data_free, button_data);
     button_data->file_changed_signal_id = g_signal_connect (button_data->file, "changed", G_CALLBACK (button_data_file_changed), button_data);
     return button_data;
}

static gboolean
nemo_path_bar_check_parent_path (NemoPathBar *path_bar,
                     GFile *location,
                     nemo::PathBarButton **current_button_data)
{
    GList *list;
    GList *current_path;
    gboolean need_new_fake_root;

    current_path = NULL;
    need_new_fake_root = FALSE;

    if (current_button_data) {
        *current_button_data = NULL;
    }

    for (list = path_bar->priv->button_list; list; list = list->next) {
        nemo::PathBarButton *button_data;

        button_data = (nemo::PathBarButton*)list->data;
        if (g_file_equal (location, button_data->path)) {
            current_path = list;

            if (current_button_data) {
                *current_button_data = button_data;
            }
            break;
        }
        if (list == path_bar->priv->fake_root) {
            need_new_fake_root = TRUE;
        }
    }

    if (current_path) {

        if (need_new_fake_root) {
            path_bar->priv->fake_root = NULL;
            for (list = current_path; list; list = list->next) {
                nemo::PathBarButton *button_data;

                button_data = (nemo::PathBarButton*)list->data;
                if (list->prev != NULL && button_data->fake_root) {
                    path_bar->priv->fake_root = list;
                    break;
                }
            }
        }

        for (list = path_bar->priv->button_list; list; list = list->next) 
        {
            ((nemo::PathBarButton*)(list->data))->update_button_state(
                                   (list == current_path) ? TRUE : FALSE);
        }

        if (!gtk_widget_get_child_visible (((nemo::PathBarButton*) (current_path->data))->getPtr())) {
            path_bar->priv->scrolled_root_button = current_path;
            gtk_widget_queue_resize (GTK_WIDGET (path_bar));
        }
        return TRUE;
    }
    return FALSE;
}

static gboolean
nemo_path_bar_update_path (NemoPathBar *path_bar,
                   GFile *file_path,
                   gboolean emit_signal)
{
    NemoFile *file, *parent_file;
    gboolean first_directory, last_directory;
    gboolean result;
    GList *new_buttons, *l, *fake_root;
    nemo::PathBarButton *button_data, *current_button_data;

    g_return_val_if_fail (NEMO_IS_PATH_BAR (path_bar), FALSE);
    g_return_val_if_fail (file_path != NULL, FALSE);

    fake_root = NULL;
    result = TRUE;
    first_directory = TRUE;
    new_buttons = NULL;
    current_button_data = NULL;

    file = nemo_file_get (file_path);

    gtk_widget_push_composite_child ();

    while (file != NULL) {
        parent_file = nemo_file_get_parent (file);
        last_directory = !parent_file;
        button_data = make_directory_button (path_bar, file, first_directory, last_directory);
        nemo_file_unref (file);

        if (first_directory) {
            current_button_data = button_data;
        }

        new_buttons = g_list_prepend (new_buttons, button_data);

        if (parent_file != NULL && button_data->fake_root) {
            fake_root = new_buttons;
        }

        file = parent_file;
        first_directory = FALSE;
    }

    nemo_path_bar_clear_buttons (path_bar);
    path_bar->priv->button_list = g_list_reverse (new_buttons);
    path_bar->priv->fake_root = fake_root;

    for (l = path_bar->priv->button_list; l; l = l->next)
    {
        GtkWidget *button = ((nemo::PathBarButton*) (l->data))->getPtr();
        gtk_container_add (GTK_CONTAINER (path_bar), button);
    }

    gtk_widget_pop_composite_child ();

    child_ordering_changed (path_bar);

    if (path_bar->priv->current_path != NULL) {
        g_object_unref (path_bar->priv->current_path);
    }

    path_bar->priv->current_path = (GFile*)g_object_ref (file_path);
    path_bar->priv->current_button_data = current_button_data;

    g_signal_emit (path_bar, path_bar_signals [PATH_SET], 0, file_path);

    return result;
}

gboolean
nemo_path_bar_set_path (NemoPathBar *path_bar, GFile *file_path)
{
    nemo::PathBarButton *button_data;

    g_return_val_if_fail (NEMO_IS_PATH_BAR (path_bar), FALSE);
    g_return_val_if_fail (file_path != NULL, FALSE);

        /* Check whether the new path is already present in the pathbar as buttons.
         * This could be a parent directory or a previous selected subdirectory. */
    if (nemo_path_bar_check_parent_path (path_bar, file_path, &button_data)) {
        if (path_bar->priv->current_path != NULL) {
            g_object_unref (path_bar->priv->current_path);
        }

        path_bar->priv->current_path = (GFile*)g_object_ref (file_path);
        path_bar->priv->current_button_data = button_data;

        return TRUE;
    }

    return nemo_path_bar_update_path (path_bar, file_path, TRUE);
}

GFile *
nemo_path_bar_get_path_for_button (NemoPathBar *path_bar,
                       GtkWidget       *button)
{
    GList *list;

    g_return_val_if_fail (NEMO_IS_PATH_BAR (path_bar), NULL);
    g_return_val_if_fail (GTK_IS_BUTTON (button), NULL);

    for (list = path_bar->priv->button_list; list; list = list->next) {
        nemo::PathBarButton *button_data;
        button_data = (nemo::PathBarButton*) (list->data);
        if (button_data->getPtr() == button) {
            return (GFile*)g_object_ref (button_data->path);
        }
    }

    return NULL;
}

