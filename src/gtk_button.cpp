#include "gtk_button.hpp"

namespace gtk
{
    Button::Button(const char *name, GtkActionGroup* actionGroup)
    {
        GtkWidget *image;
        GtkAction *action;

        widget = gtk_button_new();

        image = gtk_image_new();

        gtk_button_set_image (GTK_BUTTON (widget), image);
        action = gtk_action_group_get_action (actionGroup, name);
        gtk_activatable_set_related_action (GTK_ACTIVATABLE (widget), action);
        set_label(nullptr);
        set_tooltip_text(gtk_action_get_tooltip (action));
    }
    Button::Button(GtkButton* original, bool callAddReference)
        : Container ((GtkContainer*)original, callAddReference){}

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
    void Button::set_label(const char* label)
    {
        gtk_button_set_label ((GtkButton*)widget, label);
    }
}