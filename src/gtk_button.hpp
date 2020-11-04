#ifndef __GTK_BUTTON_HPP__
#define __GTK_BUTTON_HPP__

#include "gtk_container.hpp"
#include <string>

namespace gtk
{
    enum class ButtonEventType
    {
        Clicked,
        ButtonPressed,
        ButtonReleased,
        DragMotion,
        DragLeave
    };
    class ButtonEventTypeReader
    {
        public:
        static const char* toChars(ButtonEventType eventType)
        {
            if(eventType == ButtonEventType::Clicked)
                return "clicked";
            else if(eventType == ButtonEventType::ButtonPressed)
                return "button_press_event";
            else if(eventType == ButtonEventType::ButtonReleased)
                return "button_release_event";
            else if(eventType == ButtonEventType::DragMotion)
                return "drag_motion";
            else if(eventType == ButtonEventType::DragLeave)
                return "drag_leave";
            return "";
        }
    };

    class Button : public Container
    {
        public:

        Button(const char *name, GtkActionGroup* actionGroup);
        Button();
        Button(GtkButton* original, bool callAddReference = true);
        virtual ~Button();
        void set_focus_on_click(bool setFocus);
        /* If includeCaller signature guidelines:
        true = (GtkWidget *widget, GdkEventButton *event, calleeType* callee)
        false = (calleeType* calee)*/
        template<typename callbackType, typename calleeType>
        void connect(ButtonEventType eventType, callbackType callbackFunction, calleeType* callee, bool includeCaller = true)
        {
            if(includeCaller)
                g_signal_connect(widget, ButtonEventTypeReader::toChars(eventType), G_CALLBACK (callbackFunction), callee);
            else
                g_signal_connect_swapped(widget, ButtonEventTypeReader::toChars(eventType), G_CALLBACK (callbackFunction), callee);
        }
        void set_label(const char* label);
    };
}

#endif //__GTK_BUTTON_HPP__