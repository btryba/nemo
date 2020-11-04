#include "gtk_box.hpp"

namespace gtk
{
    Box::Box(GtkOrientation orientation) : Container{(GtkContainer*)gtk_box_new (orientation, 0), false}
    {}

    Box::Box(GtkOrientation orientation, int spacing): Container{(GtkContainer*)gtk_box_new (orientation, spacing), false}
    {}

    Box::Box(GtkBox* original, bool callAddReferenece)
        : Container((GtkContainer*)original, callAddReferenece){}

    Box::~Box() {}
    
    void Box::pack_start(Widget& child, bool expand, bool fill, unsigned int padding)
    {
        gtk_box_pack_start ((GtkBox*)widget, child.getPtr(), expand, fill, padding);
    }
}
