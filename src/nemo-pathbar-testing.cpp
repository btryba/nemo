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
    #include <gtk/gtk.h>
    #include <glib/gi18n.h>
    ////////#include <math.h>

    #include "nemo-pathbar.h"

    #include <eel/eel-vfs-extensions.h>

    //#include <libnemo-private/nemo-file.h>
    #include <libnemo-private/nemo-file-utilities.h>
    #include <libnemo-private/nemo-global-preferences.h>
    ////////#include <libnemo-private/nemo-icon-names.h>
    //#include <libnemo-private/nemo-icon-dnd.h>

    #include "nemo-window.h"
    #include "nemo-window-private.h"
    #include "nemo-window-slot.h"
    #include "nemo-window-slot-dnd.h"
}

#include "gtk_container.hpp"
#include "gtk_button.hpp"
#include "gtk_image.hpp"
#include "gtk_file.hpp"
#include "gtk_settings.hpp"
#include <cstdint>
#include "nemo-pathbar.hpp"

enum {
    PATH_CLICKED,
    PATH_SET,
    LAST_SIGNAL
};

const int SCROLL_TIMEOUT = 150;
const int INITIAL_SCROLL_TIMEOUT = 300;

static unsigned int path_bar_signals [LAST_SIGNAL] = { 0 };

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
};

namespace nemo
{
    PathBar::PathBar(GtkContainer* cClass) :
        Container(cClass, true),
        home_path{g_get_home_dir ()},
        root_path{"/"}
    {
        widget = (GtkWidget*)cClass;
        set_has_window(false);
        set_redraw_on_allocate(false);

        up_slider_button = get_slider_button(GTK_POS_LEFT);
        up_slider_button->connect(gtk::ButtonEventType::Clicked, &nemo_path_bar_scroll_up, widget, false);

        down_slider_button = get_slider_button(GTK_POS_RIGHT);
        down_slider_button->connect(gtk::ButtonEventType::Clicked, &nemo_path_bar_scroll_down, widget, false);
        
        char* p = nemo_get_desktop_directory ();
        desktop_path = new gtk::File(p);
        g_free (p);
        
        desktop_is_home = (home_path == *desktop_path);

        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_DESKTOP_IS_HOME_DIR,
                G_CALLBACK(desktop_location_changed_callback),
                widget);

