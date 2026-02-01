#include "ExamWindow.h"
#include "Globals.h"
#include "DatabaseManager.h"
#include "CourseSelectionWindow.h"
#include "ResultWindow.h"

#include <FL/fl_ask.H>
#include <cstdio>
#include <string>
#include <vector>

using namespace std;

void ExamWindow::timerCallback(void* data) {
    ExamWindow* exam = (ExamWindow*)data;
    exam->updateTimer();
}

void ExamWindow::prevCallback(Fl_Widget* w, void* data) {
    ExamWindow* exam = (ExamWindow*)data;
    exam->saveCurrentAnswer();
    if (exam->currentQuestionIndex > 0) {
        exam->currentQuestionIndex--;
        exam->displayQuestion();
    }
}

void ExamWindow::nextCallback(Fl_Widget* w, void* data) {
    ExamWindow* exam = (ExamWindow*)data;
    exam->saveCurrentAnswer();
    if (exam->currentQuestionIndex < (int)exam->examQuestions.size() - 1) {
        exam->currentQuestionIndex++;
        exam->displayQuestion();
    }
}

void ExamWindow::submitCallback(Fl_Widget* w, void* data) {
    ExamWindow* exam = (ExamWindow*)data;
    int choice = fl_choice(
        "Are you sure you want to submit?\n\nYou cannot change answers after submission.\n\nUnanswered questions will be marked incorrect.",
        "Cancel", "Submit Exam", NULL
    );
    if (choice == 1) {
        exam->submitExam();
    }
}

int ExamWindow::eventHandler(int event) {
    if (event == FL_PASTE || event == FL_SHORTCUT) {
        if (Fl::event_state(FL_CTRL)) {
            int key = Fl::event_key();
            if (key == 'c' || key == 'v' || key == 'x' ||
                key == 'C' || key == 'V' || key == 'X') {
                return 1;
            }
        }
    }
    return 0;
}

// ======================
// Member Functions
// ======================
void ExamWindow::updateTimer() {
    if (timeRemaining > 0) {
        timeRemaining--;
        timeSpent++;

        int minutes = timeRemaining / 60;
        int seconds = timeRemaining % 60;
        char timerText[50];
        sprintf(timerText, "Time: %02d:%02d", minutes, seconds);
        timerBox->copy_label(timerText);

        if (timeRemaining <= 60) {
            timerBox->labelcolor(FL_RED);
        } else if (timeRemaining <= 300) {
            timerBox->labelcolor(fl_rgb_color(255, 140, 0));
        }

        if (timeSpent % 30 == 0) {
            saveCurrentAnswer();
        }

        if (timeRemaining == 0) {
            fl_alert("Time is up! Exam will be auto-submitted.");
            submitExam();
            return;
        }

        Fl::repeat_timeout(1.0, timerCallback, this);
    }
}

void ExamWindow::displayQuestion() {
    if (currentQuestionIndex < 0 || currentQuestionIndex >= (int)examQuestions.size())
        return;

    Question& q = examQuestions[currentQuestionIndex];

    char qNum[100];
    sprintf(qNum, "Question %d of %d", currentQuestionIndex + 1, (int)examQuestions.size());
    questionNumberBox->copy_label(qNum);

    float progress = ((currentQuestionIndex + 1) * 100.0) / examQuestions.size();
    progressBar->value(progress);

    questionBox->value(q.questionText.c_str());

    optionA->copy_label(("A. " + q.optionA).c_str());
    optionB->copy_label(("B. " + q.optionB).c_str());
    optionC->copy_label(("C. " + q.optionC).c_str());
    optionD->copy_label(("D. " + q.optionD).c_str());

    string prev = candidateAnswers[currentQuestionIndex];
    optionA->value(prev == "A" ? 1 : 0);
    optionB->value(prev == "B" ? 1 : 0);
    optionC->value(prev == "C" ? 1 : 0);
    optionD->value(prev == "D" ? 1 : 0);

    prevBtn->deactivate();
    nextBtn->deactivate();

    if (currentQuestionIndex > 0) prevBtn->activate();
    if (currentQuestionIndex < (int)examQuestions.size() - 1) nextBtn->activate();
}

void ExamWindow::saveCurrentAnswer() {
    string answer = "";
    if (optionA->value()) answer = "A";
    else if (optionB->value()) answer = "B";
    else if (optionC->value()) answer = "C";
    else if (optionD->value()) answer = "D";

    candidateAnswers[currentQuestionIndex] = answer;
}

void ExamWindow::submitExam() {
    saveCurrentAnswer();

    int totalScore = 0;
    int totalPoints = 0;

    for (size_t i = 0; i < examQuestions.size(); i++) {
        Question& q = examQuestions[i];
        string userAns = candidateAnswers[i];
        bool correct = (userAns == q.correctAnswer);
        totalPoints += q.points;
        if (correct) totalScore += q.points;
    }

    Result result;
    result.userId = currentUser->id;
    result.username = currentUser->username;
    result.courseCode = selectedCourse->courseCode;
    result.courseTitle = selectedCourse->courseTitle;
    result.score = totalScore;
    result.totalQuestions = examQuestions.size();
    result.totalPoints = totalPoints;
    result.percentage = (totalPoints > 0) ? (totalScore * 100.0 / totalPoints) : 0;
    result.timeSpent = timeSpent;
    result.passed = (result.percentage >= selectedCourse->passingMark);

    dbManager->saveResult(result);

    window->hide();
    delete this;

    showResultWindow(result);
}

