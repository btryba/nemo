#include "nemo-pathbar-button.hpp"

extern "C" 
{
    #include <libnemo-private/nemo-file.h>
    #include <libnemo-private/nemo-file-utilities.h>
    #include <glib/gi18n.h> //Underscore translations
    #include <libnemo-private/nemo-icon-dnd.h>
    #include <libnemo-private/nemo-icon-names.h>
}
namespace nemo
{
    PathBarButton::PathBarButton(NemoFile& file) : mount_icon_name{nullptr}, dir_name{nullptr}
    {
        path.updatePath((GFile*)nemo_file_get_location(&file));
        setType();
        get_style_context()->add_class("text-button");
        set_focus_on_click(false);
        add_events(GDK_SCROLL_MASK);

        /* TODO update button type when xdg directories change */ 

        GtkWidget *child = nullptr;

        gtk::Image image{};

        switch (button->type)
        {
            case ButtonType::Root:
                child = button_data->image;
                button->label = nullptr;
                break;
            case ButtonType::Home:
            case ButtonType::Desktop:
            case ButtonType::Mount:
            case ButtonType::DefaultLocation:
                button->label = gtk_label_new (NULL);
                child = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
                gtk_box_pack_start (GTK_BOX (child), button_data->image, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (child), button_data->label, FALSE, FALSE, 0);
                break;
            case ButtonType::Xdg:
            case ButtonType::Normal:
            default:
                button_data->label = gtk_label_new (NULL);
                child = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
                gtk_box_pack_start (GTK_BOX (child), button_data->image, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (child), button_data->label, FALSE, FALSE, 0);
                button_data->is_base_dir = base_dir;
        }
        if (button_data->label != NULL) {
            gtk_label_set_ellipsize (GTK_LABEL (button_data->label), PANGO_ELLIPSIZE_MIDDLE);
        }

        if (button_data->path == nullptr)
            button_data->path->updatePath(path);

        if (button_data->dir_name == nullptr)
            button_data->dir_name = nemo_file_get_display_name (file);

        if (button_data->file == nullptr)
        {
            button_data->file = nemo_file_ref (file);
            nemo_file_monitor_add (button_data->file, button_data,
                        NEMO_FILE_ATTRIBUTES_FOR_ICON);
            button_data->file_changed_signal_id =
                g_signal_connect (button_data->file, "changed",
                        G_CALLBACK (button_data_file_changed),
                        button_data);
        }

        gtk_container_add (GTK_CONTAINER (button_data->button), child);
        gtk_widget_show_all (button_data->button);

        nemo_path_bar_update_button_state (button_data, current_dir);

        g_signal_connect (button_data->button, "clicked", G_CALLBACK (button_clicked_cb), button_data);
        g_object_weak_ref (G_OBJECT (button_data->button), (GWeakNotify) button_data_free, button_data);

        char *uri = g_file_get_uri (path);

        if (!eel_uri_is_search (uri)) {
            setup_button_drag_source (button_data);
        }

        g_clear_pointer (&uri, g_free);

        nemo_drag_slot_proxy_init (button_data->button, button_data->file, NULL);

        g_object_unref (path);


    }

    void PathBarButton::setType()
    {
        if (nemo_is_root_directory (path.getPtr()))
            type = ButtonType::Root;
        else if (nemo_is_home_directory (path.getPtr()))
        {
            type = ButtonType::Home;
            fake_root = true;
        }
        else if (nemo_is_desktop_directory (path.getPtr()))
        {
            if (!desktop_is_home)
                type = ButtonType::Desktop;
            else
                type = ButtonType::Normal;
        } 
        else if (path == gtk::File{G_USER_DIRECTORY_DOCUMENTS} ||
            path == gtk::File{G_USER_DIRECTORY_DOWNLOAD} ||
            path == gtk::File{G_USER_DIRECTORY_MUSIC} ||
            path == gtk::File{G_USER_DIRECTORY_PICTURES} ||
            path == gtk::File{G_USER_DIRECTORY_PUBLIC_SHARE} ||
            path == gtk::File{G_USER_DIRECTORY_TEMPLATES} ||
            path == gtk::File{G_USER_DIRECTORY_VIDEOS}) //NOTICE All nullptr checking on object classes removed
                type = ButtonType::Xdg;
        else if (setup_file_path_mounted_mount())
        {
            /* already setup */
        }
        else
            type = ButtonType::Normal;   
    }

