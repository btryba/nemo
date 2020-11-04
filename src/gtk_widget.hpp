#ifndef __GTK_WIDGET_HPP__
#define __GTK_WIDGET_HPP__

#include "gtk_stylecontext.hpp"
#include <vector>

namespace gtk
{
    struct Settings;
    class Widget
    {
        protected:
        GtkWidget* widget;
        std::vector<gtk::Widget*> ownedPtrs;

        public:
        Widget();
        Widget(GtkWidget* original, bool callAddReference = true);
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
        void takeOnOwnership(Widget* pointer);
        void get_preferred_width(int& minimumWidth);
        void get_preferred_width(int& minimumWidth, int& naturalWidth);
        void get_preferred_height(int& minimumHeight);
        void get_preferred_height(int& minimumHeight, int& naturalHeight);
        void set_sensitive(bool toSet);
        bool has_focus();
        void get_allocation(GtkAllocation& allocation);
        void size_allocate(GtkAllocation& allocation);
        bool get_child_visible();
        void set_child_visible(bool toSet);
        void setPtr(void* pointer);
        bool ptrAddressesMatch(void* pointer);
        void set_has_window(bool toSet);
        void set_redraw_on_allocate(bool toSet);
        void drag_dest_set();
        void drag_dest_set_track_motion(bool toSet);
        void set_tooltip_text(const char* text);
        bool isNull();
        PangoLayout* create_pango_layout(const char* name);
        void drag_source_set(GdkModifierType startButtonMask,const GtkTargetEntry *targets, int n_targets, GdkDragAction actions);
        Settings getSettings();
        void queue_resize();
        GtkTextDirection get_direction();
    };
}

#endif //__GTK_WIDGET_HPP__