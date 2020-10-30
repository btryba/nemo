#include "gtk_box.hpp"

namespace gtk
{
    Box::Box(GtkOrientation orientation)
    {
        widget = gtk_box_new (orientation, 0);
    }

    Box::Box(GtkBox* original)
    {
        widget = (GtkWidget*)original;
        WidgetControlsWidgetPointer = false;
    }

    Box::~Box() {}
    
    void Box::pack_start(Widget* child)
    {
        gtk_box_pack_start ((GtkBox*)widget, child->getPtr(), true, true, 0);
    }
}
