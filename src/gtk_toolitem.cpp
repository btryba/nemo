#include "gtk_toolitem.hpp"

namespace gtk
{
    ToolItem::ToolItem()
    {
        widget = (GtkWidget*)gtk_tool_item_new();
    }

    ToolItem::ToolItem(GtkToolItem* original, bool callAddReference)
        : Container((GtkContainer*)original, callAddReference){}

    ToolItem::~ToolItem() {}
    
    void ToolItem::set_expand(bool setTo)
    {
        gtk_tool_item_set_expand((GtkToolItem*)widget, setTo);   
    }
}