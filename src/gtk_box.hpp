#ifndef __GTK_BOX_HPP__
#define __GTK_BOX_HPP__

#include "gtk_container.hpp"

namespace gtk
{
    class Box : public Container
    {
        public:
        Box(GtkOrientation orientation);
        Box(GtkOrientation orientation, int spacing);
        Box(GtkBox* original, bool callAddReference = true);
        virtual ~Box();
        void pack_start(Widget& child, bool expand = true, bool fill = true, unsigned int padding = 0);
    };
}

#endif //__GTK_BOX_HPP__