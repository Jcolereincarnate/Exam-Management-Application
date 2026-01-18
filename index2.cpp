#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_File_Chooser.H>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>
#include <cstring>
#include <sqlite3.h>
#include <openssl/sha.h>
using namespace std;


string sha256(const string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.length());
    SHA256_Final(hash, &sha256);
    
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// ============================================================================
// DATA STRUCTURES
// ============================================================================

class User {
public:
    int id;
    string username;
    string passwordHash;
    string role;
    int loginAttempts;
    
    User(int i = 0, string u = "", string ph = "", string r = "candidate")
        : id(i), username(u), passwordHash(ph), role(r), loginAttempts(0) {}
};

class Course {
public:
    int id;
    string courseCode;
    string courseTitle;
    int timeAllocation;
    int questionsPerExam;
    int passingMark;
    int totalQuestions;
    
    Course() : id(0), timeAllocation(60), questionsPerExam(40), 
               passingMark(40), totalQuestions(0) {}
};

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
    
    Question() : id(0), courseId(0), points(1) {}
};

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
    
    Result() : id(0), userId(0), courseId(0), score(0), totalQuestions(0), 
               totalPoints(0), percentage(0), timeSpent(0), passed(false) {}
};

// ============================================================================
// DATABASE MANAGER
// ============================================================================

class DatabaseManager {
private:
    sqlite3* db;
    string dbPath;
    
public:
    DatabaseManager(string path = "exam_system.db") : db(NULL), dbPath(path) {
        initDatabase();
    }
    
    ~DatabaseManager() {
        if (db) {
            sqlite3_close(db);
        }
    }
    
    bool initDatabase() {
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc) {
            char errMsg[300];
            sprintf(errMsg, "Cannot open database: %s\nPath: %s", 
                   sqlite3_errmsg(db), dbPath.c_str());
            fl_alert("%s", errMsg);
            return false;
        }
        
        char successMsg[300];
        sprintf(successMsg, "Database initialized successfully!\nPath: %s", dbPath.c_str());
        fl_message("%s", successMsg);
        
