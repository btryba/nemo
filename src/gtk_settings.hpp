#ifndef __GTK_SETTINGS_HPP__
#define __GTK_SETTINGS_HPP__

extern "C"
{
    #include <gtk/gtk.h>
}

namespace gtk
{
    struct Widget;
    struct Settings
    {
        GtkSettings* settings;

        public:
        Settings(GtkSettings* original);
        unsigned int connect(const char* signal, GCallback callBack, Widget* widget);
    };
}

#endif //__GTK_SETTINGS_HPP__