    bool PathBarButton::setup_file_path_mounted_mount()
    {
        bool result = false;
        GList *mounts = g_volume_monitor_get_mounts (g_volume_monitor_get ());
        for (GList *list = mounts; list != nullptr; list = list->next)
        {
            GMount *mount = (GMount*)list->data;
            if (g_mount_is_shadowed (mount))
                continue;

            gtk::File root{g_mount_get_root (mount), false};
            if (path == root)
            {
                result = true;
                /* set mount specific details in button_data */
                mount_icon_name = nemo_get_mount_icon_name (mount);
                dir_name = g_mount_get_name (mount);
                type = ButtonType::Mount;
                fake_root = true;
                break;
            }
            gtk::File default_location{g_mount_get_default_location (mount), false};
            if (path == default_location)
            {
                result = true;
                /* set mount specific details in button_data */
                mount_icon_name = nemo_get_mount_icon_name (mount);
                type = ButtonType::DefaultLocation;
                fake_root = true;
                break;
            }
        }
        g_list_free_full (mounts, g_object_unref);
        return result;
    }

    const char* PathBarButton::get_dir_name()
    {
        if(type == ButtonType::Desktop)
            return _("Desktop");
            /*
            }
            * originally this would look like /home/Home/Desktop in the pathbar.
            * I can see the logic, when you're only staying in $HOME, i.e. Home/Desktop
            * but when you've come from a directory further up it just looks wrong. *
            } else if (button_data->type == HOME_BUTTON) {
                return _("Home");*/
        else
            return dir_name;
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

        drag_source_set((GdkModifierType)(GDK_BUTTON1_MASK | GDK_BUTTON2_MASK),
            nullptr, 0,
            (GdkDragAction)(GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_ASK));

        target_list = gtk_target_list_new (targets, G_N_ELEMENTS (targets));
        gtk_target_list_add_uri_targets (target_list, NEMO_ICON_DND_URI_LIST);
        gtk_drag_source_set_target_list (button_data->button, target_list);
        gtk_target_list_unref (target_list);

        g_signal_connect (button_data->button, "drag-data-get",
                    G_CALLBACK (button_drag_data_get_cb),
                    button_data);
    }

