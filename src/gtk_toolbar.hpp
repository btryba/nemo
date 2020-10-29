#ifndef __GTK_TOOLBAR_HPP__
#define __GTK_TOOLBAR_HPP__

#include "gtk_container.hpp"

namespace gtk
{
    class ToolBar : public Container
    {
        public:
        ToolBar();
        virtual ~ToolBar();
    };
}

#endif //__GTK_TOOLBAR_HPP__