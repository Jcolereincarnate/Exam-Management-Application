#ifndef RESULT_WINDOW_H
#define RESULT_WINDOW_H

#include <FL/Fl_Window.H>
#include "Result.h"

class ResultWindow {
private:
    Fl_Window* window;
    Result result;
    
    static void viewAnalyticsCallback(Fl_Widget* w, void* data);
    static void exitCallback(Fl_Widget* w, void* data);
    static void newExamCallback(Fl_Widget* w, void* data);
    
public:
    ResultWindow(Result r);
    ~ResultWindow();
    void show();
};

#endif