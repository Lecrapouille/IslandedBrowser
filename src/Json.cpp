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

#include "Json.hpp"
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

class Reader
{
public:

    explicit Reader(std::string const& source) : m_source(source), m_index(0) {}

    Json parse()
    {
        skipSpace();
        Json value = parseValue();
        skipSpace();
        if (m_index != m_source.size())
            fail("trailing data");
        return value;
    }

private:

    void fail(char const* message) const
    {
        std::ostringstream out;
        out << message << " at byte " << m_index;
        throw std::runtime_error(out.str());
    }

    void skipSpace()
    {
        while (m_index < m_source.size() &&
               std::isspace(static_cast<unsigned char>(m_source[m_index])))
            ++m_index;
    }

    char peek() const
    {
        if (m_index >= m_source.size())
            fail("unexpected end");
        return m_source[m_index];
    }

    char get()
    {
        char value = peek();
        ++m_index;
        return value;
    }

    Json parseValue()
    {
        // Bookmark trees are shallow; this only stops a malicious file from
        // overflowing the stack.
        if (++m_depth > 512)
            fail("document nested too deeply");
        struct Leave { size_t& depth; ~Leave() { --depth; } } leave{m_depth};
        skipSpace();
        char head = peek();
        if (head == '{')
            return parseObject();
        if (head == '[')
            return parseArray();
        if (head == '"')
            return parseString();
        if (head == 't' || head == 'f')
            return parseBool();
        if (head == 'n')
            return parseNull();
        return parseNumber();
    }

    Json parseObject()
    {
        get();
        Json value;
        value.kind = Json::Object;
        skipSpace();
        if (peek() == '}')
        {
            get();
            return value;
        }
        while (true)
        {
            skipSpace();
            if (peek() != '"')
                fail("object key expected");
            std::string key = parseString().text;
            skipSpace();
            if (get() != ':')
                fail("':' expected");
            value.object.insert(std::make_pair(key, parseValue()));
            skipSpace();
            char separator = get();
            if (separator == '}')
                return value;
            if (separator != ',')
                fail("',' expected");
        }
    }

    Json parseArray()
    {
        get();
        Json value;
        value.kind = Json::Array;
        skipSpace();
        if (peek() == ']')
        {
            get();
            return value;
        }
        while (true)
        {
            value.array.push_back(parseValue());
            skipSpace();
            char separator = get();
            if (separator == ']')
                return value;
            if (separator != ',')
                fail("',' expected");
        }
    }

    Json parseString()
    {
        get();
        Json value;
        value.kind = Json::String;
        while (true)
        {
            char head = get();
            if (head == '"')
                return value;
            if (head != '\\')
            {
                value.text.push_back(head);
                continue;
            }
            char escaped = get();
            if (escaped == '"' || escaped == '\\' || escaped == '/')
                value.text.push_back(escaped);
            else if (escaped == 'b')
                value.text.push_back('\b');
            else if (escaped == 'f')
                value.text.push_back('\f');
            else if (escaped == 'n')
                value.text.push_back('\n');
            else if (escaped == 'r')
                value.text.push_back('\r');
            else if (escaped == 't')
                value.text.push_back('\t');
            else if (escaped == 'u')
                appendUtf8(value.text, parseHex4());
            else
                fail("bad string escape");
        }
    }

    unsigned parseHex4()
    {
        unsigned code = 0;
        for (int digit = 0; digit < 4; ++digit)
        {
            char head = get();
            code <<= 4;
            if (head >= '0' && head <= '9')
                code += static_cast<unsigned>(head - '0');
            else if (head >= 'a' && head <= 'f')
                code += static_cast<unsigned>(head - 'a' + 10);
            else if (head >= 'A' && head <= 'F')
                code += static_cast<unsigned>(head - 'A' + 10);
            else
                fail("bad unicode escape");
        }
        return code;
    }

