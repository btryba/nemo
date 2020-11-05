#ifndef __NEMO_PATHBAR_HPP__
#define __NEMO_PATHBAR_HPP__

#include <gtkpp/gtkpp-Container.hpp>
#include <gtkpp/gtkpp-Button.hpp>

namespace nemo
{
    struct PathBar : public gtkpp::Container
    {
        GdkWindow *event_window;

        GFile *root_path;
        GFile *home_path;
        GFile *desktop_path;

        GFile *current_path;
        gpointer current_button_data;

        GList *button_list;
        GList *scrolled_root_button;
        GList *fake_root;

        gtkpp::Button *up_slider_button;
	    gtkpp::Button *down_slider_button;
        unsigned int settings_signal_id;
        int slider_width;
        int16_t spacing;
        int16_t button_offset;
        unsigned int timer;
        unsigned int slider_visible : 1;
        unsigned int need_timer : 1;
        unsigned int ignore_click : 1;

        unsigned int drag_slider_timeout;
        bool drag_slider_timeout_for_up_button;

        static bool desktop_is_home;

        PathBar(GtkContainer* cClass);
        ~PathBar();
        gtkpp::Button* get_slider_button(GtkPositionType position);
        void scroll_up();
        void scroll_down();
        void update_button_types();
        bool update_path(GFile& file_path, bool emit_signal);
        void get_preferred_width(int& minimum, int& natural);
        bool set_path(GFile &file_path);
        void stop_scrolling();
        void child_ordering_changed();
        void clear_buttons();

        static void scroll_up_static(PathBar* bar);
        static void scroll_down_static(PathBar* bar);
        static void get_preferred_width_static(GtkWidget* widget, int* minimum, int* natural);

        private:
        PathBarButton& buttonFromList(GList& list);
    };
}

#endif //__NEMO_PATHBAR_HPP__