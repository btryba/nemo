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
#include "nemo-window-private.h"
#include "nemo-actions.h"
#include <glib/gi18n.h>
#include <libnemo-private/nemo-global-preferences.h>
#include <libnemo-private/nemo-ui-utilities.h>
}

#include "gtk_stylecontext.hpp"
#include "gtk_widget.hpp"
#include "gtk_button.hpp"
#include "gtk_toolitem.hpp"
#include "gtk_stack.hpp"
#include "gtk_box.hpp"
#include "gtk_toolbar.hpp"

struct _NemoToolbarPriv {
	gtk::ToolBar *toolbar;

	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;

    gtk::Button *previous_button;
    gtk::Button *next_button;
    gtk::Button *up_button;
    gtk::Button *refresh_button;
    gtk::Button *home_button;
    gtk::Button *computer_button;
    gtk::Button *toggle_location_button;
    gtk::Button *open_terminal_button;
    gtk::Button *new_folder_button;
    gtk::Button *search_button;
    gtk::Button *icon_view_button;
    gtk::Button *list_view_button;
    gtk::Button *compact_view_button;
    gtk::Button *show_thumbnails_button;

    gtk::ToolItem *navigation_box;
    gtk::ToolItem *refresh_box;
    gtk::ToolItem *location_box;
    gtk::ToolItem *tools_box;
    gtk::ToolItem *view_box;

	GtkWidget *path_bar;
	GtkWidget *location_bar;
    GtkWidget *root_bar;
    gtk::Stack *stack;

	bool show_main_bar;
	bool show_location_entry;
    bool show_root_bar;
};

enum {
	PROP_ACTION_GROUP = 1,
	PROP_SHOW_LOCATION_ENTRY,
	PROP_SHOW_MAIN_BAR,
	NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (NemoToolbar, nemo_toolbar, GTK_TYPE_BOX);

static void nemo_toolbar_update_root_state (NemoToolbar *self)
{
    if (geteuid() == 0 && g_settings_get_boolean (nemo_preferences, NEMO_PREFERENCES_SHOW_ROOT_WARNING)) {
        if (self->priv->show_root_bar != TRUE) {
            self->priv->show_root_bar = TRUE;
        }
    } else {
        self->priv->show_root_bar = FALSE;
    }
}

static void showHideButton(gtk::Button& button, const char* preferenceName) 
{
    if (g_settings_get_boolean (nemo_preferences, preferenceName) == false ) 
        button.hide();
    else 
        button.show();
}

static void toolbar_update_appearance (NemoToolbar *selfPublic)
{
	_NemoToolbarPriv& self = *selfPublic->priv;

    nemo_toolbar_update_root_state (selfPublic);

	self.toolbar->set_visible(self.show_main_bar);

    if (self.show_location_entry)
        self.stack->set_visible_child_name("location_bar");
    else
        self.stack->set_visible_child_name("path_bar");
    

    gtk_widget_set_visible (self.root_bar, self.show_root_bar);
        
        /* Please refer to the element name, not the action name after the forward slash, otherwise the prefs will not work*/

    showHideButton(*self.previous_button, NEMO_PREFERENCES_SHOW_PREVIOUS_ICON_TOOLBAR);
    showHideButton(*self.next_button, NEMO_PREFERENCES_SHOW_NEXT_ICON_TOOLBAR);
    showHideButton(*self.up_button, NEMO_PREFERENCES_SHOW_UP_ICON_TOOLBAR);
    showHideButton(*self.refresh_button, NEMO_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR);
    showHideButton(*self.home_button, NEMO_PREFERENCES_SHOW_HOME_ICON_TOOLBAR);
    showHideButton(*self.computer_button, NEMO_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR);
    showHideButton(*self.search_button, NEMO_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR);
    showHideButton(*self.new_folder_button, NEMO_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR);
    showHideButton(*self.open_terminal_button, NEMO_PREFERENCES_SHOW_OPEN_IN_TERMINAL_TOOLBAR);
    showHideButton(*self.toggle_location_button, NEMO_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR);
    showHideButton(*self.icon_view_button, NEMO_PREFERENCES_SHOW_ICON_VIEW_ICON_TOOLBAR);
    showHideButton(*self.list_view_button, NEMO_PREFERENCES_SHOW_LIST_VIEW_ICON_TOOLBAR);
    showHideButton(*self.compact_view_button, NEMO_PREFERENCES_SHOW_COMPACT_VIEW_ICON_TOOLBAR);
    showHideButton(*self.show_thumbnails_button, NEMO_PREFERENCES_SHOW_SHOW_THUMBNAILS_TOOLBAR);
    
    if (!self.previous_button->get_visible() && !self.next_button->get_visible() && !self.up_button->get_visible() == false)
        self.navigation_box->hide();
    else
        self.navigation_box->show();


    if (!self.home_button->get_visible() && !self.computer_button->get_visible())
        self.location_box->hide();
    else
        self.location_box->show();

    if (!self.refresh_button->get_visible()) 
        self.refresh_box->hide();
    else
        self.refresh_box->show();

    if (!self.search_button->get_visible() && !self.new_folder_button->get_visible() && 
        !self.open_terminal_button->get_visible() && !self.toggle_location_button->get_visible() &&
        !self.show_thumbnails_button->get_visible())
        self.tools_box->hide();
    else
        self.tools_box->show();

    if (!self.icon_view_button->get_visible() && !self.list_view_button->get_visible() &&
        !self.compact_view_button->get_visible())
        self.view_box->hide();
    else
        self.view_box->show();
}

static void setup_root_info_bar (NemoToolbar *self) {

    GtkWidget *root_bar = gtk_info_bar_new ();
    gtk_info_bar_set_message_type (GTK_INFO_BAR (root_bar), GTK_MESSAGE_ERROR);
    GtkWidget *content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (root_bar));

