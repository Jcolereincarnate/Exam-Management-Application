#include "AdminDashboard.h"
#include "Globals.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <sstream>
#include <fstream>
#include <cstdio>

using namespace std;

void AdminDashboard::addCourseCallback(Fl_Widget* w, void* data) {
    AdminDashboard* panel = (AdminDashboard*)data;
    string code = panel->courseCodeInput->value();
    string title = panel->courseTitleInput->value();
    int time = (int)panel->timeInput->value();
    int qCount = (int)panel->questionsCountInput->value();
    int passing = (int)panel->passingMarkInput->value();
    
    if (code.empty() || title.empty()) {
        fl_alert("Please fill Course Code and Title!");
        return;
    }
    
    if (dbManager->addCourse(code, title, time, qCount, passing)) {
        fl_message("Course added successfully!");
        panel->clearCourseFields();
        panel->refreshCourseBrowser();
        panel->refreshCourseChoice();
    } else {
        fl_alert("Failed to add course! Code may already exist.");
    }
}

void AdminDashboard::refreshCoursesCallback(Fl_Widget* w, void* data) {
    AdminDashboard* panel = (AdminDashboard*)data;
    panel->refreshCourseBrowser();
    panel->refreshCourseChoice();
}

void AdminDashboard::debugDatabaseCallback(Fl_Widget* w, void* data) {
    AdminDashboard* panel = (AdminDashboard*)data;
    
    vector<Course> courses = dbManager->getAllCourses();
    
    stringstream debug;
    debug << "DATABASE DEBUG INFORMATION\n";
    debug << "===========================\n\n";
    debug << "Total Courses Found: " << courses.size() << "\n\n";
    
    if (courses.empty()) {
        debug << "NO COURSES IN DATABASE!\n\n";
        debug << "This could mean:\n";
        debug << "1. Courses table is empty\n";
        debug << "2. Database file is not being read\n";
        debug << "3. SQL query has an error\n";
    } else {
        debug << "Course Details:\n";
        debug << "---------------\n";
        for (size_t i = 0; i < courses.size(); i++) {
            debug << "Course " << (i+1) << ":\n";
            debug << "  ID: " << courses[i].id << "\n";
            debug << "  Code: " << courses[i].courseCode << "\n";
            debug << "  Title: " << courses[i].courseTitle << "\n";
            debug << "  Time: " << courses[i].timeAllocation << " min\n";
            debug << "  Questions: " << courses[i].questionsPerExam << "\n";
            debug << "  Pass Mark: " << courses[i].passingMark << "%\n";
            debug << "  Pool Size: " << courses[i].totalQuestions << "\n\n";
        }
    }
    
    fl_message("%s", debug.str().c_str());
    
    panel->refreshCourseBrowser();
    panel->refreshCourseChoice();
}

void AdminDashboard::addQuestionCallback(Fl_Widget* w, void* data) {
    AdminDashboard* panel = (AdminDashboard*)data;
    int courseIdx = panel->courseChoice->value();
    if (courseIdx < 0) {
        fl_alert("Please select a course first!");
        return;
    }
    
    string courseText = panel->courseChoice->text(courseIdx);
    stringstream ss(courseText);
    string code;
    ss >> code;
    
    int courseId = dbManager->getCourseIdByCode(code);
    if (courseId <= 0) {
        fl_alert("Invalid course!");
        return;
    }
    
    string question = panel->questionInput->value();
    string optA = panel->optAInput->value();
    string optB = panel->optBInput->value();
    string optC = panel->optCInput->value();
    string optD = panel->optDInput->value();
    int correctIdx = panel->correctChoice->value();
    
    if (question.empty() || optA.empty() || optB.empty() || 
        optC.empty() || optD.empty() || correctIdx < 0) {
        fl_alert("Please fill all fields!");
        return;
    }
    
    char correct = 'A' + correctIdx;
    string correctStr(1, correct);
    
    if (dbManager->addQuestion(courseId, question, optA, optB, optC, optD, correctStr, 1)) {
        fl_message("Question added successfully!");
        panel->clearQuestionFields();
        panel->refreshQuestionBrowser();
        panel->refreshCourseBrowser();
    } else {
        fl_alert("Failed to add question!");
    }
}

void AdminDashboard::uploadQuestionsCallback(Fl_Widget* w, void* data) {
    AdminDashboard* panel = (AdminDashboard*)data;
    
    int courseIdx = panel->courseChoice->value();
    if (courseIdx < 0) {
        fl_alert("Please select a course first!");
        return;
    }
    
    string courseText = panel->courseChoice->text(courseIdx);
    stringstream ss(courseText);
    string code;
    ss >> code;
    
    int courseId = dbManager->getCourseIdByCode(code);
    if (courseId <= 0) {
        fl_alert("Invalid course!");
        return;
    }
    
    const char* filename = fl_file_chooser("Select Questions File", "*.txt", "");
    if (!filename) return;
    
    int count = panel->uploadQuestionsFromFile(filename, courseId);
    
    if (count > 0) {
        char msg[200];
        sprintf(msg, "Successfully uploaded %d questions!", count);
        fl_message(msg);
        panel->refreshQuestionBrowser();
        panel->refreshCourseBrowser();
    } else {
        fl_alert("Failed to upload questions or file format incorrect!");
    }
}

