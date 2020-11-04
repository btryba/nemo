#ifndef __GTK_LABEL_HPP__
#define __GTK_LABEL_HPP__

#include "gtk_widget.hpp"

namespace gtk
{
    struct Label : public Widget
    {
        Label(const char* text);
        Label(GtkLabel* original, bool callAddReference);
        bool get_use_markup();
        void set_markup(char* markup);
        void set_text(const char* text);
        void set_label(const char* label);
        void set_use_markup(bool toUse);
        void set_ellipsize(PangoEllipsizeMode size);
    };
}

#endif //__GTK_LABEL_HPP__