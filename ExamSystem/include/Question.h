#ifndef QUESTION_H
#define QUESTION_H

#include <string>
using namespace std;

class Question {
public:
    int id;
    int courseId;
    string questionText;
    string optionA;
    string optionB;
    string optionC;
    string optionD;
    string correctAnswer;
    int points;
    
    Question();
};

#endif