#include <iostream>
#include <random>
#include <vector>
#include <windows.h>
#include <string>

class Generator
{
public:
    int m_num;
    void getNum();
};

void Generator::getNum()
{
    std::random_device rd;
    std::seed_seq seed{ rd(), rd(), rd(), rd(), rd(), rd(), rd() };
    std::mt19937_64 gen{ seed };
    std::uniform_int_distribution<int> range{ 0, 91 };

    m_num = std::move(range(gen));
}

unsigned int getLength()
{
    unsigned int lengthInput{};
    std::cout << "How much you want to enter: ";
    std::cin >> lengthInput;

    return lengthInput;
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    std::vector<std::string> passwordChain;
    const std::vector<std::string> letters{ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
                                             "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "0", "1", "2", "3", "4", "5",
                                             "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
                                             "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "!", "\"",
                                             "§", "$", "%", "&", "/", "(", ")", "=", "?", "`", "²", "³", "{", "[", "]", "}",
                                             "+", "*", "-", "_", "@", "<", ">", "|", "°", "^", ";", ",", "." };

    unsigned int length{ getLength() };
    std::string contentInput{};
    Generator gen;

    for (unsigned int i{ 0 }; i < length; ++i)
    {
        std::cout << "Enter name: ";
        std::getline(std::cin >> std::ws, contentInput);

        passwordChain.clear();

        for (const auto& letter : letters)
        {
            gen.getNum();
            passwordChain.push_back(letter + letters[gen.m_num]);
        }

        std::cout << contentInput << ": ";
        for (const auto& password : passwordChain)
            std::cout << password;
        std::cout << '\n';
    }

    std::cin.get();
    return EXIT_SUCCESS;
}
