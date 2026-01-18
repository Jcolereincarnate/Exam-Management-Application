
# Exam System
## Overview

This is a **desktop exam management system** built with **C++**, **SQLite**, and **FLTK** for the GUI.  
It allows administrators to manage courses, questions, and users, and provides candidates (students) with an interface to take exams.  

The system supports features like:

- User authentication (admin and candidate roles)
- Course management
- Question management
- Exam generation and randomization
- Result tracking
- Database clearing for testing or resetting

## Features
### 1. User Management
- **Roles**: `admin` and `candidate`
- Admin can create, authenticate, and manage users
- Login attempts are tracked, with account lockout after 5 failed attempts
- Passwords are stored securely using SHA256 hashing
### 2. Course Management
- Admin can add, edit, and delete courses
- Each course has:
  - Course code
  - Course title
  - Time allocation (in minutes)
  - Number of questions per exam
  - Passing mark percentage
- Courses can be viewed in a browser UI on the admin dashboard
### 3. Question Management
- Questions are linked to a specific course (`course_id`)
- Each question contains:
  - Question text
  - Four options (A, B, C, D)
  - Correct answer
  - Points awarded
- Admin can add multiple questions per course
- Randomized question selection for exams
### 4. Exam Generation
- Candidate selects the course they want to take
- Exam consists of the number of questions defined by the course
- Questions are selected randomly from the course question pool
- Exam timing is enforced based on the course’s time allocation
### 5. Result Tracking
- Exam results are saved in the `results` table
- Each result stores:
  - User ID and username
  - Course ID, code, and title
  - Score, total points, and percentage
  - Time spent
  - Pass/fail status
- Admin can view all results or filter by candidate

### Install Dependecies
mac: brew install fltk sqlite3 openssl
Windows (MinGW):
Download and install FLTK, SQLite3, and OpenSSL libraries

### Compile
MacOs: g++ -std=c++11 file_name.cpp -lfltk -lsqlite3 -lcrypto -o exam_system
./exam_system
Windows: g++ -std=c++11 file_name.cpp -mwindows -lfltk -lsqlite3 -lcrypto -o exam_system.exe
exam_system.exe

### BULK QUESTION UPLOAD FORMAT:
 * Create a text file with questions in this format:
 * 
 * Q: What is the capital of France?
 * A: Paris
 * B: London
 * C: Berlin
 * D: Madrid
 * ANSWER: A
 * 
 * Q: What is 2 + 2?
 * A: 3
 * B: 4
 * C: 5
 * D: 6
 * ANSWER: B