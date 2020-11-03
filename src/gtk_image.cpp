#include "gtk_image.hpp"

namespace gtk
{
    Image::Image(const char* name, GtkIconSize iconSize)
    {
        widget = gtk_image_new_from_icon_name(name, iconSize);
    }

    Image::Image(GtkImage* original, bool callAddReference)
        : Widget((GtkWidget*)original, callAddReference){}

    Image::Image()
    {
        widget = gtk_image_new();
    }
}