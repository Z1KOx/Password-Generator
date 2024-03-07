#include "Generator.h"

void Generator::getNum()
{
    std::random_device rd;
    std::seed_seq seed{ rd(), rd(), rd(), rd(), rd(), rd(), rd() };
    std::mt19937_64 gen{ seed };
    std::uniform_int_distribution<int> range{ 0, 91 };

    m_num = std::move(range(gen));
}