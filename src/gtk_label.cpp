#include "gtk_label.hpp"

namespace gtk
{
    Label::Label(const char* text) : Widget{gtk_label_new(text), false}
    {}

    Label::Label(GtkLabel* original, bool callAddReference)
        :Widget((GtkWidget*)original, callAddReference){}
        
    bool Label::get_use_markup()
    {
        return gtk_label_get_use_markup((GtkLabel*)widget);
    }

    void Label::set_markup(char* markup)
    {
        gtk_label_set_markup ((GtkLabel*)widget, markup);
    }

    void Label::set_text(const char* text)
    {
        gtk_label_set_text((GtkLabel*)widget, text);
    }

    void Label::set_label(const char* label)
    {
        gtk_label_set_label((GtkLabel*)widget, label);
    }

    void Label::set_use_markup(bool toUse)
    {
       gtk_label_set_use_markup((GtkLabel*)widget, toUse);
    }

    void Label::set_ellipsize(PangoEllipsizeMode size)
    {
        gtk_label_set_ellipsize ((GtkLabel*)widget, size);
    }
}