#ifndef __GTK_STYLECONTEXT_HPP__
#define __GTK_STYLECONTEXT_HPP__

extern "C"
{
    #include <gtk/gtk.h>
}

namespace gtk
{
    class StyleContext
    {
        GtkStyleContext* context;
        
        public:
        StyleContext(GtkStyleContext* context);
        virtual ~StyleContext();
        void add_class(const char* className);
        void invalidate();
    };
}

#endif //__GTK_STYLECONTEXT_HPP__