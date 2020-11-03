/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Nemo
 *
 * Copyright (C) 2011, Red Hat, Inc.
 *
 * Nemo is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Nemo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, MA 02110-1335, USA.
 *
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */
extern "C"
{
#include <config.h>

#include "nemo-toolbar.h"

#include "nemo-location-bar.h"
#include "nemo-pathbar.h"
#include "nemo-actions.h" //Nemo Action macros
#include <glib/gi18n.h> //For _ Translations
#include <libnemo-private/nemo-global-preferences.h> //Nemo Preferences information

//#include <libnemo-private/nemo-ui-utilities.h> //Not used
//#include "nemo-window-private.h" //Not used
}

#include "nemo-toolbar.hpp"

enum {
	PROP_ACTION_GROUP = 1,
	PROP_SHOW_LOCATION_ENTRY,
	PROP_SHOW_MAIN_BAR,
	NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

//Forward declare so they can be used below
static void nemo_toolbar_update_appearance(NemoToolbar* self);
static void nemo_toolbar_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void nemo_toolbar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

namespace nemo
{
    Toolbar::Toolbar(GtkBox* cClass, GtkActionGroup* actionGroup) : gtk::Box(cClass),
        action_group(actionGroup),
        previous_button{NEMO_ACTION_BACK, action_group},
        next_button{NEMO_ACTION_FORWARD, action_group},
        up_button{NEMO_ACTION_UP, action_group}, 
        refresh_button{NEMO_ACTION_RELOAD, action_group},
        home_button{NEMO_ACTION_HOME, action_group}, 
        computer_button{NEMO_ACTION_COMPUTER, action_group},
        toggle_location_button{NEMO_ACTION_TOGGLE_LOCATION, action_group}, 
        open_terminal_button{NEMO_ACTION_OPEN_IN_TERMINAL, action_group},
        new_folder_button{NEMO_ACTION_NEW_FOLDER, action_group},
        search_button{NEMO_ACTION_SEARCH, action_group},
        show_thumbnails_button{NEMO_ACTION_SHOW_THUMBNAILS, action_group},
        icon_view_button{NEMO_ACTION_ICON_VIEW, action_group},
        list_view_button{NEMO_ACTION_LIST_VIEW, action_group},
        compact_view_button{NEMO_ACTION_COMPACT_VIEW, action_group},
        show_main_bar{true}
    {
        gtk_style_context_set_junction_sides (gtk_widget_get_style_context (GTK_WIDGET (widget)), GTK_JUNCTION_BOTTOM);

        show_location_entry = g_settings_get_boolean (nemo_preferences, NEMO_PREFERENCES_SHOW_LOCATION_ENTRY);

	    // add the UI 
        ui_manager.insert_action_group(action_group);

	    pack_start(toolbar);
        toolbar.get_style_context()->add_class(GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
	
        // Back/Forward/Up 
        gtk::Box* box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);

        box->add(previous_button);
        box->add(next_button);
        box->add(up_button);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);
        navigation_box.add(*box);
        toolbar.add(navigation_box);
        navigation_box.show_all();
        navigation_box.set_margin_right(6);

