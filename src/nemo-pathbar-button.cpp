#include "nemo-pathbar-button.hpp"

extern "C"
{
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <eel/eel-vfs-extensions.h>

#include <libnemo-private/nemo-file.h>
#include <libnemo-private/nemo-file-utilities.h>
#include <libnemo-private/nemo-icon-names.h>

#include "nemo-window-slot-dnd.h"
}

#include "gtk_box.hpp"

namespace nemo
{
    PathBarButton::PathBarButton(NemoFile *nemoFile, bool current_dir, bool base_dir, bool desktop_is_home)
        : gtk::ToggleButton(),
        dir_name{nemo_file_get_display_name (nemoFile)},
        file{nemo_file_ref (nemoFile)},
        mount_icon_name{nullptr},
        image{},
        ignore_changes{false},
        fake_root{false},
        xdg_documents_path{get_xdg_dir (G_USER_DIRECTORY_DOCUMENTS)},
        xdg_download_path{get_xdg_dir (G_USER_DIRECTORY_DOWNLOAD)},
        xdg_music_path{get_xdg_dir (G_USER_DIRECTORY_MUSIC)},
        xdg_pictures_path{get_xdg_dir (G_USER_DIRECTORY_PICTURES)},
        xdg_public_path{get_xdg_dir (G_USER_DIRECTORY_PUBLIC_SHARE)},
        xdg_templates_path{get_xdg_dir (G_USER_DIRECTORY_TEMPLATES)},
        xdg_videos_path{get_xdg_dir (G_USER_DIRECTORY_VIDEOS)}
    {
        char *uri;

        GFile *tempPath = nemo_file_get_location (nemoFile);
        gtk::Box* box = new gtk::Box{GTK_ORIENTATION_HORIZONTAL, 2};

        setup_button_type(tempPath, desktop_is_home);
        
        get_style_context()->add_class("text-button");
        set_focus_on_click(false);
        add_events(GDK_SCROLL_MASK);
        /* TODO update button type when xdg directories change */

        switch (type)
        {
            case ButtonType::Root:
                label = nullptr;
                add(image);
                break;
            case ButtonType::Home:
            case ButtonType::Desktop:
            case ButtonType::Mount:
            case ButtonType::DefaultLocation:
                label = new gtk::Label(nullptr);
                label->set_ellipsize(PANGO_ELLIPSIZE_MIDDLE);
                box->pack_start(image, false, false);
                box->pack_start(*label, false, false);
                add(*box);
                takeOnOwnership(box);
                break;
            case ButtonType::Xdg:
            case ButtonType::Normal:
            default:
                label = new gtk::Label(nullptr);
                label->set_ellipsize(PANGO_ELLIPSIZE_MIDDLE);
                box->pack_start(image, false, false);
                box->pack_start(*label, false, false);
                add(*box);
                takeOnOwnership(box);
                is_base_dir = base_dir;
        }

        path = (GFile*)g_object_ref (tempPath);

        nemo_file_monitor_add (file, this, NEMO_FILE_ATTRIBUTES_FOR_ICON);
        show_all();

        update_button_state(current_dir);

        uri = g_file_get_uri (path);

        if (!eel_uri_is_search (uri))
            setup_button_drag_source();

        g_clear_pointer (&uri, g_free);

        nemo_drag_slot_proxy_init(getPtr(), file, nullptr);

        g_object_unref (tempPath);
    }

    PathBarButton::~PathBarButton()
    {
        g_object_unref (path);
        g_free (dir_name);

        g_clear_pointer (&mount_icon_name, g_free);

        if (file != NULL) {
            g_signal_handler_disconnect (file,
                            file_changed_signal_id);
            nemo_file_monitor_remove (file, this);
            nemo_file_unref (file);
        }

        g_clear_object (&xdg_documents_path);
        g_clear_object (&xdg_download_path);
        g_clear_object (&xdg_music_path);
        g_clear_object (&xdg_pictures_path);
        g_clear_object (&xdg_public_path);
        g_clear_object (&xdg_templates_path);
        g_clear_object (&xdg_videos_path);
    }

