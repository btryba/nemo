#include "gtk_widgetpath.hpp"
#include "gtk_widget.hpp"

namespace gtk
{
    WidgetPath::WidgetPath()
        : path{gtk_widget_path_new()}, UnRef{true}
    {}

    WidgetPath::WidgetPath(GtkWidgetPath* original, bool unrefWhenDone)
        : path{original}, UnRef{unrefWhenDone}
    {}

    WidgetPath::~WidgetPath()
    {
        if(UnRef)
            gtk_widget_path_unref(path);
    }

    WidgetPath* WidgetPath::copy()
    {
        return new WidgetPath{gtk_widget_path_copy(path), true};
    }

    void WidgetPath::append_for_widget(Widget& child)
    {
        gtk_widget_path_append_for_widget(path, child.getPtr());
    }

    void WidgetPath::append_with_siblings(WidgetPath& siblings, unsigned int position)
    {
        gtk_widget_path_append_with_siblings(path, siblings.getPtr(), position);
    }

    GtkWidgetPath* WidgetPath::getPtr()
    {
        return path;
    }
}