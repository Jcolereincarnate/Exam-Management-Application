#include "InstructionsWindow.h"
#include "Globals.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>
#include <cstdio>

void InstructionsWindow::startExamCallback(Fl_Widget* w, void* data) {
    InstructionsWindow* inst = (InstructionsWindow*)data;
    inst->window->hide();
    delete inst;
    showExamWindow();
}

void InstructionsWindow::backCallback(Fl_Widget* w, void* data) {
    InstructionsWindow* inst = (InstructionsWindow*)data;
    inst->window->hide();
    delete inst;
    if (selectedCourse != NULL) {
        delete selectedCourse;
        selectedCourse = NULL;
    }
    showCourseSelectionWindow();
}

InstructionsWindow::InstructionsWindow() {
    if (!selectedCourse) {
        fl_alert("Error: No course selected!");
        showCourseSelectionWindow();
        return;
    }
    
    window = new Fl_Window(700, 650, "Examination Instructions");
    window->color(FL_WHITE);
    
    Fl_Box* title = new Fl_Box(200, 20, 300, 40, "Examination Instructions");
    title->labelsize(18);
    title->labelfont(FL_BOLD);
    title->labelcolor(FL_BLUE);
    
    Fl_Text_Buffer* textBuffer = new Fl_Text_Buffer();
    Fl_Text_Display* textDisplay = new Fl_Text_Display(30, 80, 640, 470);
    textDisplay->buffer(textBuffer);
    textDisplay->textsize(12);
    textDisplay->box(FL_BORDER_BOX);
    
    char instructionText[1200];
    sprintf(instructionText,
        "EXAMINATION DETAILS\n"
        "==========================================\n\n"
        "Course Code: %s\n"
        "Course Title: %s\n"
        "Duration: %d minutes\n"
        "Total Questions: %d\n"
        "Passing Mark: %d%%\n\n"
        "IMPORTANT INSTRUCTIONS\n"
        "==========================================\n\n"
        "1. This exam consists of multiple-choice questions.\n\n"
        "2. Each question has four options (A, B, C, D).\n\n"
        "3. Select ONE answer for each question.\n\n"
        "4. Navigate using Next and Previous buttons.\n\n"
        "5. A countdown timer will display remaining time.\n\n"
        "6. Auto-save occurs every 30 seconds.\n\n"
        "7. Exam auto-submits when time expires.\n\n"
        "8. Copy and Paste are disabled for security.\n\n"
        "9. Confirm before final submission.\n\n"
        "10. Results displayed immediately after submission.\n\n"
        "11. You CANNOT change answers after submission.\n\n"
        "EXAMINATION RULES\n"
        "==========================================\n\n"
        "- All exam parameters are set by administrator\n"
        "- Questions randomly selected from course pool\n"
        "- No external materials or assistance allowed\n"
        "- Ensure stable internet connection\n\n"
        "Click START EXAMINATION when ready.\n\n"
        "Good Luck!",
        selectedCourse->courseCode.c_str(),
        selectedCourse->courseTitle.c_str(),
        selectedCourse->timeAllocation,
        selectedCourse->questionsPerExam,
        selectedCourse->passingMark
    );
    
    textBuffer->text(instructionText);
    
    Fl_Button* startBtn = new Fl_Button(250, 570, 200, 45, "START EXAMINATION");
    startBtn->color(FL_GREEN);
    startBtn->labelsize(14);
    startBtn->labelfont(FL_BOLD);
    startBtn->callback(startExamCallback, this);
    
    Fl_Button* backBtn = new Fl_Button(500, 570, 150, 45, "Back");
    backBtn->color(fl_rgb_color(100, 149, 237));
    backBtn->callback(backCallback, this);
    
    window->end();
}

InstructionsWindow::~InstructionsWindow() {
    delete window;
}

void InstructionsWindow::show() {
    window->show();
}