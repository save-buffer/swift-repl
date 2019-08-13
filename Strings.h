#ifndef STRINGS_H
#define STRINGS_H

#include <algorithm>
#include <functional>
#include <string>


static inline void LTrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
}

static inline void RTrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

static inline void Trim(std::string &s)
{
    LTrim(s);
    RTrim(s);
}

static inline void ToLowerCase(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](char c) { return std::tolower(c); });
}

static inline bool StartsWith(std::string str, std::string start)
{
    return str.find(start) == 0;
}

#endif
