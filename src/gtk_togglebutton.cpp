#include "gtk_togglebutton.hpp"

namespace gtk
{
    ToggleButton::ToggleButton(const char *name, GtkActionGroup* actionGroup)
        : Button((GtkButton*)gtk_toggle_button_new(), false){}
    {
        GtkWidget *image;
        GtkAction *action;

        image = gtk_image_new();

        gtk_button_set_image (GTK_BUTTON (widget), image);
        action = gtk_action_group_get_action (actionGroup, name);
        gtk_activatable_set_related_action (GTK_ACTIVATABLE (widget), action);
        set_label(nullptr);
        set_tooltip_text(gtk_action_get_tooltip (action));
    }
    ToggleButton::ToggleButton(GtkToggleButton* original, bool callAddReference)
        : Button((GtkButton*)original, callAddReference){}

    ToggleButton::ToggleButton()
        : Button((GtkButton*)gtk_toggle_button_new(), false){}
    
    ToggleButton::~ToggleButton()
    {

    }
    bool ToggleButton::get_active()
    {
        return gtk_toggle_button_get_active ((GtkToggleButton*)widget);
    }
    void ToggleButton::set_active(bool toSet)
    {
        gtk_toggle_button_set_active((GtkToggleButton*)widget, toSet);
    }
}