        // Refresh 
        box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);
        box->add(refresh_button);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);

        refresh_box.add(*box);
        toolbar.add(refresh_box);
        refresh_box.show_all();
        refresh_box.set_margin_right(6);

        // Home/Computer 
        box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);
        box->add(home_button);
        box->add(computer_button);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);
        location_box.add(*box);
        toolbar.add(location_box);
        location_box.show_all();
        location_box.set_margin_right(6);

        // Container to hold the location and pathbars 
        stack.set_transition_type(GTK_STACK_TRANSITION_TYPE_CROSSFADE);
        stack.set_transition_duration(150);

        // Regular Path Bar 
        gtk::Box* hbox = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);
        hbox->pack_start(stack);

        path_bar = nemo_path_bar_new();
        stack.add_named(path_bar, "path_bar");
    
        // Entry-Like Location Bar 
        location_bar = nemo_location_bar_new ();
        stack.add_named(location_bar, "location_bar");
        hbox->show_all();

        gtk::ToolItem* tool_box = new gtk::ToolItem();
        tool_box->set_expand(true);
        tool_box->add(*hbox);
        toolbar.add(*tool_box);
        tool_box->show();
        toolbar.takeOnOwnership(tool_box); //So that the c++ wrapper doesn't leak

        // Search/Open in Terminal/New Folder/Toggle Location 
        box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);
        box->add(toggle_location_button);
        box->add(open_terminal_button);
        box->add(new_folder_button);
        box->add(search_button);
        box->add(show_thumbnails_button);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);
        tools_box.add(*box);
        toolbar.add(tools_box);
        tools_box.show_all();
        tools_box.set_margin_left(6);

        setup_root_info_bar();

        // View Select 
        box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);
        box->add(icon_view_button);
        box->add(list_view_button);
        box->add(compact_view_button);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
        box->get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);
        view_box.add(*box);
        toolbar.add(view_box);
        view_box.show_all();
        view_box.set_margin_left(6);

        // nemo patch 
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_PREVIOUS_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_NEXT_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_UP_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_HOME_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_OPEN_IN_TERMINAL_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_ICON_VIEW_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_LIST_VIEW_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_COMPACT_VIEW_ICON_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_SHOW_THUMBNAILS_TOOLBAR, G_CALLBACK (nemo_toolbar_update_appearance), widget);
        g_signal_connect_swapped (nemo_preferences, "changed::" NEMO_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS, G_CALLBACK (nemo_toolbar_update_appearance), widget);

	    update_appearance();
    }

    Toolbar::~Toolbar()
    {
        g_clear_object (&action_group);
	    g_signal_handlers_disconnect_by_func (nemo_preferences, (gpointer)nemo_toolbar_update_appearance, widget);
    }

    void Toolbar::update_appearance()
    {
        update_root_state();

        toolbar.set_visible(show_main_bar);

        if (show_location_entry)
            stack.set_visible_child_name("location_bar");
        else
            stack.set_visible_child_name("path_bar");
        

        gtk_widget_set_visible (root_bar, show_root_bar);
            
        // Please refer to the element name, not the action name after the forward slash, otherwise the prefs will not work

        showHideButton(previous_button, NEMO_PREFERENCES_SHOW_PREVIOUS_ICON_TOOLBAR);
        showHideButton(next_button, NEMO_PREFERENCES_SHOW_NEXT_ICON_TOOLBAR);
        showHideButton(up_button, NEMO_PREFERENCES_SHOW_UP_ICON_TOOLBAR);
        showHideButton(refresh_button, NEMO_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR);
        showHideButton(home_button, NEMO_PREFERENCES_SHOW_HOME_ICON_TOOLBAR);
        showHideButton(computer_button, NEMO_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR);
        showHideButton(search_button, NEMO_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR);
        showHideButton(new_folder_button, NEMO_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR);
        showHideButton(open_terminal_button, NEMO_PREFERENCES_SHOW_OPEN_IN_TERMINAL_TOOLBAR);
        showHideButton(toggle_location_button, NEMO_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR);
        showHideButton(icon_view_button, NEMO_PREFERENCES_SHOW_ICON_VIEW_ICON_TOOLBAR);
        showHideButton(list_view_button, NEMO_PREFERENCES_SHOW_LIST_VIEW_ICON_TOOLBAR);
        showHideButton(compact_view_button, NEMO_PREFERENCES_SHOW_COMPACT_VIEW_ICON_TOOLBAR);
        showHideButton(show_thumbnails_button, NEMO_PREFERENCES_SHOW_SHOW_THUMBNAILS_TOOLBAR);
        
        if (!previous_button.get_visible() && !next_button.get_visible() && !up_button.get_visible())
            navigation_box.hide();
        else
            navigation_box.show();


        if (!home_button.get_visible() && !computer_button.get_visible())
            location_box.hide();
        else
            location_box.show();

        if (!refresh_button.get_visible()) 
            refresh_box.hide();
        else
            refresh_box.show();

        if (!search_button.get_visible() && !new_folder_button.get_visible() && 
            !open_terminal_button.get_visible() && !toggle_location_button.get_visible() &&
            !show_thumbnails_button.get_visible())
            tools_box.hide();
        else
            tools_box.show();

        if (!icon_view_button.get_visible() && !list_view_button.get_visible() &&
            !compact_view_button.get_visible())
            view_box.hide();
        else
            view_box.show();
    }

    void Toolbar::showHideButton(gtk::Button& button, const char* preferenceName) 
    {
        if (g_settings_get_boolean (nemo_preferences, preferenceName) == false ) 
            button.hide();
        else 
            button.show();
    }

    void Toolbar::setup_root_info_bar ()
    {
        root_bar = gtk_info_bar_new ();
        gtk_info_bar_set_message_type (GTK_INFO_BAR (root_bar), GTK_MESSAGE_ERROR);
        GtkWidget *content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (root_bar));

        GtkWidget *label = gtk_label_new (_("Elevated Privileges"));
        gtk_widget_show (label);
        gtk_container_add (GTK_CONTAINER (content_area), label);

        gtk_box_pack_start ((GtkBox*)widget, root_bar, true, true, 0);
    }

    void Toolbar::update_root_state()
    {
        if (geteuid() == 0 && g_settings_get_boolean (nemo_preferences, NEMO_PREFERENCES_SHOW_ROOT_WARNING))
        {
            if (!show_root_bar)
                show_root_bar = true;
        } 
        else
            show_root_bar = false;
    }

    GtkWidget* Toolbar::get_path_bar()
    {
        return path_bar;
    }

    GtkWidget* Toolbar::get_location_bar()
    {
        return location_bar;
    }

    void Toolbar::set_show_main_bar (bool new_show_main_bar)
    {
        if (new_show_main_bar != show_main_bar)
        {
            show_main_bar = new_show_main_bar;
            update_appearance();

            g_object_notify_by_pspec (G_OBJECT(widget), properties[PROP_SHOW_MAIN_BAR]);
        }
    }

    bool Toolbar::get_show_location_entry()
    {
        return show_location_entry;
    }

    void Toolbar::set_show_location_entry(bool new_show_location_entry)
    {
        if (new_show_location_entry != show_location_entry) {
            show_location_entry = new_show_location_entry;
            update_appearance();

            g_object_notify_by_pspec (G_OBJECT(widget), properties[PROP_SHOW_LOCATION_ENTRY]);
        }
    }

    void Toolbar::get_property (unsigned int property_id, GValue *value, GParamSpec *pspec)
    {
        switch (property_id) {
        case PROP_SHOW_LOCATION_ENTRY:
            g_value_set_boolean (value, show_location_entry);
            break;
        case PROP_SHOW_MAIN_BAR:
            g_value_set_boolean (value, show_main_bar);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (widget, property_id, pspec);
            break;
        }
    }

    void Toolbar::set_property (unsigned int property_id, const GValue *value, GParamSpec *pspec)
    {
        GtkActionGroup* setter;
        switch (property_id) 
        {
            case PROP_ACTION_GROUP:
                setter = (GtkActionGroup*)g_value_dup_object (value);
                if(setter != nullptr)
                    action_group = setter;
                break;
            case PROP_SHOW_LOCATION_ENTRY:
                nemo_toolbar_set_show_location_entry ((NemoToolbar*)widget, g_value_get_boolean (value));
                break;
            case PROP_SHOW_MAIN_BAR:
                nemo_toolbar_set_show_main_bar ((NemoToolbar*)widget, g_value_get_boolean (value));
                break;
            default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (widget, property_id, pspec);
                break;
        }
    }
}

