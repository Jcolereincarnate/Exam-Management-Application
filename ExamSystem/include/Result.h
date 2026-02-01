#ifndef RESULT_H
#define RESULT_H

#include <string>
using namespace std;

class Result {
public:
    int id;
    int userId;
    string username;
    int courseId;
    string courseCode;
    string courseTitle;
    string dateTime;
    int score;
    int totalQuestions;
    int totalPoints;
    double percentage;
    int timeSpent;
    bool passed;
    
    Result();
};

#endif