#include "gtk_container.hpp"

namespace gtk
{
    Container::Container(){}

    Container::Container(GtkContainer* original, bool callAddReference)
        : Widget((GtkWidget*)original, callAddReference){}

    void Container::add(Widget& widgetToAdd)
    {
        gtk_container_add((GtkContainer*)widget, widgetToAdd.getPtr());
    }

    void Container::remove(Widget& widgetToRemove)
    {
        gtk_container_remove ((GtkContainer*)widget, widgetToRemove.getPtr());
    }
}