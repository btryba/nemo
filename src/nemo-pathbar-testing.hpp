#ifndef __NEMO_PATHBAR_HPP__
#define __NEMO_PATHBAR_HPP__

#include "gtk_container.hpp"
#include "gtk_file.hpp"
#include "gtk_button.hpp"
#include "gtk_togglebutton.hpp"
#include <vector>

#include "nemo-pathbar-button.hpp"

extern "C" 
{
    #include <libnemo-private/nemo-file.h>
}

namespace nemo
{
    enum class ButtonType
    {
        Normal,
        Root,
        Home,
        Desktop,
        Mount,
        Xdg,
        DefaultLocation,
    };

    struct ButtonData
    {
        gtk::Button button;
        gtk::ToggleButton toggleButton;
        ButtonType type;
        char *dir_name;
        gtk::File *path;
        NemoFile *file;
        unsigned int file_changed_signal_id;

        char* mount_icon_name;
        /* flag to indicate its the base folder in the URI */
        bool is_base_dir;

        GtkWidget *image;
        GtkWidget *label;
        GtkWidget *alignment;
        unsigned int ignore_changes : 1;
        unsigned int fake_root : 1;
    };

    class PathBar : gtk::Container
    {
        public: //for now
        GdkWindow *event_window;

        gtk::File root_path;
        gtk::File home_path;
        gtk::File *desktop_path;

        gtk::File current_path;

        PathBarButton* current_button_data;

        std::vector<PathBarButton*> button_list;
        std::vector<PathBarButton*> scrolled_root_button;
        std::vector<PathBarButton*> fake_root;

        gtk::Button *up_slider_button;
	    gtk::Button *down_slider_button;
        
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

        public:
        PathBar(GtkContainer* cClass);
        ~PathBar();

        void setup_button_type (ButtonData& button_data, gtk::File& location);
        void clear_buttons();
        void desktop_location_changed_callback();
        bool slider_timeout();
        void slider_drag_motion(GtkWidget *widget, GdkDragContext *context, int x, int y, unsigned int time);
        void get_preferred_width(int& minimum, int& natural);
        void get_preferred_height(int& minimum, int& natural);
        void update_slider_buttons();
        bool check_parent_path(GFile *location, ButtonData **current_button_data);

        private:
        gtk::Button* get_slider_button(GtkPositionType position);
        void update_button_types();
        bool update_path (gtk::File& file_path, bool emit_signal);
        bool setup_file_path_mounted_mount (gtk::File& location, ButtonData& button_data);
        void child_ordering_changed();
        void stop_scrolling();
        void check_icon_theme();
        // Changes the icons wherever it is needed
        void reload_icons();

        static bool setup_file_path_mounted_mount (gtk::File& location, ButtonData& button_data);
        
    };
}

#endif //__NEMO_PATHBAR_HPP__