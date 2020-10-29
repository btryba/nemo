#ifndef __GTK_BOX_HPP__
#define __GTK_BOX_HPP__

#include "gtk_container.hpp"

namespace gtk
{
    class Box : public Container
    {
        public:
        Box(GtkOrientation orientation);
        virtual ~Box();
        void pack_start(Widget* child);
    };
}

#endif //__GTK_BOX_HPP__