#ifndef __GTK_BUTTON_HPP__
#define __GTK_BUTTON_HPP__

#include "gtk_widget.hpp"

namespace gtk
{
    class Button : public Widget
    {
        public:
        Button(bool createToggle, const char *name, GtkActionGroup* actionGroup);
        Button(){}
        virtual ~Button();
    };
}

#endif //__GTK_BUTTON_HPP__