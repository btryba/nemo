#ifndef __NEMO_PATHBARBUTTON_HPP
#define __NEMO_PATHBARBUTTON_HPP

extern "C"
{
    #include <libnemo-private/nemo-file.h>
    #include <gtk/gtk.h>
}

#include "gtk_togglebutton.hpp"

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

    struct PathBarButton : gtk::ToggleButton
    {
        ButtonType type;
        char *dir_name;
        GFile *path;
        ::NemoFile *file;
        unsigned int file_changed_signal_id;

        char *mount_icon_name;
        /* flag to indicate its the base folder in the URI */
        bool is_base_dir;

        GtkWidget *image;
        GtkWidget *label;
        GtkWidget *alignment;
        unsigned int ignore_changes : 1;
        unsigned int fake_root : 1;

        /** XDG Dirs */
        GFile *xdg_documents_path;
        GFile *xdg_download_path;
        GFile *xdg_music_path;
        GFile *xdg_pictures_path;
        GFile *xdg_public_path;
        GFile *xdg_templates_path;
        GFile *xdg_videos_path;

        PathBarButton(NemoFile *nemoFile, bool current_dir, bool base_dir, bool desktop_is_home);
        virtual ~PathBarButton();
        void update_button_appearance();
        void update_button_state(bool current_dir);

        private:
        const char * get_dir_name();
        GFile* get_xdg_dir(GUserDirectory dir);
        void set_label_padding_size();
        void setup_button_drag_source();
        void setup_button_type(GFile *location, bool desktop_is_home);
        bool setup_file_path_mounted_mount (GFile *location);

        public: //For Slot
        static void button_drag_data_get_cb (GtkWidget *widget, GdkDragContext  *context, GtkSelectionData *selection_data, guint info, guint time_, gpointer user_data);
    };
}

#endif //__NEMO_PATHBARBUTTON_HPP