        get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);
        get_style_context()->add_class("path-bar");
    }

    PathBar::~PathBar()
    {
        delete desktop_path;

        delete up_slider_button;
        delete down_slider_button;
    }

    gtk::Button* PathBar::get_slider_button(GtkPositionType position)
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
        add(*button);
        button->show_all();

        gtk_widget_pop_composite_child();

        button->connect(gtk::ButtonEventType::ButtonPressed, &nemo_path_bar_slider_button_press, widget);
        button->connect(gtk::ButtonEventType::ButtonReleased, &nemo_path_bar_slider_button_release, widget);
        button->drag_dest_set();
        button->drag_dest_set_track_motion(true);
        button->connect(gtk::ButtonEventType::DragMotion, &nemo_path_bar_slider_drag_motion, widget);
        button->connect(gtk::ButtonEventType::DragLeave, &nemo_path_bar_slider_drag_leave, widget);

        return button;
    }

    void PathBar::update_button_types()
    {
        gtk::File path{};
        for(PathBarButton* button : button_list)
            if(button->get_active())
            {
                path.updatePath(button->path);
                break;
            }

        if (!path.isNull())
            update_path (path, true);
    }

    bool PathBar::update_path (gtk::File& file_path, bool emit_signal)
    {
        GList *fake_root = nullptr;
        bool result = true;
        bool first_directory = true;
        GList *new_buttons = nullptr;

        NemoFile *file = nemo_file_get (file_path.getPtr());

        gtk_widget_push_composite_child ();

        while (file != nullptr)
        {
            NemoFile *parent_file = nemo_file_get_parent (file);
            bool last_directory = !parent_file;
            
            PathBarButton *button = new PathBarButton{*file};
            //PathBarButton *button = make_directory_button(*file, first_directory, last_directory);
            nemo_file_unref (file);

            if (first_directory)
                current_button_data = button;

            new_buttons = g_list_prepend (new_buttons, button);

            if (parent_file != nullptr && button->fake_root)
                fake_root = new_buttons;

            file = parent_file;
            first_directory = false;
        }

        clear_buttons();
        button_list = g_list_reverse (new_buttons);
        this->fake_root = fake_root;

        for(PathBarButton* button : button_list)
            add(*button);

        gtk_widget_pop_composite_child ();

        child_ordering_changed();

        current_path.updatePath(file_path);

        g_signal_emit (widget, path_bar_signals [PATH_SET], 0, file_path);

        return result;
    }

    void PathBar::child_ordering_changed()
    {
        if (!up_slider_button->isNull())
            up_slider_button->get_style_context()->invalidate();

        if (!down_slider_button->isNull())
            down_slider_button->get_style_context()->invalidate();

        for(PathBarButton* button : button_list)
            button->get_style_context()->invalidate();
    }

    void PathBar::clear_buttons()
    {
        for(PathBarButton* button : button_list)
        {
            remove(*button);
            delete button;
        }
        button_list.clear();
        scrolled_root_button.clear();
        fake_root.clear();
    }

    void PathBar::desktop_location_changed_callback()
    {
        desktop_path->updatePath(nemo_get_desktop_location());
        home_path.updatePath(g_get_home_dir());
        desktop_is_home = (home_path == *desktop_path);

        update_button_types();
    }

    bool PathBar::slider_timeout()
    {
        drag_slider_timeout = 0;

        if(get_visible())
        {
            if (drag_slider_timeout_for_up_button)
                nemo_path_bar_scroll_up();
            else
                nemo_path_bar_scroll_down();
        }

        return false;
    }

    void PathBar::slider_drag_motion (GtkWidget *widget, GdkDragContext *context, int x, int y, unsigned int time)
    {
        if(drag_slider_timeout == 0)
        {
            GtkSettings *settings = gtk_widget_get_settings (widget);

            unsigned int timeout;
            g_object_get (settings, "gtk-timeout-expand", &timeout, nullptr);
            drag_slider_timeout = g_timeout_add (timeout, &PathBar::slider_timeout, widget);

            drag_slider_timeout_for_up_button = up_slider_button->ptrAddressesMatch(widget);
        }
    }

    void PathBar::get_preferred_width(int& minimum, int& natural)
    {
        minimum = natural = 0;

        for(PathBarButton* button : button_list)
        {
            int child_min, child_nat;
            button->get_preferred_width(child_min, child_nat);
            minimum = MAX (minimum, child_min);
            natural = MAX (natural, child_nat);
        }
        down_slider_button->get_preferred_width(slider_width);

        minimum += slider_width * 2;
        natural += slider_width * 2;
    } 

    void PathBar::get_preferred_height(int& minimum, int& natural)
    {
        int child_min, child_nat;

        minimum = natural = 0;

        for(PathBarButton* button : button_list)
        {
            button->get_preferred_height(child_min, child_nat);

            minimum = MAX (minimum, child_min);
            natural = MAX (natural, child_nat);
        }
    }

    void PathBar::update_slider_buttons()
    {
        if(button_list.size() > 0)
        {
            PathBarButton*button = button_list[0];
            if (button->get_child_visible())
            {
                down_slider_button->set_sensitive(false);
                stop_scrolling();
            } 
            else
                down_slider_button->set_sensitive(true);

            button = button_list[button_list.size()-1];
            if (button->get_child_visible())
            {
                up_slider_button->set_sensitive(false);
                stop_scrolling();
            }
            else
                up_slider_button->set_sensitive(true);
        }
    }

    void PathBar::stop_scrolling()
    {
        if (timer)
        {
            g_source_remove(timer);
            timer = 0;
            need_timer = false;
        }
    }

    void PathBar::check_icon_theme()
    {
        if (settings_signal_id)
            return;

        gtk::Settings settings = getSettings();
        settings_signal_id = settings.connect("notify",G_CALLBACK(settings_notify_cb), *this);

        reload_icons();
    }

    void PathBar::reload_icons()
    {
        for(PathBarButton* button : button_list)
            if (button->type != ButtonType::Normal || button->is_base_dir)
                button->update_button_appearance();
    }

    bool PathBar::check_parent_path(gtk::File& location, ButtonData **current_button_data)
    {
        GList *current_path = nullptr;
        bool need_new_fake_root = false;

        if (current_button_data)
            *current_button_data = nullptr;

        for(PathBarButton* button : button_list)
        {
            if (location == button->path)
            {
                current_path = list;

                if (current_button_data)
                    current_button_data = button;
                break;
            }
            if (list == fake_root)
                need_new_fake_root = true;
        }

        if (current_path) 
        {
            if (need_new_fake_root)
            {
                fake_root = nullptr;
                for (GList *list = current_path; list; list = list->next) 
                {
                    PathBarButton* button = (PathBarButton*)list->data;
                    if (list->prev != nullptr && button->fake_root)
                    {
                        fake_root = list;
                        break;
                    }
                }
            }

            for (list = path_bar->priv->button_list; list; list = list->next) {
                nemo_path_bar_update_button_state (BUTTON_DATA (list->data),
                                    (list == current_path) ? TRUE : FALSE);
            }

            if (!gtk_widget_get_child_visible (BUTTON_DATA (current_path->data)->button)) {
                path_bar->priv->scrolled_root_button = current_path;
                gtk_widget_queue_resize (GTK_WIDGET (path_bar));
            }
            return TRUE;
        }
        return FALSE;
    }
}

