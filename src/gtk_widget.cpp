#include "gtk_widget.hpp"

namespace gtk
{
    Widget::Widget() : WidgetControlsWidgetPointer{true}
    {}

    Widget::~Widget()
    {
        if(GTK_IS_WIDGET(widget) && WidgetControlsWidgetPointer)
            gtk_widget_destroy(widget); //I know this shouldn't be needed

        for(gtk::Widget* pointer : ownedPtrs)
            delete pointer;
    }

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

    void Widget::add_events(GdkEventMask eventFlags)
    {
        gtk_widget_add_events (widget, eventFlags);
    }

    void Widget::takeOnOwnership(gtk::Widget* pointer)
    {
        ownedPtrs.push_back(pointer);
    }

    void Widget::get_preferred_width(int& width)
    {
        gtk_widget_get_preferred_width(widget, &width, nullptr);
    }

    void Widget::set_sensitive(bool toSet)
    {
        gtk_widget_set_sensitive(widget, toSet);
    }

    bool Widget::has_focus()
    {
        return gtk_widget_has_focus(widget);
    }

    void Widget::get_allocation(GtkAllocation* allocation)
    {
        gtk_widget_get_allocation (widget, allocation);
    }

    void Widget::size_allocate(GtkAllocation* allocation)
    {
        gtk_widget_size_allocate(widget, allocation);
    }

    bool Widget::get_child_visible()
    {
        return gtk_widget_get_child_visible(widget);
    }

    void Widget::set_child_visible(bool toSet)
    {
        gtk_widget_set_child_visible(widget, toSet);
    }

    void Widget::setPtr(void* pointer)
    {
        widget = (GtkWidget*)pointer;
    }
}