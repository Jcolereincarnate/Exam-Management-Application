#ifndef ADMIN_DASHBOARD_H
#define ADMIN_DASHBOARD_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Value_Input.H>

class AdminDashboard {
private:
    Fl_Window* window;
    Fl_Tabs* tabs;
    
    // Course management widgets
    Fl_Input* courseCodeInput;
    Fl_Input* courseTitleInput;
    Fl_Value_Input* timeInput;
    Fl_Value_Input* questionsCountInput;
    Fl_Value_Input* passingMarkInput;
    Fl_Browser* courseBrowser;
    
    // Question management widgets
    Fl_Choice* courseChoice;
    Fl_Multiline_Input* questionInput;
    Fl_Input* optAInput;
    Fl_Input* optBInput;
    Fl_Input* optCInput;
    Fl_Input* optDInput;
    Fl_Choice* correctChoice;
    Fl_Browser* questionBrowser;
    
    // Results widgets
    Fl_Browser* resultsBrowser;
    
    // Callbacks
    static void addCourseCallback(Fl_Widget* w, void* data);
    static void refreshCoursesCallback(Fl_Widget* w, void* data);
    static void debugDatabaseCallback(Fl_Widget* w, void* data);
    static void addQuestionCallback(Fl_Widget* w, void* data);
    static void uploadQuestionsCallback(Fl_Widget* w, void* data);
    static void refreshQuestionsCallback(Fl_Widget* w, void* data);
    static void viewResultsCallback(Fl_Widget* w, void* data);
    static void addUserCallback(Fl_Widget* w, void* data);
    static void logoutCallback(Fl_Widget* w, void* data);
    
    // Helper functions
    void clearCourseFields();
    void clearQuestionFields();
    void refreshCourseBrowser();
    void refreshCourseChoice();
    void refreshQuestionBrowser();
    void refreshResults();
    int uploadQuestionsFromFile(const char* filename, int courseId);
    
public:
    AdminDashboard();
    ~AdminDashboard();
    void show();
};

#endif
