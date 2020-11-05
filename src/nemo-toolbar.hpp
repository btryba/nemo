#ifndef __NEMO_TOOLBAR_HPP__
#define __NEMO_TOOLBAR_HPP__

#include <gtkpp/gtkpp-StyleContext.hpp>
#include <gtkpp/gtkpp-Widget.hpp>
#include <gtkpp/gtkpp-Button.hpp>
#include <gtkpp/gtkpp-ToolItem.hpp>
#include <gtkpp/gtkpp-Stack.hpp>
#include <gtkpp/gtkpp-Box.hpp>
#include <gtkpp/gtkpp-ToolBar.hpp>
#include <gtkpp/gtkpp-ToggleButton.hpp>
#include <gtkpp/gtkpp-ActionGroup.hpp>
#include <gtkpp/gtkpp-UIManager.hpp>

namespace nemo
{
    class Toolbar : public gtkpp::Box
    {
        gtkpp::ToolBar toolbar;

        gtkpp::UIManager ui_manager;
        GtkActionGroup *action_group;

        gtkpp::Button previous_button;
        gtkpp::Button next_button;
        gtkpp::Button up_button;
        gtkpp::Button refresh_button;
        gtkpp::Button home_button;
        gtkpp::Button computer_button;
        gtkpp::Button toggle_location_button;
        gtkpp::Button open_terminal_button;
        gtkpp::Button new_folder_button;
        gtkpp::ToggleButton search_button;
        gtkpp::ToggleButton show_thumbnails_button;
        gtkpp::ToggleButton icon_view_button;
        gtkpp::ToggleButton list_view_button;
        gtkpp::ToggleButton compact_view_button;

        gtkpp::ToolItem navigation_box;
        gtkpp::ToolItem refresh_box;
        gtkpp::ToolItem location_box;
        gtkpp::ToolItem tools_box;
        gtkpp::ToolItem view_box;

        GtkWidget *path_bar;
        GtkWidget *location_bar;
        GtkWidget *root_bar;
        gtkpp::Stack stack;

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
        static void showHideButton(gtkpp::Button& button, const char* preferenceName);
    };
}

#endif //__NEMO_TOOLBAR_HPP__