#include "gtk_image.hpp"

namespace gtk
{
    Image::Image(const char* name, GtkIconSize iconSize) 
        : Widget{(GtkWidget*)gtk_image_new_from_icon_name(name, iconSize), false} {}

    Image::Image(GtkImage* original, bool callAddReference)
        : Widget((GtkWidget*)original, callAddReference){}

    Image::Image() : Widget{(GtkWidget*)gtk_image_new(), false} {}
}