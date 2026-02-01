#include "User.h"

User::User(int i, string u, string ph, string r)
    : id(i), username(u), passwordHash(ph), role(r), loginAttempts(0) {}