#include "gtk_image.hpp"

namespace gtk
{
    Image::Image(const char* name, GtkIconSize iconSize)
    {
        widget = gtk_image_new_from_icon_name(name, iconSize);
    }
}