void AdminDashboard::refreshQuestionsCallback(Fl_Widget* w, void* data) {
    AdminDashboard* panel = (AdminDashboard*)data;
    panel->refreshQuestionBrowser();
}

void AdminDashboard::viewResultsCallback(Fl_Widget* w, void* data) {
    AdminDashboard* panel = (AdminDashboard*)data;
    panel->refreshResults();
}

void AdminDashboard::addUserCallback(Fl_Widget* w, void* data) {
    const char* username = fl_input("Enter new username:");
    if (!username || strlen(username) == 0) return;
    
    const char* password = fl_password("Enter password:");
    if (!password || strlen(password) == 0) return;
    
    const char* role = fl_input("Enter role (admin/candidate):", "candidate");
    if (!role || strlen(role) == 0) return;
    
    if (dbManager->addUser(username, password, role)) {
        fl_message("User created successfully!");
    } else {
        fl_alert("Failed to create user. Username may already exist.");
    }
}

void AdminDashboard::logoutCallback(Fl_Widget* w, void* data) {
    AdminDashboard* panel = (AdminDashboard*)data;
    panel->window->hide();
    delete panel;
    currentUser = NULL;
    showLoginWindow();
}

// ============================================================================
// Helper Function Implementations
// ============================================================================

void AdminDashboard::clearCourseFields() {
    courseCodeInput->value("");
    courseTitleInput->value("");
    timeInput->value(60);
    questionsCountInput->value(40);
    passingMarkInput->value(40);
}

void AdminDashboard::clearQuestionFields() {
    questionInput->value("");
    optAInput->value("");
    optBInput->value("");
    optCInput->value("");
    optDInput->value("");
    correctChoice->value(0);
}

void AdminDashboard::refreshCourseBrowser() {
    courseBrowser->clear();
    vector<Course> courses = dbManager->getAllCourses();
    
    courseBrowser->add("=== COURSES IN SYSTEM ===");
    courseBrowser->add("");
    
    for (size_t i = 0; i < courses.size(); i++) {
        char buffer[300];
        sprintf(buffer, "%s - %s", courses[i].courseCode.c_str(), 
               courses[i].courseTitle.c_str());
        courseBrowser->add(buffer);
        
        sprintf(buffer, "  Time: %d min | Questions: %d (Pool: %d) | Pass: %d%%",
               courses[i].timeAllocation, courses[i].questionsPerExam,
               courses[i].totalQuestions, courses[i].passingMark);
        courseBrowser->add(buffer);
        courseBrowser->add("---");
    }
}

void AdminDashboard::refreshCourseChoice() {
    courseChoice->clear();
    vector<Course> courses = dbManager->getAllCourses();
    
    for (size_t i = 0; i < courses.size(); i++) {
        string item = courses[i].courseCode + " - " + courses[i].courseTitle;
        courseChoice->add(item.c_str());
    }
    
    if (courses.size() > 0) {
        courseChoice->value(0);
    }
}

void AdminDashboard::refreshQuestionBrowser() {
    questionBrowser->clear();
    
    int courseIdx = courseChoice->value();
    if (courseIdx < 0) {
        questionBrowser->add("Please select a course");
        return;
    }
    
    string courseText = courseChoice->text(courseIdx);
    stringstream ss(courseText);
    string code;
    ss >> code;
    
    int courseId = dbManager->getCourseIdByCode(code);
    Course* course = dbManager->getCourseById(courseId);
    
    if (course) {
        char buffer[200];
        sprintf(buffer, "Questions for: %s", course->courseCode.c_str());
        questionBrowser->add(buffer);
        sprintf(buffer, "Total Questions: %d", course->totalQuestions);
        questionBrowser->add(buffer);
        questionBrowser->add("---");
        delete course;
    }
}

void AdminDashboard::refreshResults() {
    resultsBrowser->clear();
    vector<Result> results = dbManager->getResults();
    
    resultsBrowser->add("=== ALL EXAMINATION RESULTS ===");
    resultsBrowser->add("");
    
    for (size_t i = 0; i < results.size(); i++) {
        char buffer[400];
        sprintf(buffer, "%s - %s (%s)", 
               results[i].username.c_str(),
               results[i].courseCode.c_str(),
               results[i].courseTitle.c_str());
        resultsBrowser->add(buffer);
        
        sprintf(buffer, "  Date: %s", results[i].dateTime.c_str());
        resultsBrowser->add(buffer);
        
        sprintf(buffer, "  Score: %d/%d (%.1f%%) - %s", 
               results[i].score, results[i].totalPoints,
               results[i].percentage,
               results[i].passed ? "PASS" : "FAIL");
        resultsBrowser->add(buffer);
        resultsBrowser->add("---");
    }
}