    void PathBarButton::update_button_appearance()
    {
        if (label != nullptr)
        {
            const char *dir_name = get_dir_name();
            if (label->get_use_markup())
            {
                char *markup = g_markup_printf_escaped ("<b>%s</b>", dir_name);
                label->set_markup(markup);
                label->set_margin_left(0);
                label->set_margin_right(0);

                g_free(markup);
            } 
            else
            {
                label->set_text(dir_name);
                set_label_padding_size();
            }
        }

        char *icon_name = nullptr;

        switch (type) 
        {
            case ButtonType::Root:
                icon_name = g_strdup (NEMO_ICON_SYMBOLIC_FILESYSTEM);
                break;
            case ButtonType::Home:
            case ButtonType::Desktop:
            case ButtonType::Xdg:
                icon_name = nemo_file_get_control_icon_name (file);
                break;
            case ButtonType::Normal:
                if (is_base_dir) 
                {
                    icon_name = nemo_file_get_control_icon_name (file);
                    break;
                }
            case ButtonType::DefaultLocation:
            case ButtonType::Mount:
                if (mount_icon_name) 
                {
                    icon_name = g_strdup (mount_icon_name);
                    break;
                }
            default:
                icon_name = nullptr;
        }

        image.set_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);

        g_free (icon_name);
    }

   void PathBarButton::update_button_state(bool current_dir)
    {
        if (label != nullptr)
        {
            label->set_label(nullptr);
            label->set_use_markup(current_dir);
        }

        update_button_appearance ();

        if (get_active() != current_dir)
        {
            ignore_changes = true;
            set_active(current_dir);
            ignore_changes = false;
        }
    }