G_DEFINE_TYPE (NemoPathBar, nemo_path_bar, GTK_TYPE_CONTAINER);

static nemo::PathBar& getCppObject(void* obj)
{
    return *((nemo::PathBar*)((NemoPathBar*)obj)->cppParent);
}

static void desktop_location_changed_callback (gpointer user_data)
{
    NemoPathBar *path_bar = NEMO_PATH_BAR (user_data);

    getCppObject(path_bar).desktop_location_changed_callback();
}

static gboolean slider_timeout (gpointer user_data)
{
    return getCppObject(user_data).slider_timeout();
}

static void nemo_path_bar_slider_drag_motion (GtkWidget *widget, GdkDragContext *context, int x, int y, unsigned int time, gpointer user_data)
{
    getCppObject(user_data).slider_drag_motion(widget, context, x, y, time);
}

static void nemo_path_bar_slider_drag_leave (GtkWidget *widget, GdkDragContext *context, unsigned int time, gpointer user_data)
{
    NemoPathBar *path_bar = NEMO_PATH_BAR (user_data);

    if (path_bar->priv->drag_slider_timeout != 0)
    {
        g_source_remove (path_bar->priv->drag_slider_timeout);
        path_bar->priv->drag_slider_timeout = 0;
    }
}

/**
 * Utility function. Return a GFile for the "special directory" if it exists, or NULL
 * Ripped from nemo-file.c (nemo_file_is_user_special_directory) and slightly modified
 */

static void nemo_path_bar_init (NemoPathBar *path_bar)
{
    nemo::PathBar* cppClass = new nemo::PathBar((GtkContainer*)path_bar);
    path_bar->cppParent = cppClass;

    path_bar->priv = G_TYPE_INSTANCE_GET_PRIVATE (path_bar, NEMO_TYPE_PATH_BAR, NemoPathBarDetails);
}

static void nemo_path_bar_finalize (GObject *object)
{
    NemoPathBar *path_bar = NEMO_PATH_BAR (object);

    nemo_path_bar_stop_scrolling (path_bar);

    if (path_bar->priv->drag_slider_timeout != 0) {
        g_source_remove (path_bar->priv->drag_slider_timeout);
        path_bar->priv->drag_slider_timeout = 0;
    }

    g_list_free (path_bar->priv->button_list);
    delete getCppObject(path_bar);

    g_signal_handlers_disconnect_by_func (nemo_preferences,
                          (gpointer)desktop_location_changed_callback,
                          path_bar);

    G_OBJECT_CLASS (nemo_path_bar_parent_class)->finalize (object);
}

/* Removes the settings signal handler.  It's safe to call multiple times */
static void remove_settings_signal (NemoPathBar *path_bar, GdkScreen  *screen)
{
    if (path_bar->priv->settings_signal_id)
    {
        GtkSettings *settings = gtk_settings_get_for_screen (screen);
        g_signal_handler_disconnect (settings,
                            path_bar->priv->settings_signal_id);
        path_bar->priv->settings_signal_id = 0;
    }
}

static void
nemo_path_bar_dispose (GObject *object)
{
    remove_settings_signal (NEMO_PATH_BAR (object), gtk_widget_get_screen (GTK_WIDGET (object)));

    delete getCppObject(object);

    G_OBJECT_CLASS (nemo_path_bar_parent_class)->dispose (object);
}

