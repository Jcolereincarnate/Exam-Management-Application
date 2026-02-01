#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>

class LoginWindow {
private:
    Fl_Window* window;
    Fl_Input* userIdInput;
    Fl_Secret_Input* passwordInput;
    
    static void loginCallback(Fl_Widget* w, void* data);
    static void exitCallback(Fl_Widget* w, void* data);
    
public:
    LoginWindow();
    ~LoginWindow();
    void show();
};

#endif