        createTables();
        insertDefaultData();
        return true;
    }
    
    void createTables() {
        const char* sqlUsers = 
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT UNIQUE NOT NULL,"
            "password_hash TEXT NOT NULL,"
            "role TEXT NOT NULL,"
            "login_attempts INTEGER DEFAULT 0);";
        
        const char* sqlCourses = 
            "CREATE TABLE IF NOT EXISTS courses ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "course_code TEXT UNIQUE NOT NULL,"
            "course_title TEXT NOT NULL,"
            "time_allocation INTEGER DEFAULT 60,"
            "questions_per_exam INTEGER DEFAULT 40,"
            "passing_mark INTEGER DEFAULT 40);";
        
        const char* sqlQuestions = 
            "CREATE TABLE IF NOT EXISTS questions ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "course_id INTEGER NOT NULL,"
            "question_text TEXT NOT NULL,"
            "option_a TEXT NOT NULL,"
            "option_b TEXT NOT NULL,"
            "option_c TEXT NOT NULL,"
            "option_d TEXT NOT NULL,"
            "correct_answer TEXT NOT NULL,"
            "points INTEGER DEFAULT 1,"
            "FOREIGN KEY(course_id) REFERENCES courses(id));";
        
        const char* sqlResults = 
            "CREATE TABLE IF NOT EXISTS results ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user_id INTEGER,"
            "username TEXT,"
            "course_id INTEGER,"
            "course_code TEXT,"
            "course_title TEXT,"
            "date_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
            "score INTEGER,"
            "total_questions INTEGER,"
            "total_points INTEGER,"
            "percentage REAL,"
            "time_spent INTEGER,"
            "passed INTEGER,"
            "FOREIGN KEY(user_id) REFERENCES users(id),"
            "FOREIGN KEY(course_id) REFERENCES courses(id));";
        
        char* errMsg = 0;
        int rc;
        
        rc = sqlite3_exec(db, sqlUsers, NULL, 0, &errMsg);
        if (rc != SQLITE_OK) {
            fl_alert("Error creating users table: %s", errMsg);
            sqlite3_free(errMsg);
        }
        
        rc = sqlite3_exec(db, sqlCourses, NULL, 0, &errMsg);
        if (rc != SQLITE_OK) {
            fl_alert("Error creating courses table: %s", errMsg);
            sqlite3_free(errMsg);
        }
        
        rc = sqlite3_exec(db, sqlQuestions, NULL, 0, &errMsg);
        if (rc != SQLITE_OK) {
            fl_alert("Error creating questions table: %s", errMsg);
            sqlite3_free(errMsg);
        }
        
        rc = sqlite3_exec(db, sqlResults, NULL, 0, &errMsg);
        if (rc != SQLITE_OK) {
            fl_alert("Error creating results table: %s", errMsg);
            sqlite3_free(errMsg);
        }
    }
    
    void insertDefaultData() {
        sqlite3_stmt* stmt;
        const char* checkAdmin = "SELECT COUNT(*) FROM users WHERE role='admin'";
        
        if (sqlite3_prepare_v2(db, checkAdmin, -1, &stmt, 0) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int count = sqlite3_column_int(stmt, 0);
                if (count == 0) {
                    addUser("admin", "admin123", "admin");
                    addUser("student1", "pass123", "candidate");
                }
            }
            sqlite3_finalize(stmt);
        }
    }
    
   
    bool addUser(string username, string password, string role) {
        string passwordHash = sha256(password);
        const char* sql = "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);
            
            int result = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return result == SQLITE_DONE;
        }
        return false;
    }
    
    User* authenticateUser(string username, string password) {
        string passwordHash = sha256(password);
        const char* sql = "SELECT id, username, password_hash, role, login_attempts "
                         "FROM users WHERE username = ?";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
            
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                User* user = new User();
                user->id = sqlite3_column_int(stmt, 0);
                user->username = string((char*)sqlite3_column_text(stmt, 1));
                string storedHash = string((char*)sqlite3_column_text(stmt, 2));
                user->role = string((char*)sqlite3_column_text(stmt, 3));
                user->loginAttempts = sqlite3_column_int(stmt, 4);
                
                sqlite3_finalize(stmt);
                
                if (user->loginAttempts >= 5) {
                    delete user;
                    return NULL;
                }
                
                if (storedHash == passwordHash) {
                    resetLoginAttempts(username);
                    return user;
                } else {
                    incrementLoginAttempts(username);
                    delete user;
                    return NULL;
                }
            }
            sqlite3_finalize(stmt);
        }
        return NULL;
    }
    
    void incrementLoginAttempts(string username) {
        const char* sql = "UPDATE users SET login_attempts = login_attempts + 1 WHERE username = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    
    void resetLoginAttempts(string username) {
        const char* sql = "UPDATE users SET login_attempts = 0 WHERE username = ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    
    bool addCourse(string code, string title, int time, int questionsCount, int passing) {
        const char* sql = "INSERT INTO courses (course_code, course_title, time_allocation, "
                         "questions_per_exam, passing_mark) VALUES (?, ?, ?, ?, ?)";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, time);
            sqlite3_bind_int(stmt, 4, questionsCount);
            sqlite3_bind_int(stmt, 5, passing);
            
            int result = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return result == SQLITE_DONE;
        }
        return false;
    }
    
    vector<Course> getAllCourses() {
        vector<Course> courses;
        const char* sql = "SELECT c.id, c.course_code, c.course_title, c.time_allocation, "
                         "c.questions_per_exam, c.passing_mark, COUNT(q.id) as total_questions "
                         "FROM courses c LEFT JOIN questions q ON c.id = q.course_id "
                         "GROUP BY c.id ORDER BY c.course_code";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                Course c;
                c.id = sqlite3_column_int(stmt, 0);
                c.courseCode = string((char*)sqlite3_column_text(stmt, 1));
                c.courseTitle = string((char*)sqlite3_column_text(stmt, 2));
                c.timeAllocation = sqlite3_column_int(stmt, 3);
                c.questionsPerExam = sqlite3_column_int(stmt, 4);
                c.passingMark = sqlite3_column_int(stmt, 5);
                c.totalQuestions = sqlite3_column_int(stmt, 6);
                courses.push_back(c);
            }
            sqlite3_finalize(stmt);
        }
        return courses;
    }
    
    Course* getCourseById(int courseId) {
        const char* sql = "SELECT c.id, c.course_code, c.course_title, c.time_allocation, "
                         "c.questions_per_exam, c.passing_mark, COUNT(q.id) as total_questions "
                         "FROM courses c LEFT JOIN questions q ON c.id = q.course_id "
                         "WHERE c.id = ? GROUP BY c.id";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, courseId);
            
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                Course* c = new Course();
                c->id = sqlite3_column_int(stmt, 0);
                c->courseCode = string((char*)sqlite3_column_text(stmt, 1));
                c->courseTitle = string((char*)sqlite3_column_text(stmt, 2));
                c->timeAllocation = sqlite3_column_int(stmt, 3);
                c->questionsPerExam = sqlite3_column_int(stmt, 4);
                c->passingMark = sqlite3_column_int(stmt, 5);
                c->totalQuestions = sqlite3_column_int(stmt, 6);
                sqlite3_finalize(stmt);
                return c;
            }
            sqlite3_finalize(stmt);
        }
        return NULL;
    }
    
    int getCourseIdByCode(string code) {
        const char* sql = "SELECT id FROM courses WHERE course_code = ?";
        sqlite3_stmt* stmt;
        int id = 0;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                id = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        return id;
    }
    
    bool addQuestion(int courseId, string qText, string optA, string optB, 
                    string optC, string optD, string correct, int pts) {
        const char* sql = "INSERT INTO questions (course_id, question_text, option_a, "
                         "option_b, option_c, option_d, correct_answer, points) "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, courseId);
            sqlite3_bind_text(stmt, 2, qText.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, optA.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, optB.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, optC.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, optD.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, correct.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 8, pts);
            
            int result = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return result == SQLITE_DONE;
        }
        return false;
    }
    
    vector<Question> getRandomQuestions(int courseId, int count) {
        vector<Question> questions;
        const char* sql = "SELECT * FROM questions WHERE course_id = ? ORDER BY RANDOM() LIMIT ?";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, courseId);
            sqlite3_bind_int(stmt, 2, count);
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                Question q;
                q.id = sqlite3_column_int(stmt, 0);
                q.courseId = sqlite3_column_int(stmt, 1);
                q.questionText = string((char*)sqlite3_column_text(stmt, 2));
                q.optionA = string((char*)sqlite3_column_text(stmt, 3));
                q.optionB = string((char*)sqlite3_column_text(stmt, 4));
                q.optionC = string((char*)sqlite3_column_text(stmt, 5));
                q.optionD = string((char*)sqlite3_column_text(stmt, 6));
                q.correctAnswer = string((char*)sqlite3_column_text(stmt, 7));
                q.points = sqlite3_column_int(stmt, 8);
                questions.push_back(q);
            }
            sqlite3_finalize(stmt);
        }
        return questions;
    }
    
    int saveResult(Result r) {
        const char* sql = "INSERT INTO results (user_id, username, course_id, course_code, "
                         "course_title, score, total_questions, total_points, percentage, "
                         "time_spent, passed) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, r.userId);
            sqlite3_bind_text(stmt, 2, r.username.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, r.courseId);
            sqlite3_bind_text(stmt, 4, r.courseCode.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, r.courseTitle.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 6, r.score);
            sqlite3_bind_int(stmt, 7, r.totalQuestions);
            sqlite3_bind_int(stmt, 8, r.totalPoints);
            sqlite3_bind_double(stmt, 9, r.percentage);
            sqlite3_bind_int(stmt, 10, r.timeSpent);
            sqlite3_bind_int(stmt, 11, r.passed ? 1 : 0);
            
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                int resultId = sqlite3_last_insert_rowid(db);
                sqlite3_finalize(stmt);
                return resultId;
            }
            sqlite3_finalize(stmt);
        }
        return -1;
    }
    
    vector<Result> getResults(int userId = -1) {
        vector<Result> results;
        stringstream sql;
        sql << "SELECT * FROM results";
        if (userId > 0) {
            sql << " WHERE user_id = " << userId;
        }
        sql << " ORDER BY date_time DESC";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                Result r;
                r.id = sqlite3_column_int(stmt, 0);
                r.userId = sqlite3_column_int(stmt, 1);
                r.username = string((char*)sqlite3_column_text(stmt, 2));
                r.courseId = sqlite3_column_int(stmt, 3);
                r.courseCode = string((char*)sqlite3_column_text(stmt, 4));
                r.courseTitle = string((char*)sqlite3_column_text(stmt, 5));
                r.dateTime = string((char*)sqlite3_column_text(stmt, 6));
                r.score = sqlite3_column_int(stmt, 7);
                r.totalQuestions = sqlite3_column_int(stmt, 8);
                r.totalPoints = sqlite3_column_int(stmt, 9);
                r.percentage = sqlite3_column_double(stmt, 10);
                r.timeSpent = sqlite3_column_int(stmt, 11);
                r.passed = sqlite3_column_int(stmt, 12) == 1;
                results.push_back(r);
            }
            sqlite3_finalize(stmt);
        }
        return results;
    }
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