// ======================
// Constructor & Destructor
// ======================
ExamWindow::ExamWindow() {
    if (!selectedCourse) {
        fl_alert("Error: No course selected!");
        showCourseSelectionWindow();
        return;
    }

    window = new Fl_Window(950, 700, "Examination in Progress");
    window->color(fl_rgb_color(245, 245, 250));

    examQuestions = dbManager->getRandomQuestions(selectedCourse->id, selectedCourse->questionsPerExam);
    if (examQuestions.empty()) {
        fl_alert("Error: No questions available for this course!");
        window->hide();
        delete this;
        showCourseSelectionWindow();
        return;
    }

    candidateAnswers.resize(examQuestions.size(), "");
    currentQuestionIndex = 0;
    timeRemaining = selectedCourse->timeAllocation * 60;
    timeSpent = 0;

    // --- UI elements ---
    Fl_Box* header = new Fl_Box(300, 10, 350, 30, "Course-Based Examination");
    header->labelsize(16);
    header->labelfont(FL_BOLD);
    header->labelcolor(FL_BLUE);

    timerBox = new Fl_Box(750, 10, 150, 30);
    char timerText[50];
    sprintf(timerText, "Time: %02d:%02d", timeRemaining / 60, timeRemaining % 60);
    timerBox->copy_label(timerText);
    timerBox->labelsize(13);
    timerBox->labelfont(FL_BOLD);
    timerBox->labelcolor(FL_DARK_GREEN);
    timerBox->box(FL_BORDER_BOX);

    Fl_Box* candidateBox = new Fl_Box(30, 10, 250, 30);
    string candidateInfo = "Candidate: " + currentUser->username;
    candidateBox->copy_label(candidateInfo.c_str());
    candidateBox->labelsize(12);
    candidateBox->labelfont(FL_BOLD);

    courseInfoBox = new Fl_Box(30, 50, 890, 25);
    char courseInfo[300];
    sprintf(courseInfo, "Course: %s - %s", selectedCourse->courseCode.c_str(), selectedCourse->courseTitle.c_str());
    courseInfoBox->copy_label(courseInfo);
    courseInfoBox->labelsize(12);
    courseInfoBox->labelfont(FL_BOLD);
    courseInfoBox->labelcolor(FL_DARK_BLUE);

    progressBar = new Fl_Progress(30, 85, 890, 20);
    progressBar->minimum(0);
    progressBar->maximum(100);
    progressBar->color(FL_WHITE);
    progressBar->selection_color(FL_BLUE);

    questionNumberBox = new Fl_Box(380, 115, 200, 25, "");
    questionNumberBox->labelsize(13);
    questionNumberBox->labelfont(FL_BOLD);

    Fl_Box* qLabel = new Fl_Box(30, 150, 890, 20, "Question:");
    qLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    qLabel->labelfont(FL_BOLD);

    questionBox = new Fl_Multiline_Output(30, 175, 890, 100);
    questionBox->textsize(13);
    questionBox->box(FL_BORDER_BOX);

    radioGroup = new Fl_Group(30, 295, 890, 160);
    optionA = new Fl_Radio_Round_Button(50, 305, 870, 30, "A.");
    optionA->type(FL_RADIO_BUTTON); optionA->labelsize(12);
    optionB = new Fl_Radio_Round_Button(50, 345, 870, 30, "B.");
    optionB->type(FL_RADIO_BUTTON); optionB->labelsize(12);
    optionC = new Fl_Radio_Round_Button(50, 385, 870, 30, "C.");
    optionC->type(FL_RADIO_BUTTON); optionC->labelsize(12);
    optionD = new Fl_Radio_Round_Button(50, 425, 870, 30, "D.");
    optionD->type(FL_RADIO_BUTTON); optionD->labelsize(12);
    radioGroup->end();

    prevBtn = new Fl_Button(280, 525, 130, 45, "< Previous");
    prevBtn->color(fl_rgb_color(100, 149, 237)); prevBtn->labelsize(13); prevBtn->callback(prevCallback, this);
    nextBtn = new Fl_Button(430, 525, 130, 45, "Next >");
    nextBtn->color(fl_rgb_color(100, 149, 237)); nextBtn->labelsize(13); nextBtn->callback(nextCallback, this);
    submitBtn = new Fl_Button(580, 525, 150, 45, "Submit Exam");
    submitBtn->color(FL_GREEN); submitBtn->labelsize(13); submitBtn->labelfont(FL_BOLD);
    submitBtn->callback(submitCallback, this);

    Fl_Box* warning = new Fl_Box(30, 475, 890, 30);
    warning->copy_label("Security: Copy/Paste disabled | Auto-save every 30 seconds | Answer all questions");
    warning->labelsize(10); warning->labelcolor(FL_RED); warning->labelfont(FL_BOLD);

    window->end();

    Fl::add_handler(eventHandler);
    displayQuestion();
    Fl::add_timeout(1.0, timerCallback, this);
}

ExamWindow::~ExamWindow() {
    Fl::remove_timeout(timerCallback, this);
    delete window;
}

void ExamWindow::show() {
    window->show();
}