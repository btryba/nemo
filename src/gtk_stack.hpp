#ifndef __GTK_STACK_HPP__
#define __GTK_STACK_HPP__

#include "gtk_container.hpp"

namespace gtk
{
    class Stack : public Container
    {
        public:
        Stack();
        virtual ~Stack();
        void set_transition_type(GtkStackTransitionType type);
        void set_transition_duration(int duration);
        void add_named(Widget* childWidget, const char* childName);
        void add_named(GtkWidget* childWidget, const char* childName);
        void set_visible_child_name(const char* name);
    };
}

#endif //__GTK_STACK_HPP__