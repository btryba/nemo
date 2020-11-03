#ifndef __GTK_IMAGE_HPP__
#define __GTK_IMAGE_HPP__

#include "gtk_widget.hpp"

namespace gtk
{
    struct Image : public Widget
    {
        public:
        Image();
        Image(const char* name, GtkIconSize iconSize);
        Image(GtkImage* original, bool callAddReference = true);
    };
}

#endif //__GTK_IMAGE_HPP__