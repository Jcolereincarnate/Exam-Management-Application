#ifndef EXAM_WINDOW_H
#define EXAM_WINDOW_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Group.H>
#include <vector>
#include "Question.h"

class ExamWindow {
private:
    Fl_Window* window;
    vector<Question> examQuestions;
    vector<string> candidateAnswers;
    int currentQuestionIndex;
    int timeRemaining;
    int timeSpent;
    
    Fl_Box* timerBox;
    Fl_Box* questionNumberBox;
    Fl_Box* courseInfoBox;
    Fl_Multiline_Output* questionBox;
    Fl_Progress* progressBar;
    Fl_Radio_Round_Button* optionA;
    Fl_Radio_Round_Button* optionB;
    Fl_Radio_Round_Button* optionC;
    Fl_Radio_Round_Button* optionD;
    Fl_Button* prevBtn;
    Fl_Button* nextBtn;
    Fl_Button* submitBtn;
    Fl_Group* radioGroup;
    
    static void timerCallback(void* data);
    void updateTimer();
    void displayQuestion();
    void saveCurrentAnswer();
    
    static void prevCallback(Fl_Widget* w, void* data);
    static void nextCallback(Fl_Widget* w, void* data);
    static void submitCallback(Fl_Widget* w, void* data);
    void submitExam();
    
    static int eventHandler(int event);
    
public:
    ExamWindow();
    ~ExamWindow();
    void show();
};

#endif