int AdminDashboard::uploadQuestionsFromFile(const char* filename, int courseId) {
    ifstream file(filename);
    if (!file.is_open()) {
        return 0;
    }
    
    int count = 0;
    string line;
    
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        if (line.substr(0, 2) == "Q:") {
            string question = line.substr(2);
            if (!question.empty() && question[0] == ' ') {
                question = question.substr(1);
            }
            
            string optA, optB, optC, optD, correct;
            
            if (getline(file, line) && line.substr(0, 2) == "A:") {
                optA = line.substr(2);
                if (!optA.empty() && optA[0] == ' ') optA = optA.substr(1);
            }
            
            if (getline(file, line) && line.substr(0, 2) == "B:") {
                optB = line.substr(2);
                if (!optB.empty() && optB[0] == ' ') optB = optB.substr(1);
            }
            
            if (getline(file, line) && line.substr(0, 2) == "C:") {
                optC = line.substr(2);
                if (!optC.empty() && optC[0] == ' ') optC = optC.substr(1);
            }
            
            if (getline(file, line) && line.substr(0, 2) == "D:") {
                optD = line.substr(2);
                if (!optD.empty() && optD[0] == ' ') optD = optD.substr(1);
            }
            
            if (getline(file, line) && line.substr(0, 7) == "ANSWER:") {
                correct = line.substr(7);
                if (!correct.empty() && correct[0] == ' ') correct = correct.substr(1);
                if (!correct.empty()) correct = correct.substr(0, 1);
            }
            
            if (!question.empty() && !optA.empty() && !optB.empty() && 
                !optC.empty() && !optD.empty() && !correct.empty()) {
                
                if (dbManager->addQuestion(courseId, question, optA, optB, 
                                          optC, optD, correct, 1)) {
                    count++;
                }
            }
        }
    }
    
    file.close();
    return count;
}

// ============================================================================
// Constructor Implementation
// ============================================================================

