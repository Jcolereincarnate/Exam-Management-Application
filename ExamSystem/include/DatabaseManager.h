#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "User.h"
#include "Course.h"
#include "Question.h"
#include "Result.h"

using namespace std;

class DatabaseManager {
private:
    sqlite3* db;
    string dbPath;
    
public:
    DatabaseManager(string path = "database/exam_system.db");
    ~DatabaseManager();
    
    bool initDatabase();
    void createTables();
    void insertDefaultData();
    
    // User management
    bool addUser(string username, string password, string role);
    User* authenticateUser(string username, string password);
    void incrementLoginAttempts(string username);
    void resetLoginAttempts(string username);
    
    // Course management
    bool addCourse(string code, string title, int time, int questionsCount, int passing);
    vector<Course> getAllCourses();
    Course* getCourseById(int courseId);
    int getCourseIdByCode(string code);
    
    // Question management
    bool addQuestion(int courseId, string qText, string optA, string optB,
                    string optC, string optD, string correct, int pts);
    vector<Question> getRandomQuestions(int courseId, int count);
    
    // Result management
    int saveResult(Result r);
    vector<Result> getResults(int userId = -1);
};

#endif