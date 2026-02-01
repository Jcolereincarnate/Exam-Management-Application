#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

class User {
public:
    int id;
    string username;
    string passwordHash;
    string role;
    int loginAttempts;
    
    User(int i = 0, string u = "", string ph = "", string r = "candidate");
};

#endif