AdminDashboard::AdminDashboard() {
    window = new Fl_Window(950, 700, "Admin Dashboard");
    window->color(fl_rgb_color(240, 245, 250));
    
    Fl_Box* mainTitle = new Fl_Box(300, 10, 350, 35, "Administrator Dashboard");
    mainTitle->labelsize(18);
    mainTitle->labelfont(FL_BOLD);
    mainTitle->labelcolor(FL_BLUE);
    
    Fl_Button* logoutBtn = new Fl_Button(830, 10, 100, 35, "Logout");
    logoutBtn->color(FL_RED);
    logoutBtn->callback(logoutCallback, this);
    
    tabs = new Fl_Tabs(10, 60, 930, 620);
    
    // COURSE MANAGEMENT TAB
    Fl_Group* courseTab = new Fl_Group(10, 85, 930, 595, "Manage Courses");
    courseTab->color(FL_WHITE);
    
    Fl_Box* courseTitle = new Fl_Box(350, 100, 250, 30, "Course Management");
    courseTitle->labelsize(16);
    courseTitle->labelfont(FL_BOLD);
    
    courseCodeInput = new Fl_Input(150, 145, 150, 25, "Course Code:");
    courseTitleInput = new Fl_Input(150, 180, 350, 25, "Course Title:");
    
    timeInput = new Fl_Value_Input(150, 215, 100, 25, "Time (min):");
    timeInput->minimum(30);
    timeInput->maximum(180);
    timeInput->value(60);
    timeInput->step(5);
    
    questionsCountInput = new Fl_Value_Input(150, 250, 100, 25, "Questions:");
    questionsCountInput->minimum(10);
    questionsCountInput->maximum(100);
    questionsCountInput->value(40);
    questionsCountInput->step(5);
    
    passingMarkInput = new Fl_Value_Input(150, 285, 100, 25, "Pass Mark %:");
    passingMarkInput->minimum(0);
    passingMarkInput->maximum(100);
    passingMarkInput->value(40);
    passingMarkInput->step(5);
    
    Fl_Button* addCourseBtn = new Fl_Button(150, 325, 150, 35, "Add Course");
    addCourseBtn->color(FL_GREEN);
    addCourseBtn->callback(addCourseCallback, this);
    
    Fl_Button* refreshCourseBtn = new Fl_Button(320, 325, 150, 35, "Refresh List");
    refreshCourseBtn->color(fl_rgb_color(100, 149, 237));
    refreshCourseBtn->callback(refreshCoursesCallback, this);

    courseBrowser = new Fl_Browser(30, 380, 890, 280);
    
    courseTab->end();
    
    // QUESTION MANAGEMENT TAB
    Fl_Group* questionTab = new Fl_Group(10, 85, 930, 595, "Add Questions");
    questionTab->color(FL_WHITE);
    questionTab->hide();
    
    Fl_Box* qTitle = new Fl_Box(350, 100, 250, 30, "Question Management");
    qTitle->labelsize(16);
    qTitle->labelfont(FL_BOLD);
    
    Fl_Box* courseLabel = new Fl_Box(30, 145, 100, 25, "Select Course:");
    courseLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    courseChoice = new Fl_Choice(140, 145, 350, 25);
    
    Fl_Button* refreshQBtn = new Fl_Button(510, 145, 120, 25, "Refresh");
    refreshQBtn->callback(refreshQuestionsCallback, this);
    
    Fl_Button* uploadBtn = new Fl_Button(650, 145, 150, 25, "Upload Questions");
    uploadBtn->color(fl_rgb_color(255, 165, 0));
    uploadBtn->labelsize(11);
    uploadBtn->callback(uploadQuestionsCallback, this);
    uploadBtn->tooltip("Upload questions from a text file");
    
    Fl_Box* formatInfo = new Fl_Box(30, 180, 890, 30);
    formatInfo->copy_label("File Format: Q: Question? | A: Option A | B: Option B | C: Option C | D: Option D | ANSWER: A");
    formatInfo->labelsize(10);
    formatInfo->labelcolor(FL_DARK_BLUE);
    formatInfo->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    questionInput = new Fl_Multiline_Input(30, 225, 890, 70, "Question:");
    questionInput->align(FL_ALIGN_TOP_LEFT);
    
    optAInput = new Fl_Input(100, 310, 820, 25, "Option A:");
    optBInput = new Fl_Input(100, 345, 820, 25, "Option B:");
    optCInput = new Fl_Input(100, 380, 820, 25, "Option C:");
    optDInput = new Fl_Input(100, 415, 820, 25, "Option D:");
    
    Fl_Box* corrLabel = new Fl_Box(30, 455, 150, 25, "Correct Answer:");
    corrLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    correctChoice = new Fl_Choice(180, 455, 100, 25);
    correctChoice->add("A");
    correctChoice->add("B");
    correctChoice->add("C");
    correctChoice->add("D");
    correctChoice->value(0);
    
    Fl_Button* addQBtn = new Fl_Button(380, 495, 180, 40, "Add Question");
    addQBtn->color(FL_GREEN);
    addQBtn->labelsize(13);
    addQBtn->callback(addQuestionCallback, this);
    
    questionBrowser = new Fl_Browser(30, 550, 890, 110);
    
    questionTab->end();
    
    // RESULTS TAB
    Fl_Group* resultsTab = new Fl_Group(10, 85, 930, 595, "View Results");
    resultsTab->color(FL_WHITE);
    resultsTab->hide();
    
    Fl_Box* resTitle = new Fl_Box(350, 100, 250, 30, "Examination Results");
    resTitle->labelsize(16);
    resTitle->labelfont(FL_BOLD);
    
    Fl_Button* viewResBtn = new Fl_Button(400, 145, 150, 30, "Refresh Results");
    viewResBtn->color(fl_rgb_color(100, 149, 237));
    viewResBtn->callback(viewResultsCallback, this);
    
    resultsBrowser = new Fl_Browser(30, 190, 890, 470);
    
    resultsTab->end();
    
    // USER MANAGEMENT TAB
    Fl_Group* userTab = new Fl_Group(10, 85, 930, 595, "User Management");
    userTab->color(FL_WHITE);
    userTab->hide();
    
    Fl_Box* userTitle = new Fl_Box(350, 200, 250, 30, "User Management");
    userTitle->labelsize(16);
    userTitle->labelfont(FL_BOLD);
    
    Fl_Button* addUserBtn = new Fl_Button(350, 280, 250, 50, "Add New User");
    addUserBtn->color(FL_GREEN);
    addUserBtn->labelsize(14);
    addUserBtn->callback(addUserCallback, this);
    
    Fl_Box* infoBox = new Fl_Box(250, 360, 450, 80);
    infoBox->copy_label("Click Add New User to create accounts.\nPasswords are securely hashed using SHA-256.");
    infoBox->labelsize(12);
    infoBox->box(FL_BORDER_BOX);
    
    userTab->end();
    
    tabs->end();
    window->end();
    
    refreshCourseBrowser();
    refreshCourseChoice();
}

// ============================================================================
// Destructor Implementation
// ============================================================================

AdminDashboard::~AdminDashboard() {
    delete window;
}

// ============================================================================
// Show Function Implementation
// ============================================================================

void AdminDashboard::show() {
    window->show();
}