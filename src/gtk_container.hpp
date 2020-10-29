#ifndef __GTK_CONTAINER_HPP__
#define __GTK_CONTAINER_HPP__

#include "gtk_widget.hpp"

namespace gtk
{
    class Container : public Widget
    {
        public:
        void add(Widget* widgetToAdd);
    };
}

#endif //__GTK_CONTAINER_HPP__