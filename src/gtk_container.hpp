#ifndef __GTK_CONTAINER_HPP__
#define __GTK_CONTAINER_HPP__

#include "gtk_widget.hpp"

namespace gtk
{
    class Container : public Widget
    {
        public:
        Container(GtkContainer* original, bool callAddReference = true);
        void add(Widget& widgetToAdd);
        void remove(Widget& widgetToRemove);
    };
}

#endif //__GTK_CONTAINER_HPP__