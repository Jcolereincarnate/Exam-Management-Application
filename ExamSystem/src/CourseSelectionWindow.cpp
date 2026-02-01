#include "CourseSelectionWindow.h"
#include "Globals.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <cstdio>

void CourseSelectionWindow::continueCallback(Fl_Widget* w, void* data) {
    CourseSelectionWindow* csw = (CourseSelectionWindow*)data;
    
    int selected = csw->courseBrowser->value();
    if (selected <= 0) {
        fl_alert("Please select a course before continuing!");
        return;
    }
    
    int courseIdx = selected - 1;
    
    if (courseIdx >= 0 && courseIdx < (int)csw->availableCourses.size()) {
        if (selectedCourse != NULL) {
            delete selectedCourse;
            selectedCourse = NULL;
        }
        
        selectedCourse = new Course();
        *selectedCourse = csw->availableCourses[courseIdx];
        
        if (selectedCourse->totalQuestions < selectedCourse->questionsPerExam) {
            fl_alert("Error: Course %s has insufficient questions!\nRequired: %d, Available: %d",
                    selectedCourse->courseCode.c_str(),
                    selectedCourse->questionsPerExam,
                    selectedCourse->totalQuestions);
            delete selectedCourse;
            selectedCourse = NULL;
            return;
        }
        
        csw->window->hide();
        delete csw;
        showInstructionsWindow();
    }
}

void CourseSelectionWindow::logoutCallback(Fl_Widget* w, void* data) {
    CourseSelectionWindow* csw = (CourseSelectionWindow*)data;
    csw->window->hide();
    delete csw;
    currentUser = NULL;
    if (selectedCourse != NULL) {
        delete selectedCourse;
        selectedCourse = NULL;
    }
    showLoginWindow();
}

CourseSelectionWindow::CourseSelectionWindow() {
    window = new Fl_Window(700, 600, "Course Selection");
    window->color(FL_WHITE);
    
    Fl_Box* title = new Fl_Box(200, 20, 300, 40, "Select Course for Examination");
    title->labelsize(18);
    title->labelfont(FL_BOLD);
    title->labelcolor(FL_BLUE);
    
    Fl_Box* welcomeBox = new Fl_Box(50, 70, 600, 30);
    string welcome = "Welcome, " + currentUser->username + "!";
    welcomeBox->copy_label(welcome.c_str());
    welcomeBox->labelsize(14);
    welcomeBox->labelfont(FL_BOLD);
    
    Fl_Box* infoBox = new Fl_Box(50, 110, 600, 40);
    infoBox->copy_label("Select a course below to begin your examination.\nExam settings are controlled by the administrator.");
    infoBox->labelsize(12);
    
    courseBrowser = new Fl_Hold_Browser(50, 170, 600, 330);
    courseBrowser->textsize(12);
    
    availableCourses = dbManager->getAllCourses();
    
    if (availableCourses.empty()) {
        courseBrowser->add("@C1@bNo courses available");
        courseBrowser->add("");
        courseBrowser->add("Please contact your administrator to:");
        courseBrowser->add("1. Add courses in the Admin Dashboard");
        courseBrowser->add("2. Ensure the database is working correctly");
    } else {
        for (size_t i = 0; i < availableCourses.size(); i++) {
            char buffer[400];
            
            sprintf(buffer, "@B15@C4@b%s - %s", 
                   availableCourses[i].courseCode.c_str(),
                   availableCourses[i].courseTitle.c_str());
            courseBrowser->add(buffer);
            
            sprintf(buffer, "@.   Duration: %d minutes", 
                   availableCourses[i].timeAllocation);
            courseBrowser->add(buffer);
            
            sprintf(buffer, "@.   Questions: %d | Passing Mark: %d%%", 
                   availableCourses[i].questionsPerExam,
                   availableCourses[i].passingMark);
            courseBrowser->add(buffer);
            
            sprintf(buffer, "@.   Question Pool: %d questions available", 
                   availableCourses[i].totalQuestions);
            courseBrowser->add(buffer);
            
            if (availableCourses[i].totalQuestions >= availableCourses[i].questionsPerExam) {
                courseBrowser->add("@C2@.   Status: Ready to take exam");
            } else {
                sprintf(buffer, "@C1@.   Status: Not ready (needs %d more questions)", 
                       availableCourses[i].questionsPerExam - availableCourses[i].totalQuestions);
                courseBrowser->add(buffer);
            }
            
            courseBrowser->add("   ========================================");
        }
    }
    
    Fl_Button* continueBtn = new Fl_Button(250, 520, 200, 40, "Continue to Instructions");
    continueBtn->color(FL_GREEN);
    continueBtn->labelsize(13);
    continueBtn->labelfont(FL_BOLD);
    continueBtn->callback(continueCallback, this);
    
    Fl_Button* logoutBtn = new Fl_Button(550, 520, 100, 40, "Logout");
    logoutBtn->color(FL_RED);
    logoutBtn->callback(logoutCallback, this);
    
    window->end();
}

CourseSelectionWindow::~CourseSelectionWindow() {
    delete window;
}

void CourseSelectionWindow::show() {
    window->show();
}