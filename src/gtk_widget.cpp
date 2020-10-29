#include "gtk_widget.hpp"

namespace gtk
{
    Widget::Widget(){}

    Widget::~Widget(){}

    void Widget::show()
    {
        gtk_widget_show(widget);
    }

    void Widget::hide()
    {
        gtk_widget_hide(widget);
    }

    bool Widget::get_visible()
    {
        return gtk_widget_get_visible(widget);
    }

    GtkWidget* Widget::getPtr()
    {
        return widget;
    }

    void Widget::show_all()
    {
        gtk_widget_show_all (widget);
    }

    void Widget::set_margin_left(double marginSize)
    {
        gtk_widget_set_margin_left(widget, marginSize);
    }

    void Widget::set_margin_right(double marginSize)
    {
        gtk_widget_set_margin_right(widget, marginSize);
    }

    StyleContext* Widget::get_style_context()
    {
        return new StyleContext(gtk_widget_get_style_context(widget));
    }

    void Widget::set_visible(bool toShow)
    {
        gtk_widget_set_visible(widget, toShow);
    }
}