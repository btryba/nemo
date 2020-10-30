#ifndef __NEMO_TOOLBAR_HPP__
#define __NEMO_TOOLBAR_HPP__

#include "gtk_stylecontext.hpp"
#include "gtk_widget.hpp"
#include "gtk_button.hpp"
#include "gtk_toolitem.hpp"
#include "gtk_stack.hpp"
#include "gtk_box.hpp"
#include "gtk_toolbar.hpp"

namespace gtk
{
    struct ActionGroup
    {
        GtkActionGroup* action_group;
        public:
        GtkActionGroup* getPtr()
        {
            return action_group;
        }
    };

    struct UIManager
    {
        GtkUIManager* ui_manager;
        public:
        UIManager()
        {
            ui_manager = gtk_ui_manager_new();
        }
        GtkUIManager* getPtr()
        {
            return ui_manager;
        }
        void insert_action_group(GtkActionGroup* actionGroup)
        {
             gtk_ui_manager_insert_action_group (ui_manager, actionGroup, 0);
        }
    };
}

namespace nemo
{
    class Toolbar : public gtk::Box
    {
        gtk::ToolBar toolbar;

        gtk::UIManager ui_manager;
        GtkActionGroup *action_group;

        gtk::Button previous_button;
        gtk::Button next_button;
        gtk::Button up_button;
        gtk::Button refresh_button;
        gtk::Button home_button;
        gtk::Button computer_button;
        gtk::Button toggle_location_button;
        gtk::Button open_terminal_button;
        gtk::Button new_folder_button;
        gtk::Button search_button;
        gtk::Button show_thumbnails_button;
        gtk::Button icon_view_button;
        gtk::Button list_view_button;
        gtk::Button compact_view_button;

        gtk::ToolItem navigation_box;
        gtk::ToolItem refresh_box;
        gtk::ToolItem location_box;
        gtk::ToolItem tools_box;
        gtk::ToolItem view_box;

        GtkWidget *path_bar;
        GtkWidget *location_bar;
        GtkWidget *root_bar;
        gtk::Stack stack;

        bool show_main_bar;
        bool show_location_entry;
        bool show_root_bar;

        public:
        Toolbar(GtkBox* cClass, GtkActionGroup* actionGroup);
        virtual ~Toolbar();
        
        void update_appearance();
        GtkWidget* get_path_bar();
        GtkWidget* get_location_bar();
        void set_show_main_bar(bool new_show_main_bar);
        bool get_show_location_entry();
        void set_show_location_entry(bool new_show_location_entry);
        void get_property(unsigned int property_id, GValue *value, GParamSpec *pspec);
        void set_property (unsigned int property_id, const GValue *value, GParamSpec *pspec);

        private:
        void setup_root_info_bar();
        void update_root_state();
        static void showHideButton(gtk::Button& button, const char* preferenceName);
    };
}

#endif //__NEMO_TOOLBAR_HPP__