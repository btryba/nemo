#ifndef __GTK_TOOLITEM_HPP__
#define __GTK_TOOLITEM_HPP__

#include "gtk_container.hpp"

namespace gtk
{
    class ToolItem : public Container
    {
        public:
        ToolItem();
        ToolItem(GtkToolItem* original, bool callAddReference = true);
        virtual ~ToolItem();
        void set_expand(bool setTo);
    };
}

#endif //__GTK_TOOLITEM_HPP__