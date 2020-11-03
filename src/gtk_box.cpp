#include "gtk_box.hpp"

namespace gtk
{
    Box::Box(GtkOrientation orientation)
    {
        widget = gtk_box_new (orientation, 0);
    }

    Box::Box(GtkBox* original, bool callAddReferenece)
        : Container((GtkContainer*)original, callAddReferenece){}

    Box::~Box() {}
    
    void Box::pack_start(Widget& child, bool expand, bool fill, unsigned int padding)
    {
        gtk_box_pack_start ((GtkBox*)widget, child.getPtr(), expand, fill, padding);
    }
}
