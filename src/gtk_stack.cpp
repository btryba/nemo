#include "gtk_stack.hpp"

namespace gtk
{
    Stack::Stack()
    {
        widget = gtk_stack_new();
    }

    Stack::~Stack(){}

    void Stack::set_transition_type(GtkStackTransitionType type)
    {
        gtk_stack_set_transition_type((GtkStack*)widget, type);
    }

    void Stack::set_transition_duration(int duration)
    {
        gtk_stack_set_transition_duration((GtkStack*)widget, duration);
    }

    void Stack::add_named(Widget* childWidget, const char* childName)
    {
        gtk_stack_add_named((GtkStack*)widget, childWidget->getPtr(), childName);
    }

    void Stack::add_named(GtkWidget* childWidget, const char* childName)
    {
        gtk_stack_add_named((GtkStack*)widget, childWidget, childName);
    }

    void Stack::set_visible_child_name(const char* name)
    {
        gtk_stack_set_visible_child_name ((GtkStack*)widget, name);
    }
}