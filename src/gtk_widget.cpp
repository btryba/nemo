#include "gtk_widget.hpp"
#include "gtk_settings.hpp"

namespace gtk
{
    Widget::Widget() {}

    Widget::Widget(GtkWidget* original, bool callAddReference)
    {
        if(callAddReference)
            widget = (GtkWidget*)g_object_ref(original);
        else
            widget = original;
    }

    Widget::~Widget()
    {
        if(G_IS_OBJECT(widget)) //If it is already removed from last container this will not be true
            g_object_unref(widget);

        // if(GTK_IS_WIDGET(widget) && DeletePtrOnDestructor)
        //     gtk_widget_destroy(widget); //I know this shouldn't be needed

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

    void Widget::get_preferred_width(int& minimumWidth)
    {
        gtk_widget_get_preferred_width(widget, &minimumWidth, nullptr);
    }

    void Widget::get_preferred_width(int& minimumWidth, int& naturalWidth)
    {
        gtk_widget_get_preferred_width(widget, &minimumWidth, &naturalWidth);
    }

    void Widget::get_preferred_height(int& minimumHeight)
    {
        gtk_widget_get_preferred_height(widget, &minimumHeight, nullptr);
    }

    void Widget::get_preferred_height(int& minimumHeight, int& naturalHeight)
    {
        gtk_widget_get_preferred_height(widget, &minimumHeight, &minimumHeight);
    }

    void Widget::set_sensitive(bool toSet)
    {
        gtk_widget_set_sensitive(widget, toSet);
    }

    bool Widget::has_focus()
    {
        return gtk_widget_has_focus(widget);
    }

    void Widget::get_allocation(GtkAllocation& allocation)
    {
        gtk_widget_get_allocation (widget, &allocation);
    }

    void Widget::size_allocate(GtkAllocation& allocation)
    {
        gtk_widget_size_allocate(widget, &allocation);
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

    bool Widget::ptrAddressesMatch(void* pointer)
    {
        return pointer == widget;
    }

    void Widget::set_has_window(bool toSet)
    {
        gtk_widget_set_has_window (widget, toSet);
    }

    void Widget::set_redraw_on_allocate(bool toSet)
    {
        gtk_widget_set_redraw_on_allocate(widget, toSet);
    }

    void Widget::drag_dest_set()
    {
        gtk_drag_dest_set (widget, (GtkDestDefaults)0, nullptr, 0, (GdkDragAction)0);
    }

    void Widget::drag_dest_set_track_motion(bool toSet)
    {
        gtk_drag_dest_set_track_motion (widget, toSet);
    }

    void Widget::set_tooltip_text(const char* text)
    {
        gtk_widget_set_tooltip_text (widget, text);
    }

    bool Widget::isNull()
    {
        return widget == nullptr;
    }

    PangoLayout* Widget::create_pango_layout(const char* name)
    {
        return gtk_widget_create_pango_layout(widget, name);
    }

    void Widget::drag_source_set(GdkModifierType startButtonMask, const GtkTargetEntry *targets, int n_targets, GdkDragAction actions)
    {
        gtk_drag_source_set(widget, startButtonMask, targets, n_targets, actions);
    }

    Settings Widget::getSettings()
    {
        return Settings{gtk_settings_get_for_screen(gtk_widget_get_screen(widget))};
    }

    void Widget::queue_resize()
    {
        gtk_widget_queue_resize(widget);
    }

     GtkTextDirection Widget::get_direction()
     {
         return gtk_widget_get_direction(widget);
     }
}