    GtkWidget *label = gtk_label_new (_("Elevated Privileges"));
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (content_area), label);

    self->priv->root_bar = root_bar;
    gtk_box_pack_start (GTK_BOX (self), self->priv->root_bar, TRUE, TRUE, 0);
}

static void nemo_toolbar_constructed (GObject *obj)
{
	NemoToolbar *self = NEMO_TOOLBAR (obj);
    _NemoToolbarPriv& selfPriv = *self->priv;

	G_OBJECT_CLASS (nemo_toolbar_parent_class)->constructed (obj);

	gtk_style_context_set_junction_sides (gtk_widget_get_style_context (GTK_WIDGET (self)),
					      GTK_JUNCTION_BOTTOM);

    selfPriv.show_location_entry = g_settings_get_boolean (nemo_preferences, NEMO_PREFERENCES_SHOW_LOCATION_ENTRY);

	/* add the UI */
	selfPriv.ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (selfPriv.ui_manager, selfPriv.action_group, 0);

	selfPriv.toolbar = new gtk::ToolBar();
    gtk_box_pack_start (GTK_BOX (self), selfPriv.toolbar->getPtr(), true, true, 0);
	
    selfPriv.toolbar->get_style_context()->add_class(GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
	
    /* Back/Forward/Up */
    selfPriv.navigation_box = new gtk::ToolItem();
    gtk::Box* box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);

    selfPriv.previous_button = new gtk::Button(false, NEMO_ACTION_BACK, selfPriv.action_group);
    box->add(selfPriv.previous_button);

    selfPriv.next_button = new gtk::Button(false, NEMO_ACTION_FORWARD, selfPriv.action_group);
    box->add(selfPriv.next_button);

    selfPriv.up_button = new gtk::Button(false, NEMO_ACTION_UP, selfPriv.action_group);
    box->add(selfPriv.up_button);

    box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
    box->get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);

    selfPriv.navigation_box->add(box);
    selfPriv.toolbar->add(selfPriv.navigation_box);

    selfPriv.navigation_box->show_all();
    selfPriv.navigation_box->set_margin_right(6);

    /* Refresh */
    selfPriv.refresh_box = new gtk::ToolItem();
    box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);

    selfPriv.refresh_button = new gtk::Button(false, NEMO_ACTION_RELOAD, selfPriv.action_group);
    box->add(selfPriv.refresh_button);
    box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);

    selfPriv.refresh_box->add(box);
    selfPriv.toolbar->add(selfPriv.refresh_box);

    selfPriv.refresh_box->show_all();
    selfPriv.refresh_box->set_margin_right(6);

    /* Home/Computer */
    selfPriv.location_box = new gtk::ToolItem();
    box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);

    selfPriv.home_button = new gtk::Button(false, NEMO_ACTION_HOME, selfPriv.action_group);
    box->add(selfPriv.home_button);

    selfPriv.computer_button = new gtk::Button(false, NEMO_ACTION_COMPUTER, selfPriv.action_group);
    box->add(selfPriv.computer_button);

    box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
    box->get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);

    selfPriv.location_box->add(box);
    selfPriv.toolbar->add(selfPriv.location_box);

    selfPriv.location_box->show_all();
    selfPriv.location_box->set_margin_right(6);

    /* Container to hold the location and pathbars */
    selfPriv.stack = new gtk::Stack();
    selfPriv.stack->set_transition_type(GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    selfPriv.stack->set_transition_duration(150);

    /* Regular Path Bar */
    gtk::Box* hbox = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);
    hbox->pack_start(selfPriv.stack);

    selfPriv.path_bar = (GtkWidget*)g_object_new (NEMO_TYPE_PATH_BAR, NULL);
    selfPriv.stack->add_named(selfPriv.path_bar, "path_bar");
    
    /* Entry-Like Location Bar */
    selfPriv.location_bar = nemo_location_bar_new ();
    selfPriv.stack->add_named(selfPriv.location_bar, "location_bar");
    hbox->show_all();

    gtk::ToolItem* tool_box = new gtk::ToolItem();
    tool_box->set_expand(true);
    tool_box->add(hbox);
    selfPriv.toolbar->add(tool_box);
    tool_box->show();

    /* Search/Open in Terminal/New Folder/Toggle Location */
    selfPriv.tools_box = new gtk::ToolItem();
    box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);

    selfPriv.toggle_location_button = new gtk::Button(false, NEMO_ACTION_TOGGLE_LOCATION, selfPriv.action_group);
    box->add(selfPriv.toggle_location_button);

    selfPriv.open_terminal_button = new gtk::Button(false, NEMO_ACTION_OPEN_IN_TERMINAL, selfPriv.action_group);
    box->add(selfPriv.open_terminal_button);

    selfPriv.new_folder_button = new gtk::Button(false, NEMO_ACTION_NEW_FOLDER, selfPriv.action_group);
    box->add(selfPriv.new_folder_button);

    selfPriv.search_button = new gtk::Button(true, NEMO_ACTION_SEARCH, selfPriv.action_group);
    box->add(selfPriv.search_button);
    
    selfPriv.show_thumbnails_button = new gtk::Button(true, NEMO_ACTION_SHOW_THUMBNAILS, selfPriv.action_group);
    box->add(selfPriv.show_thumbnails_button);

    box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
    box->get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);

    selfPriv.tools_box->add(box);
    selfPriv.toolbar->add(selfPriv.tools_box);

    selfPriv.tools_box->show_all();
    selfPriv.tools_box->set_margin_left(6);

    setup_root_info_bar (self);

    /* View Select */
    selfPriv.view_box = new gtk::ToolItem();
    box = new gtk::Box(GTK_ORIENTATION_HORIZONTAL);

    selfPriv.icon_view_button = new gtk::Button(true, NEMO_ACTION_ICON_VIEW, selfPriv.action_group);
    box->add(selfPriv.icon_view_button);

    selfPriv.list_view_button = new gtk::Button(true, NEMO_ACTION_LIST_VIEW, selfPriv.action_group);
    box->add(selfPriv.list_view_button);

    selfPriv.compact_view_button = new gtk::Button(true, NEMO_ACTION_COMPACT_VIEW, selfPriv.action_group);
    box->add(selfPriv.compact_view_button);

    box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
    box->get_style_context()->add_class(GTK_STYLE_CLASS_LINKED);

    selfPriv.view_box->add(box);
    selfPriv.toolbar->add(selfPriv.view_box);

    selfPriv.view_box->show_all();
    selfPriv.view_box->set_margin_left(6);

    /* nemo patch */
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_PREVIOUS_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_NEXT_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_UP_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_EDIT_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_RELOAD_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_HOME_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_COMPUTER_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_SEARCH_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_NEW_FOLDER_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_OPEN_IN_TERMINAL_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_ICON_VIEW_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_LIST_VIEW_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_COMPACT_VIEW_ICON_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_SHOW_THUMBNAILS_TOOLBAR,
                  G_CALLBACK (toolbar_update_appearance), self);
    g_signal_connect_swapped (nemo_preferences,
                  "changed::" NEMO_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
                  G_CALLBACK (toolbar_update_appearance), self);

	toolbar_update_appearance (self);
}

