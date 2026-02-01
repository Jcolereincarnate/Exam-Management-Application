#ifndef COURSE_H
#define COURSE_H

#include <string>
using namespace std;

class Course {
public:
    int id;
    string courseCode;
    string courseTitle;
    int timeAllocation;
    int questionsPerExam;
    int passingMark;
    int totalQuestions;
    
    Course();
};

#endif