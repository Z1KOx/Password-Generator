#ifndef GENERATOR_H
#define GENERATOR_H

#include <random>

std::string GenerateRandomPassword(int length, const std::string& charset)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> indexDist(0, charset.size() - 1);

    std::string password;
    for (int i = 0; i < length; ++i)
        password += charset[indexDist(gen)];

    return password;
}

#endif