static void nemo_toolbar_init (NemoToolbar *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, NEMO_TYPE_TOOLBAR,
						  NemoToolbarPriv);
	self->priv->show_main_bar = TRUE;	
}

static void nemo_toolbar_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	NemoToolbar *self = NEMO_TOOLBAR (object);

	switch (property_id) {
	case PROP_SHOW_LOCATION_ENTRY:
		g_value_set_boolean (value, self->priv->show_location_entry);
		break;
	case PROP_SHOW_MAIN_BAR:
		g_value_set_boolean (value, self->priv->show_main_bar);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void nemo_toolbar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	NemoToolbar *self = NEMO_TOOLBAR (object);

	switch (property_id) {
	case PROP_ACTION_GROUP:
		self->priv->action_group = (GtkActionGroup*)g_value_dup_object (value);
		break;
	case PROP_SHOW_LOCATION_ENTRY:
		nemo_toolbar_set_show_location_entry (self, g_value_get_boolean (value));
		break;
	case PROP_SHOW_MAIN_BAR:
		nemo_toolbar_set_show_main_bar (self, g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
nemo_toolbar_dispose (GObject *obj)
{
	NemoToolbar *self = NEMO_TOOLBAR (obj);

	g_clear_object (&self->priv->action_group);

	g_signal_handlers_disconnect_by_func (nemo_preferences,
					      (gpointer)toolbar_update_appearance, self);

    delete self->priv->toolbar;
    delete self->priv->previous_button;
    delete self->priv->next_button;
    delete self->priv->up_button;
    delete self->priv->refresh_button;
    delete self->priv->home_button;
    delete self->priv->computer_button;
    delete self->priv->toggle_location_button;
    delete self->priv->open_terminal_button;
    delete self->priv->new_folder_button;
    delete self->priv->search_button;
    delete self->priv->icon_view_button;
    delete self->priv->list_view_button;
    delete self->priv->compact_view_button;
    delete self->priv->show_thumbnails_button;
    delete self->priv->navigation_box;
    delete self->priv->refresh_box;
    delete self->priv->location_box;
    delete self->priv->view_box;
    delete self->priv->stack;

	G_OBJECT_CLASS (nemo_toolbar_parent_class)->dispose (obj);
}

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

GtkWidget * nemo_toolbar_new (GtkActionGroup *action_group)
{
	return (GtkWidget*)g_object_new (NEMO_TYPE_TOOLBAR,
			     "action-group", action_group,
			     "orientation", GTK_ORIENTATION_VERTICAL,
			     NULL);
}

GtkWidget * nemo_toolbar_get_path_bar (NemoToolbar *self)
{
	return self->priv->path_bar;
}

GtkWidget * nemo_toolbar_get_location_bar (NemoToolbar *self)
{
	return self->priv->location_bar;
}

gboolean nemo_toolbar_get_show_location_entry (NemoToolbar *self)
{
	return self->priv->show_location_entry;
}

void nemo_toolbar_set_show_main_bar (NemoToolbar *self, gboolean show_main_bar)
{
	if (show_main_bar != self->priv->show_main_bar)
    {
		self->priv->show_main_bar = show_main_bar;
		toolbar_update_appearance (self);

		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_MAIN_BAR]);
	}
}

void nemo_toolbar_set_show_location_entry (NemoToolbar *self, gboolean show_location_entry)
{
	if (show_location_entry != self->priv->show_location_entry) {
		self->priv->show_location_entry = show_location_entry;
		toolbar_update_appearance (self);

		g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_LOCATION_ENTRY]);
	}
}
