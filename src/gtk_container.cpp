#include "gtk_container.hpp"

namespace gtk
{
    void Container::add(Widget* widgetToAdd)
    {
        gtk_container_add((GtkContainer*)widget, widgetToAdd->getPtr());
    }
}