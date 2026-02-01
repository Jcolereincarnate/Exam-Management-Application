#ifndef INSTRUCTIONS_WINDOW_H
#define INSTRUCTIONS_WINDOW_H

#include <FL/Fl_Window.H>

class InstructionsWindow {
private:
    Fl_Window* window;
    
    static void startExamCallback(Fl_Widget* w, void* data);
    static void backCallback(Fl_Widget* w, void* data);
    
public:
    InstructionsWindow();
    ~InstructionsWindow();
    void show();
};

#endif