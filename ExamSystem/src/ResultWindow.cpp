#include "ResultWindow.h"
#include "Result.h"
#include "Globals.h"
#include "DatabaseManager.h"
#include "LoginWindow.h"
#include "CourseSelectionWindow.h"
#include <FL/fl_ask.H>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>


using namespace std;

void ResultWindow::viewAnalyticsCallback(Fl_Widget* w, void* data) {
    ResultWindow* resWin = (ResultWindow*)data;

    vector<Result> history = dbManager->getResults(currentUser->id);

    stringstream analytics;
    analytics << "=== PERFORMANCE ANALYTICS ===\n\n";
    analytics << "Student: " << currentUser->username << "\n\n";
    analytics << "Total Exams Taken: " << history.size() << "\n\n";

    if (!history.empty()) {
        double avgScore = 0;
        int passed = 0;

        for (size_t i = 0; i < history.size(); i++) {
            avgScore += history[i].percentage;
            if (history[i].passed) passed++;
        }

        avgScore /= history.size();

        analytics << "Average Score: " << fixed << setprecision(1) << avgScore << "%\n";
        analytics << "Exams Passed: " << passed << " / " << history.size() << "\n";
        analytics << "Pass Rate: " << (passed * 100 / (int)history.size()) << "%\n\n";

        analytics << "--- Recent Exam History ---\n\n";
        for (size_t i = 0; i < min(history.size(), (size_t)10); i++) {
            analytics << history[i].courseCode << " - " << history[i].courseTitle << "\n";
            analytics << "Date: " << history[i].dateTime << "\n";
            analytics << "Score: " << history[i].score << "/" << history[i].totalPoints
                      << " (" << history[i].percentage << "%) - "
                      << (history[i].passed ? "PASS" : "FAIL") << "\n\n";
        }
    }

    fl_message("%s", analytics.str().c_str());
}

void ResultWindow::exitCallback(Fl_Widget* w, void* data) {
    ResultWindow* resWin = (ResultWindow*)data;
    resWin->window->hide();
    delete resWin;
    currentUser = nullptr;
    if (selectedCourse != nullptr) {
        delete selectedCourse;
        selectedCourse = nullptr;
    }
    showLoginWindow();
}

void ResultWindow::newExamCallback(Fl_Widget* w, void* data) {
    ResultWindow* resWin = (ResultWindow*)data;
    resWin->window->hide();
    delete resWin;
    if (selectedCourse != nullptr) {
        delete selectedCourse;
        selectedCourse = nullptr;
    }
    showCourseSelectionWindow();
}

// ======================
// Constructor & Destructor
// ======================
ResultWindow::ResultWindow(Result r) : result(r) {
    window = new Fl_Window(700, 650, "Examination Result");
    window->color(FL_WHITE);

    Fl_Box* title = new Fl_Box(200, 20, 300, 40, "Examination Completed!");
    title->labelsize(18);
    title->labelfont(FL_BOLD);
    title->labelcolor(FL_BLUE);

    bool passed = result.passed;
    int yPos = 90;

    Fl_Box* box1 = new Fl_Box(100, yPos, 500, 25);
    box1->copy_label(("Candidate: " + result.username).c_str());
    box1->labelsize(13);
    box1->labelfont(FL_BOLD);
    box1->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    yPos += 35;
    Fl_Box* box2 = new Fl_Box(100, yPos, 500, 25);
    char temp[300];
    sprintf(temp, "Course: %s - %s", result.courseCode.c_str(), result.courseTitle.c_str());
    box2->copy_label(temp);
    box2->labelsize(13);
    box2->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    yPos += 35;
    Fl_Box* box3 = new Fl_Box(100, yPos, 500, 25);
    sprintf(temp, "Date: %s", result.dateTime.c_str());
    box3->copy_label(temp);
    box3->labelsize(12);
    box3->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    yPos += 35;
    Fl_Box* box4 = new Fl_Box(100, yPos, 500, 25);
    sprintf(temp, "Total Questions: %d", result.totalQuestions);
    box4->copy_label(temp);
    box4->labelsize(13);
    box4->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    yPos += 35;
    Fl_Box* box5 = new Fl_Box(100, yPos, 500, 25);
    sprintf(temp, "Points Earned: %d / %d", result.score, result.totalPoints);
    box5->copy_label(temp);
    box5->labelsize(13);
    box5->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    yPos += 35;
    Fl_Box* box6 = new Fl_Box(100, yPos, 500, 25);
    int mins = result.timeSpent / 60;
    int secs = result.timeSpent % 60;
    sprintf(temp, "Time Spent: %d min %d sec", mins, secs);
    box6->copy_label(temp);
    box6->labelsize(13);
    box6->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    yPos += 40;
    Fl_Box* box7 = new Fl_Box(100, yPos, 500, 30);
    sprintf(temp, "Score: %.1f%%", result.percentage);
    box7->copy_label(temp);
    box7->labelsize(16);
    box7->labelfont(FL_BOLD);
    box7->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    yPos += 40;
    Fl_Box* box8 = new Fl_Box(100, yPos, 500, 35);
    sprintf(temp, "Status: %s", passed ? "PASS" : "FAIL");
    box8->copy_label(temp);
    box8->labelsize(20);
    box8->labelfont(FL_BOLD);
    box8->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box8->labelcolor(passed ? FL_DARK_GREEN : FL_RED);

    Fl_Box* borderBox = new Fl_Box(90, 80, 520, 320);
    borderBox->box(FL_BORDER_BOX);

    Fl_Box* statusBox = new Fl_Box(100, 420, 500, 50);
    if (passed) {
        statusBox->copy_label("Congratulations! You have passed this examination.\nYour result has been recorded in the system.");
    } else {
        char msg[200];
        sprintf(msg, "You did not meet the passing mark of %d%%.\nKeep studying and try again!",
                selectedCourse->passingMark);
        statusBox->copy_label(msg);
    }
    statusBox->labelsize(12);
    statusBox->labelfont(FL_BOLD);
    statusBox->labelcolor(passed ? FL_DARK_GREEN : FL_RED);

    Fl_Button* analyticsBtn = new Fl_Button(100, 490, 160, 40, "View Analytics");
    analyticsBtn->color(fl_rgb_color(100, 149, 237));
    analyticsBtn->labelsize(12);
    analyticsBtn->callback(viewAnalyticsCallback, this);

    Fl_Button* newExamBtn = new Fl_Button(280, 490, 160, 40, "New Exam");
    newExamBtn->color(FL_GREEN);
    newExamBtn->labelsize(12);
    newExamBtn->callback(newExamCallback, this);

    Fl_Button* exitBtn = new Fl_Button(460, 490, 160, 40, "Logout");
    exitBtn->color(FL_RED);
    exitBtn->labelsize(12);
    exitBtn->callback(exitCallback, this);

    window->end();
}

ResultWindow::~ResultWindow() {
    delete window;
}

void ResultWindow::show() {
    window->show();
}