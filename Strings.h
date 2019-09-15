/**
 * Copyright (c) 2019-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
