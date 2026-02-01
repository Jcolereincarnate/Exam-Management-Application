#ifndef GLOBALS_H
#define GLOBALS_H

#include "DatabaseManager.h"
#include "User.h"
#include "Course.h"

// Forward declarations for window functions
void showLoginWindow();
void showAdminDashboard();
void showCourseSelectionWindow();
void showInstructionsWindow();
void showExamWindow();
void showResultWindow(Result result);

// Global variables
extern DatabaseManager* dbManager;
extern User* currentUser;
extern Course* selectedCourse;

#endif