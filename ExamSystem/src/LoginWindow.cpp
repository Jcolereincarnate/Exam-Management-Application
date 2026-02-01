#include "LoginWindow.h"
#include "Globals.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <cstdlib>


void LoginWindow::loginCallback(Fl_Widget* w, void* data) {
    LoginWindow* login = (LoginWindow*)data;
    
    string userId = login->userIdInput->value();
    string password = login->passwordInput->value();
    
    if (userId.empty() || password.empty()) {
        fl_alert("Please enter both Username and Password!");
        return;
    }
    
    currentUser = dbManager->authenticateUser(userId, password);
    
    if (currentUser != NULL) {
        login->window->hide();
        delete login;
        
        if (currentUser->role == "admin") {
            showAdminDashboard();
        } else {
            showCourseSelectionWindow();
        }
    } else {
        fl_alert("Invalid credentials or account locked!\n\nDefault accounts:\nAdmin: admin/admin123\nStudent: student1/pass123");
    }
}

void LoginWindow::exitCallback(Fl_Widget* w, void* data) {
    exit(0);
}

// ============================================================================
// Constructor Implementation
// ============================================================================

LoginWindow::LoginWindow() {
    window = new Fl_Window(500, 450, "Course-Based Examination System");
    window->color(fl_rgb_color(240, 248, 255));
    
    Fl_Box* title = new Fl_Box(80, 30, 340, 50, "Course-Based Exam System");
    title->labelsize(18);
    title->labelfont(FL_BOLD);
    title->labelcolor(FL_BLUE);
    
    Fl_Box* subtitle = new Fl_Box(150, 75, 200, 30, "Secure Login Portal");
    subtitle->labelsize(14);
    subtitle->labelcolor(FL_DARK_BLUE);
    
    Fl_Box* features = new Fl_Box(50, 115, 400, 60);
    features->copy_label("Admin-Controlled Examination System\nCourse-Based | Secure | Professional");
    features->labelsize(11);
    features->labelcolor(FL_DARK_GREEN);
    features->box(FL_BORDER_BOX);
    
    Fl_Box* userLabel = new Fl_Box(80, 200, 100, 30, "Username:");
    userLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    userIdInput = new Fl_Input(200, 200, 220, 30);
    
    Fl_Box* passLabel = new Fl_Box(80, 250, 100, 30, "Password:");
    passLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    passwordInput = new Fl_Secret_Input(200, 250, 220, 30);
    
    Fl_Button* loginBtn = new Fl_Button(150, 310, 100, 40, "Login");
    loginBtn->color(FL_GREEN);
    loginBtn->labelsize(14);
    loginBtn->callback(loginCallback, this);
    
    Fl_Button* exitBtn = new Fl_Button(270, 310, 100, 40, "Exit");
    exitBtn->color(FL_RED);
    exitBtn->labelsize(14);
    exitBtn->callback(exitCallback, NULL);
    
    Fl_Box* info = new Fl_Box(50, 370, 400, 60);
    info->copy_label("Default Accounts:\nAdmin: admin/admin123\nStudent: student1/pass123");
    info->labelsize(11);
    info->labelcolor(fl_rgb_color(0, 100, 0));
    
    window->end();
}


LoginWindow::~LoginWindow() {
    delete window;
}


void LoginWindow::show() {
    window->show();
}