#include "gtk_toolbar.hpp"

namespace gtk
{
    ToolBar::ToolBar()
    {
        widget = gtk_toolbar_new();
    }
    
    ToolBar::~ToolBar() { }
}