DatabaseManager* dbManager = NULL;
User* currentUser = NULL;
Course* selectedCourse = NULL;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

void showLoginWindow();
void showAdminDashboard();
void showCourseSelectionWindow();
void showInstructionsWindow();
void showExamWindow();
void showResultWindow(Result result);

// ============================================================================
// ADMIN DASHBOARD
// ============================================================================

class AdminDashboard {
private:
    Fl_Window* window;
    Fl_Tabs* tabs;
    Fl_Input* courseCodeInput;
    Fl_Input* courseTitleInput;
    Fl_Value_Input* timeInput;
    Fl_Value_Input* questionsCountInput;
    Fl_Value_Input* passingMarkInput;
    Fl_Browser* courseBrowser;
    Fl_Choice* courseChoice;
    Fl_Multiline_Input* questionInput;
    Fl_Input* optAInput;
    Fl_Input* optBInput;
    Fl_Input* optCInput;
    Fl_Input* optDInput;
    Fl_Choice* correctChoice;
    Fl_Browser* questionBrowser;
    Fl_Browser* resultsBrowser;
    
    static void addCourseCallback(Fl_Widget* w, void* data) {
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
    
    static void refreshCoursesCallback(Fl_Widget* w, void* data) {
        AdminDashboard* panel = (AdminDashboard*)data;
        panel->refreshCourseBrowser();
        panel->refreshCourseChoice();
    }
    

    static void uploadQuestionsCallback(Fl_Widget* w, void* data) {
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
    
    int uploadQuestionsFromFile(const char* filename, int courseId) {
        ifstream file(filename);
        if (!file.is_open()) {
            return 0;
        }
        
        int count = 0;
        string line;
        
        while (getline(file, line)) {
            // Skip empty lines
            if (line.empty() || line[0] == '#') continue;
            
            // Question format:
            // Q: Question text?
            // A: Option A
            // B: Option B
            // C: Option C
            // D: Option D
            // ANSWER: A
            
            if (line.substr(0, 2) == "Q:") {
                string question = line.substr(2);
                // Trim leading space
                if (!question.empty() && question[0] == ' ') {
                    question = question.substr(1);
                }
                
                string optA, optB, optC, optD, correct;
                
                // Read options
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
                    // Take only first character
                    if (!correct.empty()) correct = correct.substr(0, 1);
                }
                
                // Validate and add question
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
    
    static void addQuestionCallback(Fl_Widget* w, void* data) {
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
    
    static void refreshQuestionsCallback(Fl_Widget* w, void* data) {
        AdminDashboard* panel = (AdminDashboard*)data;
        panel->refreshQuestionBrowser();
    }
    
    static void viewResultsCallback(Fl_Widget* w, void* data) {
        AdminDashboard* panel = (AdminDashboard*)data;
        panel->refreshResults();
    }
    
    static void addUserCallback(Fl_Widget* w, void* data) {
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
    
    static void logoutCallback(Fl_Widget* w, void* data) {
        AdminDashboard* panel = (AdminDashboard*)data;
        panel->window->hide();
        delete panel;
        currentUser = NULL;
        showLoginWindow();
    }
    
    void clearCourseFields() {
        courseCodeInput->value("");
        courseTitleInput->value("");
        timeInput->value(60);
        questionsCountInput->value(40);
        passingMarkInput->value(40);
    }
    
    void clearQuestionFields() {
        questionInput->value("");
        optAInput->value("");
        optBInput->value("");
        optCInput->value("");
        optDInput->value("");
        correctChoice->value(0);
    }
    
    void refreshCourseBrowser() {
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
    
    void refreshCourseChoice() {
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
    
    void refreshQuestionBrowser() {
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
    
    void refreshResults() {
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
    
public:
    AdminDashboard() {
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
    
    void show() {
        window->show();
    }
    
    ~AdminDashboard() {
        delete window;
    }
};

// ============================================================================
// LOGIN WINDOW
// ============================================================================

class LoginWindow {
private:
    Fl_Window* window;
    Fl_Input* userIdInput;
    Fl_Secret_Input* passwordInput;
    
    static void loginCallback(Fl_Widget* w, void* data) {
        LoginWindow* login = (LoginWindow*)data;
        
        string userId = login->userIdInput->value();
        string password = login->passwordInput->value();
        
        if (userId.empty() || password.empty()) {
            fl_alert("Please enter both Username and Password!");
            return;
        }
        
        currentUser = dbManager->authenticateUser(userId, password);
        
        if (currentUser != NULL) {
            login->window->hide();
            delete login;
            
            if (currentUser->role == "admin") {
                showAdminDashboard();
            } else {
                showCourseSelectionWindow();
            }
        } else {
            fl_alert("Invalid credentials or account locked!\n\nDefault accounts:\nAdmin: admin/admin123\nStudent: student1/pass123");
        }
    }
    
    static void exitCallback(Fl_Widget* w, void* data) {
        exit(0);
    }
    
public:
    LoginWindow() {
        window = new Fl_Window(500, 450, "Course-Based Examination System");
        window->color(fl_rgb_color(240, 248, 255));
        
        Fl_Box* title = new Fl_Box(80, 30, 340, 50, "Course-Based Exam System");
        title->labelsize(18);
        title->labelfont(FL_BOLD);
        title->labelcolor(FL_BLUE);
        
        Fl_Box* subtitle = new Fl_Box(150, 75, 200, 30, "Secure Login Portal");
        subtitle->labelsize(14);
        subtitle->labelcolor(FL_DARK_BLUE);
        
        Fl_Box* features = new Fl_Box(50, 115, 400, 60);
        features->copy_label("Admin-Controlled Examination System\nCourse-Based | Secure | Professional");
        features->labelsize(11);
        features->labelcolor(FL_DARK_GREEN);
        features->box(FL_BORDER_BOX);
        
        Fl_Box* userLabel = new Fl_Box(80, 200, 100, 30, "Username:");
        userLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
        userIdInput = new Fl_Input(200, 200, 220, 30);
        
        Fl_Box* passLabel = new Fl_Box(80, 250, 100, 30, "Password:");
        passLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
        passwordInput = new Fl_Secret_Input(200, 250, 220, 30);
        
        Fl_Button* loginBtn = new Fl_Button(150, 310, 100, 40, "Login");
        loginBtn->color(FL_GREEN);
        loginBtn->labelsize(14);
        loginBtn->callback(loginCallback, this);
        
        Fl_Button* exitBtn = new Fl_Button(270, 310, 100, 40, "Exit");
        exitBtn->color(FL_RED);
        exitBtn->labelsize(14);
        exitBtn->callback(exitCallback, NULL);
        
        Fl_Box* info = new Fl_Box(50, 370, 400, 60);
        info->copy_label("Default Accounts:\nAdmin: admin/admin123\nStudent: student1/pass123");
        info->labelsize(11);
        info->labelcolor(fl_rgb_color(0, 100, 0));
        
        window->end();
    }
    
    void show() {
        window->show();
    }
    
    ~LoginWindow() {
        delete window;
    }
};

// ============================================================================
// COURSE SELECTION WINDOW
// ============================================================================

class CourseSelectionWindow {
private:
    Fl_Window* window;
    Fl_Hold_Browser* courseBrowser;
    vector<Course> availableCourses;
    
    static void continueCallback(Fl_Widget* w, void* data) {
        CourseSelectionWindow* csw = (CourseSelectionWindow*)data;
        
        int selected = csw->courseBrowser->value();
        if (selected <= 0) {
            fl_alert("Please select a course before continuing!");
            return;
        }
        
        int courseIdx = selected - 1;
        
        if (courseIdx >= 0 && courseIdx < (int)csw->availableCourses.size()) {
            if (selectedCourse != NULL) {
                delete selectedCourse;
                selectedCourse = NULL;
            }
            
            selectedCourse = new Course();
            *selectedCourse = csw->availableCourses[courseIdx];
            
            if (selectedCourse->totalQuestions < selectedCourse->questionsPerExam) {
                fl_alert("Error: Course %s has insufficient questions!\nRequired: %d, Available: %d",
                        selectedCourse->courseCode.c_str(),
                        selectedCourse->questionsPerExam,
                        selectedCourse->totalQuestions);
                delete selectedCourse;
                selectedCourse = NULL;
                return;
            }
            
            csw->window->hide();
            delete csw;
            showInstructionsWindow();
        }
    }
    
    static void logoutCallback(Fl_Widget* w, void* data) {
        CourseSelectionWindow* csw = (CourseSelectionWindow*)data;
        csw->window->hide();
        delete csw;
        currentUser = NULL;
        if (selectedCourse != NULL) {
            delete selectedCourse;
            selectedCourse = NULL;
        }
        showLoginWindow();
    }
    
public:
    CourseSelectionWindow() {
        window = new Fl_Window(700, 600, "Course Selection");
        window->color(FL_WHITE);
        
        Fl_Box* title = new Fl_Box(200, 20, 300, 40, "Select Course for Examination");
        title->labelsize(18);
        title->labelfont(FL_BOLD);
        title->labelcolor(FL_BLUE);
        
        Fl_Box* welcomeBox = new Fl_Box(50, 70, 600, 30);
        string welcome = "Welcome, " + currentUser->username + "!";
        welcomeBox->copy_label(welcome.c_str());
        welcomeBox->labelsize(14);
        welcomeBox->labelfont(FL_BOLD);
        
        Fl_Box* infoBox = new Fl_Box(50, 110, 600, 40);
        infoBox->copy_label("Select a course below to begin your examination.\nExam settings are controlled by the administrator.");
        infoBox->labelsize(12);
        
        courseBrowser = new Fl_Hold_Browser(50, 170, 600, 330);
        courseBrowser->textsize(12);
        
        // Load available courses directly from database
        availableCourses = dbManager->getAllCourses();
        
        if (availableCourses.empty()) {
            courseBrowser->add("@C1@bNo courses available");
            courseBrowser->add("");
            courseBrowser->add("Please contact your administrator to:");
            courseBrowser->add("1. Add courses in the Admin Dashboard");
            courseBrowser->add("2. Ensure the database is working correctly");
        } else {
            for (size_t i = 0; i < availableCourses.size(); i++) {
                char buffer[400];
                
                // Add course header with formatting
                sprintf(buffer, "@B15@C4@b%s - %s", 
                       availableCourses[i].courseCode.c_str(),
                       availableCourses[i].courseTitle.c_str());
                courseBrowser->add(buffer);
                
                // Add course details
                sprintf(buffer, "@.   Duration: %d minutes", 
                       availableCourses[i].timeAllocation);
                courseBrowser->add(buffer);
                
                sprintf(buffer, "@.   Questions: %d | Passing Mark: %d%%", 
                       availableCourses[i].questionsPerExam,
                       availableCourses[i].passingMark);
                courseBrowser->add(buffer);
                
                sprintf(buffer, "@.   Question Pool: %d questions available", 
                       availableCourses[i].totalQuestions);
                courseBrowser->add(buffer);
                
                // Status indicator
                if (availableCourses[i].totalQuestions >= availableCourses[i].questionsPerExam) {
                    courseBrowser->add("@C2@.   Status: Ready to take exam");
                } else {
                    sprintf(buffer, "@C1@.   Status: Not ready (needs %d more questions)", 
                           availableCourses[i].questionsPerExam - availableCourses[i].totalQuestions);
                    courseBrowser->add(buffer);
                }
                
                courseBrowser->add("   ========================================");
            }
        }
        
        Fl_Button* continueBtn = new Fl_Button(250, 520, 200, 40, "Continue to Instructions");
        continueBtn->color(FL_GREEN);
        continueBtn->labelsize(13);
        continueBtn->labelfont(FL_BOLD);
        continueBtn->callback(continueCallback, this);
        
        Fl_Button* logoutBtn = new Fl_Button(550, 520, 100, 40, "Logout");
        logoutBtn->color(FL_RED);
        logoutBtn->callback(logoutCallback, this);
        
        window->end();
    }
    
    void show() {
        window->show();
    }
    
    ~CourseSelectionWindow() {
        delete window;
    }
};

// ============================================================================
// INSTRUCTIONS WINDOW
// ============================================================================

class InstructionsWindow {
private:
    Fl_Window* window;
    
    static void startExamCallback(Fl_Widget* w, void* data) {
        InstructionsWindow* inst = (InstructionsWindow*)data;
        inst->window->hide();
        delete inst;
        showExamWindow();
    }
    
    static void backCallback(Fl_Widget* w, void* data) {
        InstructionsWindow* inst = (InstructionsWindow*)data;
        inst->window->hide();
        delete inst;
        if (selectedCourse != NULL) {
            delete selectedCourse;
            selectedCourse = NULL;
        }
        showCourseSelectionWindow();
    }
    
public:
    InstructionsWindow() {
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
    
    void show() {
        window->show();
    }
    
    ~InstructionsWindow() {
        delete window;
    }
};

// ============================================================================
// EXAMINATION WINDOW
// ============================================================================

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
    
    static void timerCallback(void* data) {
        ExamWindow* exam = (ExamWindow*)data;
        exam->updateTimer();
    }
    
    void updateTimer() {
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
    
    void displayQuestion() {
        if (currentQuestionIndex < 0 || currentQuestionIndex >= (int)examQuestions.size()) {
            return;
        }
        
        Question& q = examQuestions[currentQuestionIndex];
        
        char qNum[100];
        sprintf(qNum, "Question %d of %d", currentQuestionIndex + 1, 
                (int)examQuestions.size());
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
        
        if (currentQuestionIndex > 0) {
            prevBtn->activate();
        }
        
        if (currentQuestionIndex < (int)examQuestions.size() - 1) {
            nextBtn->activate();
        }
    }
    
    void saveCurrentAnswer() {
        string answer = "";
        if (optionA->value()) answer = "A";
        else if (optionB->value()) answer = "B";
        else if (optionC->value()) answer = "C";
        else if (optionD->value()) answer = "D";
        
        candidateAnswers[currentQuestionIndex] = answer;
    }
    
    static void prevCallback(Fl_Widget* w, void* data) {
        ExamWindow* exam = (ExamWindow*)data;
        exam->saveCurrentAnswer();
        if (exam->currentQuestionIndex > 0) {
            exam->currentQuestionIndex--;
            exam->displayQuestion();
        }
    }
    
    static void nextCallback(Fl_Widget* w, void* data) {
        ExamWindow* exam = (ExamWindow*)data;
        exam->saveCurrentAnswer();
        if (exam->currentQuestionIndex < (int)exam->examQuestions.size() - 1) {
            exam->currentQuestionIndex++;
            exam->displayQuestion();
        }
    }
    
    static void submitCallback(Fl_Widget* w, void* data) {
        ExamWindow* exam = (ExamWindow*)data;
        
        int choice = fl_choice("Are you sure you want to submit?\n\nYou cannot change answers after submission.\n\nUnanswered questions will be marked incorrect.",
                              "Cancel", "Submit Exam", NULL);
        
        if (choice == 1) {
            exam->submitExam();
        }
    }
    
    void submitExam() {
        saveCurrentAnswer();
        
        int totalScore = 0;
        int totalPoints = 0;
        
        for (size_t i = 0; i < examQuestions.size(); i++) {
            Question& q = examQuestions[i];
            string userAns = candidateAnswers[i];
            bool correct = (userAns == q.correctAnswer);
            
            totalPoints += q.points;
            if (correct) {
                totalScore += q.points;
            }
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
    
    static int eventHandler(int event) {
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
    
public:
    ExamWindow() {
        if (!selectedCourse) {
            fl_alert("Error: No course selected!");
            showCourseSelectionWindow();
            return;
        }
        
        window = new Fl_Window(950, 700, "Examination in Progress");
        window->color(fl_rgb_color(245, 245, 250));
        
        examQuestions = dbManager->getRandomQuestions(selectedCourse->id, 
                                                      selectedCourse->questionsPerExam);
        
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
        
        Fl_Box* header = new Fl_Box(300, 10, 350, 30, "Course-Based Examination");
        header->labelsize(16);
        header->labelfont(FL_BOLD);
        header->labelcolor(FL_BLUE);
        
        timerBox = new Fl_Box(750, 10, 150, 30);
        char timerText[50];
        sprintf(timerText, "Time: %02d:%02d", timeRemaining/60, timeRemaining%60);
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
        sprintf(courseInfo, "Course: %s - %s", 
               selectedCourse->courseCode.c_str(),
               selectedCourse->courseTitle.c_str());
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
        optionA->type(FL_RADIO_BUTTON);
        optionA->labelsize(12);
        
        optionB = new Fl_Radio_Round_Button(50, 345, 870, 30, "B.");
        optionB->type(FL_RADIO_BUTTON);
        optionB->labelsize(12);
        
        optionC = new Fl_Radio_Round_Button(50, 385, 870, 30, "C.");
        optionC->type(FL_RADIO_BUTTON);
        optionC->labelsize(12);
        
        optionD = new Fl_Radio_Round_Button(50, 425, 870, 30, "D.");
        optionD->type(FL_RADIO_BUTTON);
        optionD->labelsize(12);
        
        radioGroup->end();
        
        Fl_Box* warning = new Fl_Box(30, 475, 890, 30);
        warning->copy_label("Security: Copy/Paste disabled | Auto-save every 30 seconds | Answer all questions");
        warning->labelsize(10);
        warning->labelcolor(FL_RED);
        warning->labelfont(FL_BOLD);
        
        prevBtn = new Fl_Button(280, 525, 130, 45, "< Previous");
        prevBtn->color(fl_rgb_color(100, 149, 237));
        prevBtn->labelsize(13);
        prevBtn->callback(prevCallback, this);
        
        nextBtn = new Fl_Button(430, 525, 130, 45, "Next >");
        nextBtn->color(fl_rgb_color(100, 149, 237));
        nextBtn->labelsize(13);
        nextBtn->callback(nextCallback, this);
        
        submitBtn = new Fl_Button(580, 525, 150, 45, "Submit Exam");
        submitBtn->color(FL_GREEN);
        submitBtn->labelsize(13);
        submitBtn->labelfont(FL_BOLD);
        submitBtn->callback(submitCallback, this);
        
        window->end();
        
        Fl::add_handler(eventHandler);
        displayQuestion();
        Fl::add_timeout(1.0, timerCallback, this);
    }
    
    void show() {
        window->show();
    }
    
    ~ExamWindow() {
        Fl::remove_timeout(timerCallback, this);
        delete window;
    }
};

// ============================================================================
// RESULT WINDOW
// ============================================================================

class ResultWindow {
private:
    Fl_Window* window;
    Result result;
    
    static void viewAnalyticsCallback(Fl_Widget* w, void* data) {
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
            
            analytics << "Average Score: " << fixed << setprecision(1) 
                     << avgScore << "%\n";
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
    
    static void exitCallback(Fl_Widget* w, void* data) {
        ResultWindow* resWin = (ResultWindow*)data;
        resWin->window->hide();
        delete resWin;
        currentUser = NULL;
        if (selectedCourse != NULL) {
            delete selectedCourse;
            selectedCourse = NULL;
        }
        showLoginWindow();
    }
    
    static void newExamCallback(Fl_Widget* w, void* data) {
        ResultWindow* resWin = (ResultWindow*)data;
        resWin->window->hide();
        delete resWin;
        if (selectedCourse != NULL) {
            delete selectedCourse;
            selectedCourse = NULL;
        }
        showCourseSelectionWindow();
    }
    
public:
    ResultWindow(Result r) : result(r) {
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
    
    void show() {
        window->show();
    }
    
    ~ResultWindow() {
        delete window;
    }
};

// ============================================================================
// WINDOW CREATION FUNCTIONS
// ============================================================================

void showLoginWindow() {
    LoginWindow* login = new LoginWindow();
    login->show();
}

void showAdminDashboard() {
    AdminDashboard* admin = new AdminDashboard();
    admin->show();
}

void showCourseSelectionWindow() {
    CourseSelectionWindow* csw = new CourseSelectionWindow();
    csw->show();
}

void showInstructionsWindow() {
    InstructionsWindow* inst = new InstructionsWindow();
    inst->show();
}

void showExamWindow() {
    ExamWindow* exam = new ExamWindow();
    exam->show();
}

void showResultWindow(Result result) {
    ResultWindow* resWin = new ResultWindow(result);
    resWin->show();
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main(int argc, char** argv) {
    dbManager = new DatabaseManager();
    
    if (!dbManager) {
        cerr << "Failed to initialize database!" << endl;
        return 1;
    }
    
    showLoginWindow();
    
    int result = Fl::run();
    
    if (dbManager) {
        delete dbManager;
    }
    
    if (currentUser) {
        delete currentUser;
    }
    
    if (selectedCourse) {
        delete selectedCourse;
    }
    
    return result;
}