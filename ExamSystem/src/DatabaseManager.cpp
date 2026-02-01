#include "DatabaseManager.h"
#include "Utils.h"
#include <FL/fl_ask.H>
#include <sstream>
#include <cstdio>
#include "Result.h"
using namespace std;

DatabaseManager::DatabaseManager(string path) : db(NULL), dbPath(path) {
    initDatabase();
}

DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close(db);
    }
}
bool DatabaseManager::initDatabase() {
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

void DatabaseManager::createTables() {
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

void DatabaseManager::insertDefaultData() {
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


bool DatabaseManager::addUser(string username, string password, string role) {
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

User* DatabaseManager::authenticateUser(string username, string password) {
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

void DatabaseManager::incrementLoginAttempts(string username) {
    const char* sql = "UPDATE users SET login_attempts = login_attempts + 1 WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

void DatabaseManager::resetLoginAttempts(string username) {
    const char* sql = "UPDATE users SET login_attempts = 0 WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

bool DatabaseManager::addCourse(string code, string title, int time, int questionsCount, int passing) {
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

vector<Course> DatabaseManager::getAllCourses() {
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

Course* DatabaseManager::getCourseById(int courseId) {
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

int DatabaseManager::getCourseIdByCode(string code) {
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


bool DatabaseManager::addQuestion(int courseId, string qText, string optA, string optB, 
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

vector<Question> DatabaseManager::getRandomQuestions(int courseId, int count) {
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

// ============================================================================
// Result Management Methods
// ============================================================================

int DatabaseManager::saveResult(Result r) {
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

vector<Result> DatabaseManager::getResults(int userId) {
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