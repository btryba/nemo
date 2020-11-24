#ifndef __NEMO_PATHBARBUTTON_HPP
#define __NEMO_PATHBARBUTTON_HPP

extern "C"
{
    #include <libnemo-private/nemo-file.h>
    #include <gtk/gtk.h>
}

#include <gtkpp/gtkpp-ToggleButton.hpp>
#include <gtkpp/gtkpp-Image.hpp>
#include <gtkpp/gtkpp-Label.hpp>
#include <simplex-lib/include/string.hpp>

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

    struct PathBarButton : gtkpp::ToggleButton
    {
        ButtonType type;
        char *dir_name;
        simplex::string path;
        ::NemoFile *file;
        unsigned int file_changed_signal_id;

        char *mount_icon_name;
        /* flag to indicate its the base folder in the URI */
        bool is_base_dir;

        gtkpp::Image image;
        gtkpp::Label *label;
        GtkWidget *alignment;
        unsigned int ignore_changes : 1;
        unsigned int fake_root : 1;

        /** XDG Dirs */
        const simplex::string xdg_documents_path;
        const simplex::string xdg_download_path;
        const simplex::string xdg_music_path;
        const simplex::string xdg_pictures_path;
        const simplex::string xdg_public_path;
        const simplex::string xdg_templates_path;
        const simplex::string xdg_videos_path;

        PathBarButton(NemoFile *nemoFile, bool current_dir, bool base_dir, bool desktop_is_home);
        virtual ~PathBarButton();
        void update_button_appearance();
        void update_button_state(bool current_dir);

        private:
        const char * get_dir_name();
        void set_label_padding_size();
        void setup_button_drag_source();
        void setup_button_type(simplex::string location, bool desktop_is_home);
        bool setup_file_path_mounted_mount (simplex::string location);
        
        
        static char* getUriFromPath(simplex::string path);

        public: //For Slot
        static void button_drag_data_get_cb (GtkWidget *widget, GdkDragContext  *context, GtkSelectionData *selection_data, guint info, guint time_, gpointer user_data);
    };
}

#endif //__NEMO_PATHBARBUTTON_HPP