/* Size requisition:
 *
 * Ideally, our size is determined by another widget, and we are just filling
 * available space.
 */

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

/* This is a tad complicated */
static void nemo_path_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    GtkWidget* child;
    GtkAllocation child_allocation;
    GList *list, *pathbar_root_button;
    int width_min;
    int largest_width;
    GtkRequisition child_requisition;
    GtkRequisition child_requisition_min;
    
    bool needs_reorder = false;
    int button_count = 0;

    bool need_sliders = false;
    int up_slider_offset = 0;
    int down_slider_offset = 0;
    NemoPathBar *path_bar = NEMO_PATH_BAR (widget);

    gtk_widget_set_allocation (widget, allocation);

    if (gtk_widget_get_realized (widget))
    {
        gdk_window_move_resize (path_bar->priv->event_window,
                    allocation->x, allocation->y,
                    allocation->width, allocation->height);
    }

    /* No path is set so we don't have to allocate anything. */
    if (path_bar->priv->button_list == nullptr)
        return;

    GtkTextDirection direction = gtk_widget_get_direction (widget);

    getCppObject(path_bar)->up_slider_button->get_preferred_width(path_bar->priv->slider_width);

    gtk_widget_get_preferred_size (BUTTON_DATA (path_bar->priv->button_list->data)->button,
                       nullptr, &child_requisition);
    int width = child_requisition.width;

    for (list = path_bar->priv->button_list->next; list; list = list->next)
    {
        GtkWidget *child = BUTTON_DATA (list->data)->button;
        gtk_widget_get_preferred_size (child, nullptr, &child_requisition);
        width += child_requisition.width;

        if (list == path_bar->priv->fake_root)
        {
            width += path_bar->priv->slider_width;
            break;
        }
    }

    largest_width = allocation->width;

    if (width <= allocation->width && !need_sliders)
    {
        if (path_bar->priv->fake_root)
            pathbar_root_button = path_bar->priv->fake_root;
        else
            pathbar_root_button = g_list_last (path_bar->priv->button_list);
    }
    else 
    {
        bool reached_end = false;
		need_sliders = true;
        int slider_space = 2 * path_bar->priv->slider_width;
        largest_width -= slider_space;

        /* To see how much space we have, and how many buttons we can display.
         * We start at the first button, count forward until hit the new
         * button, then count backwards.
         */

        /* First assume, we can only display one button */
        if (path_bar->priv->scrolled_root_button)
            pathbar_root_button = path_bar->priv->scrolled_root_button;
        else
            pathbar_root_button = path_bar->priv->button_list;

        /* Count down the path chain towards the end. */
        gtk_widget_get_preferred_size (BUTTON_DATA (pathbar_root_button->data)->button,
                                       NULL, &child_requisition);
        button_count = 1;
        width = child_requisition.width;

        /* Count down the path chain towards the end. */
        list = pathbar_root_button->prev;
        
        while (list && !reached_end)
        {
            if (list == path_bar->priv->fake_root)
                break;

            child = BUTTON_DATA (list->data)->button;
            gtk_widget_get_preferred_size (child, NULL, &child_requisition);

            if (width + child_requisition.width + slider_space > allocation->width) {
                reached_end = TRUE;
                if (button_count == 1) {
                    /* Display two Buttons if they fit shrinked */
                    gtk_widget_get_preferred_size (child, &child_requisition_min, NULL);
                    width_min = child_requisition_min.width;
                    gtk_widget_get_preferred_size (BUTTON_DATA (pathbar_root_button->data)->button, &child_requisition_min, NULL);
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
        while (pathbar_root_button->next && ! reached_end)
        {
            if (pathbar_root_button == path_bar->priv->fake_root)
                break;

            child = BUTTON_DATA (pathbar_root_button->next->data)->button;
            gtk_widget_get_preferred_size (child, nullptr, &child_requisition);

            if (width + child_requisition.width + slider_space > allocation->width)
            {
                reached_end = true;
                if (button_count == 1)
                {
            	    gtk_widget_get_preferred_size (child, &child_requisition_min, nullptr);
            	    width_min = child_requisition_min.width;
            	    gtk_widget_get_preferred_size (BUTTON_DATA (pathbar_root_button->data)->button, &child_requisition_min, NULL);
            	    width_min += child_requisition_min.width;
            	    if (width_min <= largest_width)
                    {
                        // Two shinked buttons fit
                        pathbar_root_button = pathbar_root_button->next;
                        button_count++;
                        largest_width /= 2;
                        if (width < largest_width)
                        {
                            /* unused space for second button */
                            largest_width += largest_width - width;
                        } 
                        else if (child_requisition.width < largest_width)
                        {
                            /* unused space for first button */
                            largest_width += largest_width - child_requisition.width;
                        }
            	    }
                }
            } 
            else
            {
                width += child_requisition.width;
                pathbar_root_button = pathbar_root_button->next;
                button_count++;
            }
        }
    }

    /* Now, we allocate space to the buttons */
    child_allocation.y = allocation->y;
    child_allocation.height = allocation->height;

    if (direction == GTK_TEXT_DIR_RTL)
    {
        child_allocation.x = allocation->x + allocation->width;
        if (need_sliders || path_bar->priv->fake_root)
        {
            child_allocation.x -= (path_bar->priv->slider_width);
            up_slider_offset = allocation->width - path_bar->priv->slider_width;
        }
    } 
    else 
    {
        child_allocation.x = allocation->x;
        if (need_sliders || path_bar->priv->fake_root) 
        {
            up_slider_offset = 0;
            child_allocation.x += path_bar->priv->slider_width;
        }
    }

    for (list = pathbar_root_button; list; list = list->prev)
    {
        GtkWidget* child = BUTTON_DATA (list->data)->button;
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
        child = BUTTON_DATA (list->data)->button;
        needs_reorder |= gtk_widget_get_child_visible (child) == TRUE;
        gtk_widget_set_child_visible (child, FALSE);
        list = list->prev;
    }

    if (BUTTON_DATA (pathbar_root_button->data)->fake_root) {
        path_bar->priv->fake_root = pathbar_root_button;
    }

    for (list = pathbar_root_button->next; list; list = list->next) {
        child = BUTTON_DATA (list->data)->button;
        needs_reorder |= gtk_widget_get_child_visible (child) == TRUE;
        gtk_widget_set_child_visible (child, FALSE);
        if (BUTTON_DATA (list->data)->fake_root) {
            path_bar->priv->fake_root = list;
        }
    }

    if (need_sliders || path_bar->priv->fake_root) {
        child_allocation.width = path_bar->priv->slider_width;
        child_allocation.x = up_slider_offset + allocation->x;
        getCppObject(path_bar)->up_slider_button->size_allocate(&child_allocation);

        needs_reorder |= getCppObject(path_bar)->up_slider_button->get_child_visible() == false;
        getCppObject(path_bar)->up_slider_button->set_child_visible(true);
        getCppObject(path_bar)->up_slider_button->show_all();

        if (direction == GTK_TEXT_DIR_LTR) {
            down_slider_offset += path_bar->priv->slider_width;
        }
    } else {
        needs_reorder |= getCppObject(path_bar)->up_slider_button->get_child_visible() == true;
        getCppObject(path_bar)->up_slider_button->set_child_visible(false);
    }

    if (need_sliders) {
        child_allocation.width = path_bar->priv->slider_width;
        child_allocation.x = down_slider_offset + allocation->x;
        getCppObject(path_bar)->down_slider_button->size_allocate(&child_allocation);

        needs_reorder |= getCppObject(path_bar)->up_slider_button->get_child_visible() == false;
        getCppObject(path_bar)->down_slider_button->set_child_visible(true);
        getCppObject(path_bar)->down_slider_button->show_all();
        nemo_path_bar_update_slider_buttons (path_bar);
    } else {
        needs_reorder |= getCppObject(path_bar)->up_slider_button->get_child_visible() == true;
        getCppObject(path_bar)->down_slider_button->set_child_visible(false);
        /* Reset Scrolling to have the left most folder in focus when resizing again */
        path_bar->priv->scrolled_root_button = NULL;
    }

    if (needs_reorder) {
        child_ordering_changed (path_bar);
    }
}

static void
nemo_path_bar_style_updated (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (nemo_path_bar_parent_class)->style_updated (widget);

    nemo_path_bar_check_icon_theme (NEMO_PATH_BAR (widget));
}

static void
nemo_path_bar_screen_changed (GtkWidget *widget,
                      GdkScreen *previous_screen)
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

static gboolean
nemo_path_bar_scroll (GtkWidget      *widget,
              GdkEventScroll *event)
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

static void
nemo_path_bar_realize (GtkWidget *widget)
{
    NemoPathBar *path_bar;
    GtkAllocation allocation;
    GdkWindow *window;
    GdkWindowAttr attributes;
    int attributes_mask;

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

static void
nemo_path_bar_unrealize (GtkWidget *widget)
{
    NemoPathBar *path_bar;

    path_bar = NEMO_PATH_BAR (widget);

    gdk_window_set_user_data (path_bar->priv->event_window, NULL);
    gdk_window_destroy (path_bar->priv->event_window);
    path_bar->priv->event_window = NULL;

    GTK_WIDGET_CLASS (nemo_path_bar_parent_class)->unrealize (widget);
}

static void
nemo_path_bar_add (GtkContainer *container,
               GtkWidget    *widget)
{
    gtk_widget_set_parent (widget, GTK_WIDGET (container));
}

static void
nemo_path_bar_remove_1 (GtkContainer *container,
                    GtkWidget    *widget)
{
    gboolean was_visible = gtk_widget_get_visible (widget);
    gtk_widget_unparent (widget);
    if (was_visible) {
        gtk_widget_queue_resize (GTK_WIDGET (container));
    }
}

static void
nemo_path_bar_remove (GtkContainer *container,
                  GtkWidget    *widget)
{
    NemoPathBar *path_bar;
    GList *children;

    path_bar = NEMO_PATH_BAR (container);

    if (getCppObject(path_bar)->up_slider_button->ptrAddressesMatch(widget)) {
            nemo_path_bar_remove_1 (container, widget);
            getCppObject(path_bar)->up_slider_button->setPtr(nullptr);
            return;
    }

    if (getCppObject(path_bar)->down_slider_button->ptrAddressesMatch(widget)) {
            nemo_path_bar_remove_1 (container, widget);
            getCppObject(path_bar)->down_slider_button->setPtr(nullptr);
            return;
    }

    children = path_bar->priv->button_list;
    while (children) {
        if (widget == BUTTON_DATA (children->data)->button) {
          nemo_path_bar_remove_1 (container, widget);
            path_bar->priv->button_list = g_list_remove_link (path_bar->priv->button_list, children);
            g_list_free_1 (children);
            return;
        }
        children = children->next;
    }
}

static void
nemo_path_bar_forall (GtkContainer *container,
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
        child = BUTTON_DATA (children->data)->button;
        children = children->next;
        (* callback) (child, callback_data);
    }

    if (getCppObject(path_bar)->up_slider_button->getPtr())
        (* callback) (getCppObject(path_bar)->up_slider_button->getPtr(), callback_data);

    if (getCppObject(path_bar)->down_slider_button->getPtr())
        (* callback) (getCppObject(path_bar)->down_slider_button->getPtr(), callback_data);
}

static void
nemo_path_bar_grab_notify (GtkWidget *widget,
                   gboolean   was_grabbed)
{
    if (!was_grabbed) {
        nemo_path_bar_stop_scrolling (NEMO_PATH_BAR (widget));
    }
}

static void
nemo_path_bar_state_changed (GtkWidget    *widget,
                     GtkStateType  previous_state)
{
    if (!gtk_widget_get_sensitive (widget)) {
        nemo_path_bar_stop_scrolling (NEMO_PATH_BAR (widget));
    }
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

        if (getCppObject(path_bar)->down_slider_button->get_visible() &&
            getCppObject(path_bar)->down_slider_button->get_child_visible()) {
            visible_children = g_list_prepend (visible_children, getCppObject(path_bar)->down_slider_button->getPtr());
        }

        for (l = path_bar->priv->button_list; l; l = l->next) {
            ButtonData *data = (ButtonData*)l->data;

            if (gtk_widget_get_visible (data->button) &&
                gtk_widget_get_child_visible (data->button))
                visible_children = g_list_prepend (visible_children, data->button);
        }

        if (getCppObject(path_bar)->up_slider_button->get_visible() &&
            getCppObject(path_bar)->up_slider_button->get_child_visible()) {
            visible_children = g_list_prepend (visible_children, getCppObject(path_bar)->up_slider_button->getPtr());
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

static void
nemo_path_bar_class_init (NemoPathBarClass *path_bar_class)
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

static void
nemo_path_bar_scroll_down (NemoPathBar *path_bar)
{
    GList *list;
    GList *down_button;
    GList *up_button;
    int space_available;
    int space_needed;
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
        if (list->next && gtk_widget_get_child_visible (BUTTON_DATA (list->next->data)->button)) {
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
    for (list = g_list_last (path_bar->priv->button_list); list; list = list->prev) {
        if (gtk_widget_get_child_visible (BUTTON_DATA (list->data)->button)) {
            up_button = list;
            break;
        }
    }

    gtk_widget_get_allocation (BUTTON_DATA (down_button->data)->button, &down_button_allocation);
    gtk_widget_get_allocation (GTK_WIDGET (path_bar), &allocation);
    getCppObject(path_bar)->down_slider_button->get_allocation(&slider_allocation);

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
        gtk_widget_get_allocation (BUTTON_DATA (up_button->data)->button, &up_button_allocation);
        space_available += up_button_allocation.width;
        up_button = up_button->prev;
        path_bar->priv->scrolled_root_button = up_button;
    }
}

static void
nemo_path_bar_scroll_up (NemoPathBar *path_bar)
{
    GList *list;

    if (path_bar->priv->ignore_click) {
        path_bar->priv->ignore_click = FALSE;
        return;
    }

    gtk_widget_queue_resize (GTK_WIDGET (path_bar));

    /* scroll in parent folder direction */
    for (list = g_list_last (path_bar->priv->button_list); list; list = list->prev) {
        if (list->prev && gtk_widget_get_child_visible (BUTTON_DATA (list->prev->data)->button)) {
            if (list->prev == path_bar->priv->fake_root) {
                path_bar->priv->fake_root = NULL;
            }
            path_bar->priv->scrolled_root_button = list;
            return;
        }
    }
}

static gboolean
nemo_path_bar_scroll_timeout (NemoPathBar *path_bar)
{
    gboolean retval = FALSE;

    if (path_bar->priv->timer) 
    {
        if (getCppObject(path_bar)->up_slider_button->has_focus())
            nemo_path_bar_scroll_up (path_bar);
        else if (getCppObject(path_bar)->down_slider_button->has_focus())
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

static gboolean
nemo_path_bar_slider_button_press (GtkWidget       *widget,
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

    if (getCppObject(path_bar)->up_slider_button->ptrAddressesMatch(widget)) {
        nemo_path_bar_scroll_up (path_bar);
    } else if (getCppObject(path_bar)->down_slider_button->ptrAddressesMatch(widget)) {
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

static gboolean
nemo_path_bar_slider_button_release (GtkWidget      *widget,
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


/* Callback used when a GtkSettings value changes */
static void
settings_notify_cb (GObject    *object,
            GParamSpec *pspec,
            NemoPathBar *path_bar)
{
    const char *name;

    name = g_param_spec_get_name (pspec);

     if (! strcmp (name, "gtk-icon-theme-name") || ! strcmp (name, "gtk-icon-sizes")) {
        reload_icons (path_bar);
    }
}

/* Public functions and their helpers */
void nemo_path_bar_clear_buttons (NemoPathBar *path_bar)
{
    getCppObject(path_bar)->clear_buttons();
}

GtkWidget* nemo_path_bar_new()
{
    NemoPathBar* cClass = (NemoPathBar*)g_object_new (NEMO_TYPE_PATH_BAR, nullptr); //calls init
    return (GtkWidget*)cClass;
}

gboolean nemo_path_bar_set_path (NemoPathBar *path_bar, GFile *file_path)
{
    ButtonData *button_data;

    /* Check whether the new path is already present in the pathbar as buttons.
        * This could be a parent directory or a previous selected subdirectory. */
    if (nemo_path_bar_check_parent_path (path_bar, file_path, &button_data)) 
    {
        getCppObject(path_bar)->current_path->updatePath(file_path);
        path_bar->priv->current_button_data = button_data;

        return TRUE;
    }

    return nemo_path_bar_update_path (path_bar, file_path, TRUE);
}

GFile * nemo_path_bar_get_path_for_button (NemoPathBar *path_bar, GtkWidget *button)
{
    GList *list;

    for (list = path_bar->priv->button_list; list; list = list->next)
    {
        ButtonData *button_data;
        button_data = BUTTON_DATA (list->data);
        if (button_data->button == button)
            return (GFile*)g_object_ref (button_data->path);
    }

    return NULL;
}


