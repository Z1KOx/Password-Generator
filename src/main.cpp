#include <iostream>
#include <random>
#include <vector>
#include <windows.h>

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

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    const std::vector<std::string_view> letters{ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
                                             "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "0", "1", "2", "3", "4", "5",
                                             "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
                                             "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "!", "\"",
                                             "§", "$", "%", "&", "/", "(", ")", "=", "?", "`", "²", "³", "{", "[", "]", "}",
                                             "+", "*", "-", "_", "@", "<", ">", "|", "°", "^", ";", ",", "." };

    Generator gen;
    for (unsigned int i{ 0 }; i < letters.size(); ++i)
    {
        gen.getNum();
            
        std::cout << letters[gen.m_num];
    }

    std::cin.get();
    return EXIT_SUCCESS;
}
