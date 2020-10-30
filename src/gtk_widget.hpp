#ifndef __GTK_WIDGET_HPP__
#define __GTK_WIDGET_HPP__

#include "gtk_stylecontext.hpp"
#include <vector>

namespace gtk
{
    class Widget
    {
        protected:
        bool WidgetControlsWidgetPointer;
        GtkWidget* widget;
        std::vector<gtk::Widget*> ownedPtrs;

        public:
        Widget();
        virtual ~Widget();
        void show();
        void hide();
        bool get_visible();
        GtkWidget* getPtr();
        void show_all();
        void set_margin_left(double marginSize);
        void set_margin_right(double marginSize);
        StyleContext* get_style_context();
        void set_visible(bool toShow);
        void add_events(GdkEventMask eventFlags);
        void takeOnOwnership(gtk::Widget* pointer);
        void get_preferred_width(int& width);
        void set_sensitive(bool toSet);
        bool has_focus();
        void get_allocation(GtkAllocation* allocation);
        void size_allocate(GtkAllocation* allocation);
        bool get_child_visible();
        void set_child_visible(bool toSet);
        void setPtr(void* pointer);
    };
}

#endif //__GTK_WIDGET_HPP__