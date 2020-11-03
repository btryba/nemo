#ifndef __GTK_TOGGLEBUTTON_HPP__
#define __GTK_TOGGLEBUTTON_HPP__

#include "gtk_button.hpp"

namespace gtk
{
    class ToggleButton : public Button
    {
        public:

        ToggleButton(const char *name, GtkActionGroup* actionGroup);
        ToggleButton();
        ToggleButton(GtkToggleButton* original, bool callAddReference = true);
        virtual ~ToggleButton();
        bool get_active();
        void set_active(bool toSet);
    };
}

#endif //__GTK_TOGGLEBUTTON_HPP__