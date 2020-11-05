#ifndef __NEMO_PATHBARBUTTON_HPP
#define __NEMO_PATHBARBUTTON_HPP

#include "gtk_togglebutton.hpp"
#include "gtk_widget.hpp"
#include "gtk_file.hpp"
#include "gtk_label.hpp"

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
    
    struct PathBar;
    struct PathBarButton : gtk::ToggleButton
    {
        ButtonType type;
        gtk::File path;
        unsigned int fake_root : 1;
        char* mount_icon_name;
        char *dir_name;
        gtk::Label *label;
        bool ignore_changes; //So that signals don't fire at bad times.
        bool is_base_dir;

        PathBar* parent;

        PathBarButton(NemoFile& file);
        ~PathBarButton();
        void setType();
        bool setup_file_path_mounted_mount();
        const char* get_dir_name();
        void set_label_padding_size();
        void setup_button_drag_source();
        void update_button_state(bool current_dir);
        void button_data_free();
        void button_data_file_changed (NemoFile *file);
        void update_button_appearance();
        void button_clicked_implementation();

        static void drag_data_get_cb (GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, unsigned int info, unsigned int time_, gpointer user_data);
        static void button_clicked_cb (GtkWidget *button, gpointer   data);
        
    };
}

#endif //__NEMO_PATHBARBUTTON_HPP