    void appendUtf8(std::string& text, unsigned code)
    {
        if (code >= 0xD800 && code <= 0xDBFF && peek() == '\\')
        {
            size_t saved = m_index;
            get();
            if (peek() == 'u')
            {
                get();
                unsigned low = parseHex4();
                if (low >= 0xDC00 && low <= 0xDFFF)
                {
                    code = 0x10000 + (((code - 0xD800) << 10) | (low - 0xDC00));
                }
                else
                {
                    m_index = saved;
                }
            }
            else
            {
                m_index = saved;
            }
        }
        if (code < 0x80)
        {
            text.push_back(static_cast<char>(code));
        }
        else if (code < 0x800)
        {
            text.push_back(static_cast<char>(0xC0 | (code >> 6)));
            text.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
        else if (code < 0x10000)
        {
            text.push_back(static_cast<char>(0xE0 | (code >> 12)));
            text.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
        else
        {
            text.push_back(static_cast<char>(0xF0 | (code >> 18)));
            text.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
    }

    Json parseBool()
    {
        Json value;
        value.kind = Json::Bool;
        if (m_source.compare(m_index, 4, "true") == 0)
        {
            value.boolean = true;
            m_index += 4;
            return value;
        }
        if (m_source.compare(m_index, 5, "false") == 0)
        {
            value.boolean = false;
            m_index += 5;
            return value;
        }
        fail("bad boolean");
        return value;
    }

    Json parseNull()
    {
        if (m_source.compare(m_index, 4, "null") != 0)
            fail("bad null");
        m_index += 4;
        return Json();
    }

    Json parseNumber()
    {
        size_t start = m_index;
        if (peek() == '-')
            get();
        if (!std::isdigit(static_cast<unsigned char>(peek())))
            fail("bad number");
        while (m_index < m_source.size() &&
               std::isdigit(static_cast<unsigned char>(m_source[m_index])))
            ++m_index;
        bool integral = true;
        if (m_index < m_source.size() && m_source[m_index] == '.')
        {
            integral = false;
            ++m_index;
            while (m_index < m_source.size() &&
                   std::isdigit(static_cast<unsigned char>(m_source[m_index])))
                ++m_index;
        }
        if (m_index < m_source.size() &&
            (m_source[m_index] == 'e' || m_source[m_index] == 'E'))
        {
            integral = false;
            ++m_index;
            if (m_index < m_source.size() &&
                (m_source[m_index] == '+' || m_source[m_index] == '-'))
                ++m_index;
            while (m_index < m_source.size() &&
                   std::isdigit(static_cast<unsigned char>(m_source[m_index])))
                ++m_index;
        }
        Json value;
        value.kind = Json::Number;
        value.integral = integral;
        value.text = m_source.substr(start, m_index - start);
        try
        {
            value.real = std::stod(value.text);
            if (integral)
                value.integer = std::stoll(value.text);
        }
        catch (std::out_of_range const&)
        {
            fail("number out of range");
        }
        return value;
    }

    std::string const& m_source;
    size_t m_index;
    size_t m_depth = 0;
};

} // namespace

// -----------------------------------------------------------------------------
std::string const& Json::empty()
{
    static std::string const nothing;
    return nothing;
}

// -----------------------------------------------------------------------------
Json const* Json::get(char const* key) const
{
    if (kind != Object)
        return nullptr;
    auto const it = object.find(key);
    return (it == object.end()) ? nullptr : &it->second;
}

// -----------------------------------------------------------------------------
std::string const& Json::str(std::string const& fallback) const
{
    return (kind == String) ? text : fallback;
}

// -----------------------------------------------------------------------------
std::string textOf(Json const* node)
{
    return (node == nullptr) ? std::string() : node->str();
}

// -----------------------------------------------------------------------------
Json Json::parse(std::string const& source)
{
    return Reader(source).parse();
}

// -----------------------------------------------------------------------------
Json Json::load(std::string const& path)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open " + path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string source = buffer.str();
    if (source.compare(0, 3, "\xEF\xBB\xBF") == 0)
        source.erase(0, 3);
    try
    {
        return parse(source);
    }
    catch (std::runtime_error const& error)
    {
        throw std::runtime_error(path + ": " + error.what());
    }
}