//Private
    const char * PathBarButton::get_dir_name()
    {
        if (type == ButtonType::Desktop) 
        {
            return _("Desktop");
        /*
        }
        * originally this would look like /home/Home/Desktop in the pathbar.
        * I can see the logic, when you're only staying in $HOME, i.e. Home/Desktop
        * but when you've come from a directory further up it just looks wrong. *
        } else if (button_data->type == HOME_BUTTON) {
            return _("Home");*/
        } 
        else
            return dir_name;
    }

    /**
     * Utility function. Return a GFile for the "special directory" if it exists, or NULL
     * Ripped from nemo-file.c (nemo_file_is_user_special_directory) and slightly modified
     */
    GFile* PathBarButton::get_xdg_dir(GUserDirectory dir)
    {
        const char *special_dir = g_get_user_special_dir (dir);

        if (special_dir)
            return g_file_new_for_path (special_dir);
        else
            return nullptr;
    }

    void PathBarButton::set_label_padding_size()
    {
        const char *dir_name = get_dir_name();
       
        PangoLayout *layout = label->create_pango_layout(dir_name);
        int width, height;
        pango_layout_get_pixel_size (layout, &width, &height);

        char *markup = g_markup_printf_escaped ("<b>%s</b>", dir_name);
        pango_layout_set_markup (layout, markup, -1);
        g_free (markup);

        int bold_width, bold_height;
        pango_layout_get_pixel_size (layout, &bold_width, &bold_height);

        label->set_margin_left((bold_width - width) / 2);
        label->set_margin_right((bold_width - width) / 2);

        g_object_unref (layout);
    }

    void PathBarButton::setup_button_drag_source()
    {
        GtkTargetList *target_list;
        const GtkTargetEntry targets[] = {
            { (char *)NEMO_ICON_DND_GNOME_ICON_LIST_TYPE, 0, NEMO_ICON_DND_GNOME_ICON_LIST }
        };

            drag_source_set((GdkModifierType)(GDK_BUTTON1_MASK |
                    GDK_BUTTON2_MASK),
                        NULL, 0,
                    (GdkDragAction)(GDK_ACTION_MOVE |
                    GDK_ACTION_COPY |
                    GDK_ACTION_LINK |
                    GDK_ACTION_ASK));

        target_list = gtk_target_list_new (targets, G_N_ELEMENTS (targets));
        gtk_target_list_add_uri_targets (target_list, NEMO_ICON_DND_URI_LIST);
        gtk_drag_source_set_target_list (getPtr(), target_list);
        gtk_target_list_unref (target_list);

        g_signal_connect (getPtr(), "drag-data-get",
                    G_CALLBACK (PathBarButton::button_drag_data_get_cb),
                    this);
    }

    void PathBarButton::setup_button_type(GFile *location, bool desktop_is_home)
    {
        if (nemo_is_root_directory (location))
            type = ButtonType::Root;
        else if (nemo_is_home_directory (location))
        {
            type = ButtonType::Home;
            fake_root = true;
        }
        else if (nemo_is_desktop_directory (location)) 
        {
            if (!desktop_is_home)
                type = ButtonType::Desktop;
            else
                type = ButtonType::Normal;
        }
        else if (xdg_documents_path != NULL && g_file_equal (location, xdg_documents_path))
            type = ButtonType::Xdg;
        else if (xdg_download_path != NULL && g_file_equal (location, xdg_download_path))
            type = ButtonType::Xdg;
        else if (xdg_music_path != NULL && g_file_equal (location, xdg_music_path))
            type = ButtonType::Xdg;
        else if (xdg_pictures_path != NULL && g_file_equal (location, xdg_pictures_path))
            type = ButtonType::Xdg;
        else if (xdg_templates_path != NULL && g_file_equal (location, xdg_templates_path))
            type = ButtonType::Xdg;
        else if (xdg_videos_path != NULL && g_file_equal (location, xdg_videos_path))
            type = ButtonType::Xdg;
        else if (xdg_public_path != NULL && g_file_equal (location, xdg_public_path))
            type = ButtonType::Xdg;
        else if (setup_file_path_mounted_mount (location))
        {
            /* already setup */
        }
        else
            type = ButtonType::Normal;
    }
    
    bool PathBarButton::setup_file_path_mounted_mount (GFile *location)
    {
        GVolumeMonitor *volume_monitor;
        GList *mounts, *l;
        GMount *mount;
        bool result;
        GFile *root, *default_location;

        result = false;
        volume_monitor = g_volume_monitor_get ();
        mounts = g_volume_monitor_get_mounts (volume_monitor);
        for (l = mounts; l != NULL; l = l->next)
        {
            mount = (GMount*)l->data;
            if (g_mount_is_shadowed (mount))
                continue;

            if (result)
                continue;

            root = g_mount_get_root (mount);
            if (g_file_equal (location, root)) 
            {
                result = true;
                /* set mount specific details in button_data */
                mount_icon_name = nemo_get_mount_icon_name (mount);
                dir_name = g_mount_get_name (mount);
                type = nemo::ButtonType::Mount;
                fake_root = true;
                g_object_unref (root);
                break;
            }
            default_location = g_mount_get_default_location (mount);
            if (!g_file_equal (default_location, root) &&
                g_file_equal (location, default_location))
            {
                result = true;
                /* set mount specific details in button_data */
                mount_icon_name = nemo_get_mount_icon_name (mount);
                type = nemo::ButtonType::DefaultLocation;
                fake_root = true;
                g_object_unref (default_location);
                g_object_unref (root);
                break;
            }
            g_object_unref (default_location);
            g_object_unref (root);
        }
        g_list_free_full (mounts, g_object_unref);
        return result;
    }

//Public Static
    void PathBarButton::button_drag_data_get_cb (GtkWidget *widget, GdkDragContext  *context, GtkSelectionData *selection_data, unsigned int info, unsigned int time_, gpointer user_data)
    {
        nemo::PathBarButton *button_data;
        char *uri_list[2];
        char *tmp;

        button_data = (nemo::PathBarButton*)user_data;

        uri_list[0] = g_file_get_uri (button_data->path);
        uri_list[1] = NULL;

        if (info == NEMO_ICON_DND_GNOME_ICON_LIST) 
        {
            tmp = g_strdup_printf ("%s\r\n", uri_list[0]);
            gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data),
                        8, (const unsigned char *) tmp, strlen (tmp));
            g_free (tmp);
        } 
        else if (info == NEMO_ICON_DND_URI_LIST)
            gtk_selection_data_set_uris (selection_data, uri_list);

        g_free (uri_list[0]);
    }
 }