//C Logic to support GTK Objects
G_DEFINE_TYPE (NemoToolbar, nemo_toolbar, GTK_TYPE_BOX);

GtkWidget* nemo_toolbar_new (GtkActionGroup *action_group)
{
	//Action Group is removed because "private" members moved to cppWrapper
    NemoToolbar* cClass = (NemoToolbar*)g_object_new (NEMO_TYPE_TOOLBAR,
			     "orientation", GTK_ORIENTATION_VERTICAL,
			     NULL);
    
    //Create the cpp Wrapper
    nemo::Toolbar* toolbar = new nemo::Toolbar((GtkBox*)cClass, action_group);
    //Point to cpp Wrapper
    cClass->cppParent = (void*)toolbar;
    //Return the GTK object like nothing special is going on.
    return (GtkWidget*)cClass;
}

//Needed to make GTK Objects work.
//Just call base class constructor
void nemo_toolbar_constructed (GObject *obj)
{
	G_OBJECT_CLASS (nemo_toolbar_parent_class)->constructed (obj);
}

//Needed to make GTK Objects work.
void nemo_toolbar_init (NemoToolbar *self){}

//Needed to make GTK Objects work.
//Destroy the cpp wrapper (delete looks at type)
//Call GTK base class destructor.
void nemo_toolbar_dispose (GObject *obj)
{
	NemoToolbar* self = (NemoToolbar*)obj;
    delete ((nemo::Toolbar*)self->cppParent);

	G_OBJECT_CLASS (nemo_toolbar_parent_class)->dispose (obj);
}

