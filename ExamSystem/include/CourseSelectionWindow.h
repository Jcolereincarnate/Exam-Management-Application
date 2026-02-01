#ifndef COURSE_SELECTION_WINDOW_H
#define COURSE_SELECTION_WINDOW_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <vector>
#include "Course.h"

class CourseSelectionWindow {
private:
    Fl_Window* window;
    Fl_Hold_Browser* courseBrowser;
    vector<Course> availableCourses;
    
    static void continueCallback(Fl_Widget* w, void* data);
    static void logoutCallback(Fl_Widget* w, void* data);
    
public:
    CourseSelectionWindow();
    ~CourseSelectionWindow();
    void show();
};

#endif
