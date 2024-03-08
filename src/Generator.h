#ifndef GENERATOR_H
#define GENERATOR_H

#include <random>

std::string GenerateRandomPassword(int length)
{
    static const char charset[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "!@#$%^&*()_+-=[]{}|;:,.<>?";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> indexDist(0, sizeof(charset) - 2);

    std::string password;
    for (int i = 0; i < length; ++i)
        password += charset[indexDist(gen)];

    return password;
}
#endif