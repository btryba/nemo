#include "gtk_button.hpp"

namespace gtk
{
    Button::Button(bool createToggle, const char *name, GtkActionGroup* actionGroup)
    {
        GtkWidget *image;
        GtkAction *action;

        if (createToggle)
        {
            widget = gtk_toggle_button_new();
        } else {
            widget = gtk_button_new();
        }

        image = gtk_image_new();

        gtk_button_set_image (GTK_BUTTON (widget), image);
        action = gtk_action_group_get_action (actionGroup, name);
        gtk_activatable_set_related_action (GTK_ACTIVATABLE (widget), action);
        gtk_button_set_label (GTK_BUTTON (widget), NULL);
        gtk_widget_set_tooltip_text (widget, gtk_action_get_tooltip (action));
    }
    Button::Button()
    {
        widget = gtk_button_new();
    }
    Button::~Button()
    {

    }
    void Button::set_focus_on_click(bool setFocus)
    {
        gtk_button_set_focus_on_click ((GtkButton*)widget, setFocus);
    }
}