    void PathBarButton::drag_data_get_cb (GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, unsigned int info, unsigned int time_, gpointer user_data)
    {
        PathBarButton *button = (PathBarButton*)user_data;

        char *uri_list[2];
        uri_list[0] = button->path.get_uri();
        uri_list[1] = nullptr;

        if (info == NEMO_ICON_DND_GNOME_ICON_LIST)
        {
            char *tmp = g_strdup_printf ("%s\r\n", uri_list[0]);
            gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data),
                        8, (const guchar *) tmp, strlen (tmp));
            g_free (tmp);
        } 
        else if (info == NEMO_ICON_DND_URI_LIST)
            gtk_selection_data_set_uris (selection_data, uri_list);

        g_free (uri_list[0]);
    }

    void PathBarButton::update_button_appearance()
    {
        const char *dir_name = get_dir_name();
        if (!label->isNull())
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

        char *icon_name = nullptr;

        if (button_data->image != NULL)
        {
            switch (type) {
                case ButtonType::Root:
                    icon_name = g_strdup (NEMO_ICON_SYMBOLIC_FILESYSTEM);
                    break;
                case ButtonType::Home:
                case ButtonType::Desktop:
                case ButtonType::Xdg:
                    icon_name = nemo_file_get_control_icon_name (file);
                    break;
                case ButtonType::Normal:
                    if (button_data->is_base_dir) {
                        icon_name = nemo_file_get_control_icon_name (file);
                        break;
                    }
                case ButtonType::DefaultLocation:
                case ButtonType::Mount:
                    if (button_data->mount_icon_name) {
                        icon_name = g_strdup (button_data->mount_icon_name);
                        break;
                    }
                default:
                    icon_name = nullptr;
            }

            gtk_image_set_from_icon_name (GTK_IMAGE (button_data->image),
                                        icon_name,
                                        GTK_ICON_SIZE_MENU);

            g_free (icon_name);
        }

    }

    void PathBarButton::update_button_state(bool current_dir)
    {
        if (!label->isNull())
        {
            label->set_label(nullptr);
            label->set_use_markup(current_dir);
        }

        update_button_appearance();

        if (get_active() != current_dir) {
            ignore_changes = true;
            set_active(current_dir);
            ignore_changes = false;
        }
    }

    PathBarButton::~PathBarButton()
    {
        g_free(dir_name);
        g_clear_pointer(&button_data->mount_icon_name, g_free);
    }

    void PathBarButton::button_data_free()
    {
        

        

        if (button_data->file != NULL) {
            g_signal_handler_disconnect (button_data->file,
                            button_data->file_changed_signal_id);
            nemo_file_monitor_remove (button_data->file, button_data);
            nemo_file_unref (button_data->file);
        }
    }

    void PathBarButton::button_clicked_implementation()
    {
        GList *button_list;

        if (ignore_changes)
            return;

        path_bar = NEMO_PATH_BAR (gtk_widget_get_parent (button));

        button_list = g_list_find (path_bar->priv->button_list, button_data);
        g_assert (button_list != NULL);

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

        g_signal_emit (path_bar, path_bar_signals [PATH_CLICKED], 0, button_data->path);
    }

    void PathBarButton::button_clicked_cb (GtkWidget *button, gpointer   data)
    {
        ((PathBarButton*)data)->button_clicked_implementation();
    }

    void PathBarButton::button_data_file_changed (NemoFile *file)
    {
        PathBarButton *current_button_data = parent->current_button_data;

        gtk::File location{nemo_file_get_location(file), false}
        if (!g_file_equal (button_data->path, location))
        {
            gtk::File *parent = g_file_get_parent (location);
            gtk::File *button_parent = g_file_get_parent (button_data->path);

            bool renamed = (parent != nullptr && button_parent != nullptr) &&
                parent == button_parent);

            if (parent != NULL) {
                g_object_unref (parent);
            }
            if (button_parent != NULL) {
                g_object_unref (button_parent);
            }

            if (renamed)
                button_data->path->updatePath(location);
            else 
            {
                /* the file has been moved.
                * If it was below the currently displayed location, remove it.
                * If it was not below the currently displayed location, update the path bar
                */
                bool child = button_data->path == getCppObject(path_bar)->current_path;
                gtk::File *current_location;
                if (child)
                {
                    /* moved file inside current path hierarchy */
                    g_object_unref (location);
                    location = g_file_get_parent (button_data->path);
                    current_location->updatePath(getCppObject(path_bar)->current_path);
                } 
                else 
                {
                    /* moved current path, or file outside current path hierarchy.
                    * Update path bar to new locations.
                    */
                    current_location = nemo_file_get_location (current_button_data->file);
                }

                    nemo_path_bar_update_path (path_bar, location, FALSE);
                    nemo_path_bar_set_path (path_bar, current_location);
                g_object_unref (location);
                g_object_unref (current_location);
                return;
            }
        } 
        else if (nemo_file_is_gone (file)) 
        {
            int idx, position;

            /* if the current or a parent location are gone, don't do anything, as the view
            * will get the event too and call us back.
            */
            current_location = nemo_file_get_location (current_button_data->file);

            if (g_file_has_prefix (location, current_location)) {
                /* remove this and the following buttons */
                position = g_list_position (path_bar->priv->button_list,
                        g_list_find (path_bar->priv->button_list, button_data));

                if (position != -1) {
                    for (idx = 0; idx <= position; idx++) {
                        gtk_container_remove (GTK_CONTAINER (path_bar),
                                    BUTTON_DATA (path_bar->priv->button_list->data)->button);
                    }
                }
            }

            g_object_unref (current_location);
            return;
        }

        /* MOUNTs use the GMount as the name, so don't update for those */
        if (button_data->type != MOUNT_BUTTON) 
        {
            char* display_name = nemo_file_get_display_name (file);
            if (g_strcmp0 (display_name, button_data->dir_name) != 0) {
                g_free (button_data->dir_name);
                button_data->dir_name = g_strdup (display_name);
            }

            g_free (display_name);
        }
        update_button_appearance();
    }

}