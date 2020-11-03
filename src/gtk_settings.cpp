#include "gtk_settings.hpp"
#include "gtk_widget.hpp"

namespace gtk
{
    Settings::Settings(GtkSettings* original) : settings{original}{}

    unsigned int Settings::connect(const char* signal, GCallback callBack, Widget* widget)
    {
        return g_signal_connect(settings, signal, G_CALLBACK(callBack), widget->getPtr());
    }
}