#include "common.h"

#include <cctype>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <regex>
#include <deque>
#include <cmath>

// -------  Position from common.h  ------

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const
{
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const
{
    return std::pair(row, col) < std::pair(rhs.row, rhs.col);
}

bool Position::IsValid() const
{
    return !(row < 0 || col < 0 || row >= MAX_ROWS || col >= MAX_COLS);
}

char GetCharDigit(int digit, int MAX_DIGIT = LETTERS)
{
    assert(digit >= 0 && digit < MAX_DIGIT);

    return digit + 65;
}

std::string GetStringNumber(int number, const int MAX_DIGIT = LETTERS)
{
    assert(number >= 0);
    assert(MAX_DIGIT > 0 && MAX_DIGIT <= LETTERS);
    int remainder = (number) % (MAX_DIGIT);
    number = (number) / (MAX_DIGIT);
    std::deque<int> d_result;
    while (number != 0)
    {
        d_result.push_front(remainder);
        remainder = (number - 1) % (MAX_DIGIT);
        number = (number - 1) / (MAX_DIGIT);
    }
    d_result.push_front(remainder);
    std::string result;
    for (const int digit : d_result)
    {
        result.push_back(GetCharDigit(digit, MAX_DIGIT));
    }
    return result;
}

std::string Position::ToString() const
{
    if (!IsValid())
    {
        return {};
    }
    return GetStringNumber(col) + (std::to_string(row + 1));
}

int GetDigitFromChar(char ch)
{
    assert(ch > 64 && ch < 92);
    return ch - 65;
}

int ConvertStringToInt(std::string_view str)
{
    int result = 0;
    int index = 0;
    for (const char* ch = str.data() + str.size() - 1; ch > str.data(); --ch)
    {
        result += GetDigitFromChar(*ch) * pow(LETTERS, index) + (index > 0 ? LETTERS : 0);
        ++index;
    }
    if (str.size() > 1)
    {
        result += (GetDigitFromChar(*str.data()) + 1) * pow(LETTERS, index);
    }
    else
    {
        result = GetDigitFromChar(*str.data());
    }
    return result;
}

Position Position::FromString(std::string_view str)
{
    using namespace std::literals;
    std::regex adress("[A-Z]+\\d+"s);
    std::string s = std::string(str.data(), str.size());
    if (!std::regex_match(s, adress) || str.size() > MAX_POSITION_LENGTH)
    {
        return Position::NONE;
    }

    static const std::string_view DEC_DIGITS = "0123456789"sv;
    size_t delim = str.find_first_of(DEC_DIGITS);
    return { std::stoi(str.substr(delim).data()) - 1, ConvertStringToInt(str.substr(0, delim)) };
}

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}

size_t PositionHasher::operator()(const Position key) const
{
    return std::hash<int>()(key.row) + std::hash<int>()(key.col) * 71;
}

// -------  FormulaError from common.h  -------

const std::unordered_map<FormulaError::Category, std::string> FormulaError::string_category_ = {
            std::pair{ Category::Ref,           "#REF"s     },
            std::pair{ Category::Value,         "#VALUE"s   },
            std::pair{ Category::Arithmetic,    "#ARITHM!"s  }
};


std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

FormulaError::Category FormulaError::GetCategory() const
{
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const
{
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const
{
    return string_category_.at(category_);
}
