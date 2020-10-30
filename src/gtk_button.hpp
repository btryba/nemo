#ifndef __GTK_BUTTON_HPP__
#define __GTK_BUTTON_HPP__

#include "gtk_container.hpp"

namespace gtk
{
    class Button : public Container
    {
        public:
        Button(bool createToggle, const char *name, GtkActionGroup* actionGroup);
        Button();
        virtual ~Button();
        void set_focus_on_click(bool setFocus);
    };
}

#endif //__GTK_BUTTON_HPP__