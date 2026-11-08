/* *****************************************************************************
** MIT License
**
** Copyright (c) 2022 -- 2026 Quentin Quadrat
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
********************************************************************************
*/

#ifndef JSON_HPP
#  define JSON_HPP

#  include <map>
#  include <string>
#  include <vector>

// *****************************************************************************
//! \brief Minimal JSON document, enough to read the bookmark files without
//! an external dependency. Numbers keep their text; integers are also
//! decoded. Strings are UTF-8.
// *****************************************************************************
struct Json
{
    enum Kind { Null, Bool, Number, String, Array, Object };

    Kind kind = Null;
    bool boolean = false;
    bool integral = false;
    long long integer = 0;
    double real = 0.0;
    std::string text;
    std::vector<Json> array;
    std::map<std::string, Json> object;

    //! \brief Member of an object, or nullptr when absent or not an object.
    Json const* get(char const* key) const;
    //! \brief String value, or the fallback when the node is not a string.
    std::string const& str(std::string const& fallback = empty()) const;
    //! \brief True when the node is an array.
    bool isArray() const { return kind == Array; }

    //! \brief Parse a whole document. Throw std::runtime_error with the byte
    //! offset on malformed input.
    static Json parse(std::string const& source);
    //! \brief Read and parse a file (an UTF-8 BOM is accepted).
    static Json load(std::string const& path);

private:

    static std::string const& empty();
};

//! \brief Text of an optional string member ("" when absent).
std::string textOf(Json const* node);

#endif
