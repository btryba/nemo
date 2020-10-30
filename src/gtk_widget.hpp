#ifndef __GTK_WIDGET_HPP__
#define __GTK_WIDGET_HPP__

#include "gtk_stylecontext.hpp"

namespace gtk
{
    class Widget
    {
        protected:
        bool WidgetControlsWidgetPointer;
        GtkWidget* widget;

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
    };
}

#endif //__GTK_WIDGET_HPP__