#ifndef __GTK_WIDGETPATH_HPP__
#define __GTK_WIDGETPATH_HPP__

extern "C"
{
    #include <gtk/gtk.h>
}

namespace gtk
{
    class Widget;
    class WidgetPath
    {
        protected:
        GtkWidgetPath* path;
        bool UnRef;

        public:
        WidgetPath();
        WidgetPath(GtkWidgetPath* original, bool unrefWhenDone);
        ~WidgetPath();
        WidgetPath* copy();
        void append_for_widget(Widget& child);
        void append_with_siblings(WidgetPath& siblings, unsigned int position);
        GtkWidgetPath* getPtr();
    };
}

#endif //__GTK_WIDGETPATH_HPP__