//Needed to make GTK Objects work.
static void nemo_toolbar_class_init (NemoToolbarClass *klass)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (klass);
	oclass->get_property = nemo_toolbar_get_property;
	oclass->set_property = nemo_toolbar_set_property;
	oclass->constructed = nemo_toolbar_constructed;
	oclass->dispose = nemo_toolbar_dispose;

	properties[PROP_ACTION_GROUP] =
		g_param_spec_object ("action-group",
				     "The action group",
				     "The action group to get actions from",
				     GTK_TYPE_ACTION_GROUP,
				     (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_STRINGS));
	properties[PROP_SHOW_LOCATION_ENTRY] =
		g_param_spec_boolean ("show-location-entry",
				      "Whether to show the location entry",
				      "Whether to show the location entry instead of the pathbar",
				      FALSE,
				      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	properties[PROP_SHOW_MAIN_BAR] =
		g_param_spec_boolean ("show-main-bar",
				      "Whether to show the main bar",
				      "Whether to show the main toolbar",
				      TRUE,
				      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	
	g_type_class_add_private (klass, sizeof (NemoToolbarClass));
	g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);
}



static void nemo_toolbar_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	NemoToolbar* self = (NemoToolbar*)object;
    ((nemo::Toolbar*)(self->cppParent))->get_property(property_id, value, pspec);
}

static void nemo_toolbar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	NemoToolbar* self = (NemoToolbar*)object;
    ((nemo::Toolbar*)(self->cppParent))->set_property(property_id, value, pspec);
}

static void nemo_toolbar_update_appearance(NemoToolbar* self)
{
    ((nemo::Toolbar*)(self->cppParent))->update_appearance();
}


// C Public Functions
GtkWidget * nemo_toolbar_get_path_bar (NemoToolbar *self)
{
	return ((nemo::Toolbar*)(self->cppParent))->get_path_bar();
}

GtkWidget * nemo_toolbar_get_location_bar (NemoToolbar *self)
{
	return ((nemo::Toolbar*)(self->cppParent))->get_location_bar();
}

gboolean nemo_toolbar_get_show_location_entry (NemoToolbar *self)
{
	return ((nemo::Toolbar*)(self->cppParent))->get_show_location_entry();
}

void nemo_toolbar_set_show_main_bar(NemoToolbar *self, gboolean show_main_bar)
{
	((nemo::Toolbar*)(self->cppParent))->set_show_main_bar(show_main_bar);
}

void nemo_toolbar_set_show_location_entry (NemoToolbar *self, gboolean show_location_entry)
{
	((nemo::Toolbar*)(self->cppParent))->set_show_location_entry(show_location_entry);
}
