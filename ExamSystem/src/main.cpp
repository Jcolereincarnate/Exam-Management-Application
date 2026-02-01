

#include <FL/Fl.H>
#include "Globals.h"
#include "DatabaseManager.h"
#include "LoginWindow.h"
#include "AdminDashboard.h"
#include "CourseSelectionWindow.h"
#include "InstructionsWindow.h"
#include "ExamWindow.h"
#include "ResultWindow.h"

DatabaseManager* dbManager = NULL;
User* currentUser = NULL;
Course* selectedCourse = NULL;


void showLoginWindow() {
    LoginWindow* login = new LoginWindow();
    login->show();
}

void showAdminDashboard() {
    AdminDashboard* admin = new AdminDashboard();
    admin->show();
}

void showCourseSelectionWindow() {
    CourseSelectionWindow* csw = new CourseSelectionWindow();
    csw->show();
}

void showInstructionsWindow() {
    InstructionsWindow* inst = new InstructionsWindow();
    inst->show();
}

void showExamWindow() {
    ExamWindow* exam = new ExamWindow();
    exam->show();
}

void showResultWindow(Result result) {
    ResultWindow* resWin = new ResultWindow(result);
    resWin->show();
}



int main(int argc, char** argv) {
    // Initialize database manager
    dbManager = new DatabaseManager();
    
    if (!dbManager) {
        return 1;
    }
    
    // Show login window
    showLoginWindow();
    
    // Run FLTK event loop
    int result = Fl::run();
    
    // Cleanup
    if (dbManager) {
        delete dbManager;
    }
    
    if (currentUser) {
        delete currentUser;
    }
    
    if (selectedCourse) {
        delete selectedCourse;
    }
    
    return result;
}