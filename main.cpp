#include <cstddef>
#include <cassert>
#include <iostream>
#include <string>
#include <fstream>

/*
 * Little to no syntax checking.
 * Might parse malformed files.
 */

#define PRINT_CONTENT
static int indentation = 0;
static constexpr int indentation_step = 4;

//==============================================================================
class string_view
{
public:
    string_view() : m_begin{ nullptr }, m_end{ nullptr } {}
    string_view(const char * begin, const char * end) : m_begin{ begin }, m_end{ end } { assert(begin <= end); }

    const char & operator [] (std::size_t i) const { assert(i < size()); return m_begin[i]; }

    const char * begin() const { return m_begin; }
    const char * end() const { return m_end; }

    std::size_t size() const { return m_end - m_begin; }

private:
    const char * m_begin;
    const char * m_end;

};

//==============================================================================
struct value
{
    enum class type { T_STRING, T_NUMBER, T_OBJECT, T_ARRAY, T_TRUE, T_FALSE, T_NULL, T_UNKNOWN };

    type type;
    string_view start;
};

//==============================================================================
std::ostream & operator <<(std::ostream & out, const string_view & sv)
{
    out.write(sv.begin(), sv.size());
    return out;
}

//==============================================================================
std::string load_file(const std::string & file_name)
{
    std::ifstream file{ file_name };

    return std::string{
        std::istreambuf_iterator<char>{ file },
        std::istreambuf_iterator<char>{}
    };
}

//==============================================================================
string_view find_string_marker(string_view string)
{
    if (string.size() == 0 || string[0] == '"')
        return string;

    for (std::size_t i = 0; i < string.size() - 1; ++i)
        if (string[i + 1] == '"' && string[i] != '\\')
            return { string.begin() + i + 1, string.end() };

    return { string.end(), string.end() };
}

//==============================================================================
string_view find(string_view string, char c)
{
    for (const char * i = string.begin(); i != string.end(); ++i)
        if (*i == c)
            return { i, string.end() };

    return { string.end(), string.end() };
}

//==============================================================================
string_view find_string(string_view string)
{
    string_view first = find_string_marker(string);

    if (first.size() == 0)
        return first;

    string_view second = find_string_marker({ first.begin() + 1, first.end() });

    if (second.size() == 0)
        return second;

    return { first.begin(), second.begin() + 1 };
}

//==============================================================================
value identify_value(string_view string)
{
    for (const char * i = string.begin(); i != string.end(); ++i)
        switch (*i)
        {
            case '"':
                return { value::type::T_STRING, { i, string.end() } };
            case '{':
                return { value::type::T_OBJECT, { i, string.end() } };
            case '[':
                return { value::type::T_ARRAY, { i, string.end() } };
            case 't':
                return { value::type::T_TRUE, { i, string.end() } };
            case 'f':
                return { value::type::T_FALSE, { i, string.end() } };
            case 'n':
                return { value::type::T_NULL, { i, string.end() } };
            case '0' ... '9':
            case '-':
                return { value::type::T_NUMBER, { i, string.end() } };
            default:
                continue;
        }

    return { value::type::T_UNKNOWN, { string.end(), string.end() } };
}

//==============================================================================
void print_content(string_view content)
{
    for (int i = 0; i < indentation + indentation_step; ++i)
        std::cout << ' ';

    std::cout << content << std::endl;
}

//==============================================================================
string_view parse_string(string_view string)
{
    assert(string.size() != 0 && string[0] == '"');

    string_view end = find_string_marker({ string.begin() + 1, string.end() });

    string_view result;

    if (end.size() == 0)
        result = { string.end(), string.end() };
    else
        result = { string.begin(), end.begin() + 1 };

#ifdef PRINT_CONTENT
    print_content(result);
#endif

    return result;
}

//==============================================================================
bool equal(string_view a, string_view b)
{
    if (a.size() != b.size())
        return false;

    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i])
            return false;

    return true;
}

//==============================================================================
string_view parse_null(string_view string)
{
    assert(string.size() != 0 && string[0] == 'n');

    const char correct_string[]{ "null" };
    std::size_t correct_length = 4;

    string_view correct_sv{ correct_string, correct_string + correct_length };

    string_view result;

    if (string.size() >= correct_length && equal(correct_sv, { string.begin(), string.begin() + correct_length }))
        result = { string.begin(), string.begin() + correct_length };
    else
        result = { string.end(), string.end() };

#ifdef PRINT_CONTENT
    print_content(result);
#endif

    return result;
}

//==============================================================================
string_view parse_true(string_view string)
{
    assert(string.size() != 0 && string[0] == 't');

    const char correct_string[]{ "true" };
    std::size_t correct_length = 4;

    string_view correct_sv{ correct_string, correct_string + correct_length };

    string_view result;

    if (string.size() >= correct_length && equal(correct_sv, { string.begin(), string.begin() + correct_length }))
        result = { string.begin(), string.begin() + correct_length };
    else
        result = { string.end(), string.end() };

#ifdef PRINT_CONTENT
    print_content(result);
#endif

    return result;
}

//==============================================================================
string_view parse_false(string_view string)
{
    assert(string.size() != 0 && string[0] == 'f');

    const char correct_string[]{ "false" };
    std::size_t correct_length = 5;

    string_view correct_sv{ correct_string, correct_string + correct_length };

    string_view result;

    if (string.size() >= correct_length && equal(correct_sv, { string.begin(), string.begin() + correct_length }))
        result = { string.begin(), string.begin() + correct_length };
    else
        result = { string.end(), string.end() };

#ifdef PRINT_CONTENT
    print_content(result);
#endif

    return result;
}

//==============================================================================
string_view parse_number(string_view string)
{
    const char * end = string.begin();

    int state = -1;

    while (end != string.end() && state != 8)
    {
        switch (state)
        {
            case -1:
                if (*end == '-')
                    state = 0;
                else if (*end >= '1' && *end <= '9')
                    state = 1;
                else if (*end == '0')
                    state = 2;
                else
                    state = 0xDEAD;
                break;
            case 1:
                if (*end >= '0' && *end <= '9')
                    state = 1;
                else if (*end == '.')
                    state = 3;
                else if (*end == 'e' || *end == 'E')
                    state = 5;
                else
                    state = 8;
                break;
            case 2:
                if (*end == '.')
                    state = 3;
                else if (*end == 'e' || *end == 'E')
                    state = 5;
                else
                    state = 8;
                break;
            case 3:
                if (*end >= '0' && *end <= '9')
                    state = 4;
                else
                    state = 0xDEAD;
                break;
            case 4:
                if (*end >= '0' && *end <= '9')
                    state = 4;
                else if (*end == 'e' || *end == 'E')
                    state = 5;
                else
                    state = 8;
                break;
            case 5:
                if (*end == '+' || *end == '-')
                    state = 6;
                else
                    state = 0xDEAD;
                break;
            case 6:
                if (*end >= '0' && *end <= '9')
                    state = 7;
                else
                    state = 0xDEAD;
                break;
            case 7:
                if (*end >= '0' && *end <= '9')
                    state = 7;
                else
                    state = 8;
                break;
            default:
                assert(0);
                state = 0xDEAD;
                break;
        }

        if (state == 0xDEAD)
        {
#ifdef PRINT_CONTENT
            print_content({ string.end(), string.end() });
#endif
            return { string.end(), string.end() };
        }

        if (state != 8)
            ++end;
    }

#ifdef PRINT_CONTENT
    print_content({ string.begin(), end });
#endif
    return { string.begin(), end };
}

//==============================================================================
void print_type_name(value type, string_view name)
{
    for (int i = 0; i < indentation; ++i)
        std::cout << ' ';

    switch (type.type)
    {
        case value::type::T_STRING:  std::cout << "STRING "; break;
        case value::type::T_NUMBER:  std::cout << "NUMBER "; break;
        case value::type::T_OBJECT:  std::cout << "OBJECT "; break;
        case value::type::T_ARRAY:   std::cout << "ARRAY "; break;
        case value::type::T_TRUE:    std::cout << "TRUE "; break;
        case value::type::T_FALSE:   std::cout << "FALSE "; break;
        case value::type::T_NULL:    std::cout << "NULL "; break;
        case value::type::T_UNKNOWN: std::cout << "UNKNOWN "; break;
    }

    std::cout << name << " :" << std::endl;

    if (type.type == value::type::T_OBJECT || type.type == value::type::T_ARRAY)
        indentation += indentation_step;
}

//==============================================================================
void print_type(value type)
{
    for (int i = 0; i < indentation; ++i)
        std::cout << ' ';

    switch (type.type)
    {
        case value::type::T_STRING:  std::cout << "STRING"   << std::endl; break;
        case value::type::T_NUMBER:  std::cout << "NUMBER "  << std::endl; break;
        case value::type::T_OBJECT:  std::cout << "OBJECT "  << std::endl; break;
        case value::type::T_ARRAY:   std::cout << "ARRAY "   << std::endl; break;
        case value::type::T_TRUE:    std::cout << "TRUE "    << std::endl; break;
        case value::type::T_FALSE:   std::cout << "FALSE "   << std::endl; break;
        case value::type::T_NULL:    std::cout << "NULL "    << std::endl; break;
        case value::type::T_UNKNOWN: std::cout << "UNKNOWN " << std::endl; break;
    }

    if (type.type == value::type::T_OBJECT || type.type == value::type::T_ARRAY)
        indentation += indentation_step;
}

//==============================================================================
string_view parse_object(string_view string);

//==============================================================================
string_view parse_array(string_view string)
{
    assert(string.size() > 0 && string[0] == '[');

    string_view string_i = string;

    while (string_i.size() > 0)
    {
        string_i = { string_i.begin() + 1, string_i.end() };

        value value_start = identify_value(string_i);

        string_view value_bounds;

        print_type(value_start);

        switch (value_start.type)
        {
            case value::type::T_STRING: value_bounds = parse_string(value_start.start); break;
            case value::type::T_NUMBER: value_bounds = parse_number(value_start.start); break;
            case value::type::T_OBJECT: value_bounds = parse_object(value_start.start); break;
            case value::type::T_ARRAY:  value_bounds = parse_array(value_start.start);  break;
            case value::type::T_TRUE:   value_bounds = parse_true(value_start.start);   break;
            case value::type::T_FALSE:  value_bounds = parse_false(value_start.start);  break;
            case value::type::T_NULL:   value_bounds = parse_null(value_start.start);   break;
            case value::type::T_UNKNOWN: value_bounds = { value_start.start.begin(), string.end() }; break;
        }

        string_i = { value_bounds.end(), string_i.end() };

        {
            const char * i = string_i.begin();

            for (; i != string_i.end(); ++i)
                if (*i == ']' || *i == ',')
                    break;

            string_i = { i, string_i.end() };
        }

        if (string_i.size() == 0)
            break;

        if (string_i[0] == ']')
            break;
        else if (string_i[0] == ',')
        {
            string_i = { string_i.begin() + 1, string_i.end() };
            continue;
        }
        else
            assert(0);
    }

    indentation -= indentation_step;
    if (string_i.size() == 0 || string_i[0] != ']')
        return { string.begin(), string.end() };
    else if (string_i[0] == ']')
        return { string.begin(), string_i.begin() + 1};
    else
        assert(0);
}

//==============================================================================
string_view parse_object(string_view string)
{
    assert(string.size() > 0 && string[0] == '{');

    string_view string_i = string;

    while (string_i.size() > 0)
    {
        string_view element_name = find_string({ string_i.begin() + 1, string_i.end() });

        string_i = { element_name.end(), string_i.end() };

        string_view separator = find(string_i, ':');

        if (separator.size() == 0)
        {
            string_i = { string_i.end(), string_i.end() };
            break;
        }

        string_i = { separator.begin() + 1, string_i.end() };

        value value_start = identify_value(string_i);

        string_view value_bounds;

        print_type_name(value_start, element_name);

        switch (value_start.type)
        {
            case value::type::T_STRING: value_bounds = parse_string(value_start.start); break;
            case value::type::T_NUMBER: value_bounds = parse_number(value_start.start); break;
            case value::type::T_OBJECT: value_bounds = parse_object(value_start.start); break;
            case value::type::T_ARRAY:  value_bounds = parse_array(value_start.start);  break;
            case value::type::T_TRUE:   value_bounds = parse_true(value_start.start);   break;
            case value::type::T_FALSE:  value_bounds = parse_false(value_start.start);  break;
            case value::type::T_NULL:   value_bounds = parse_null(value_start.start);   break;
            case value::type::T_UNKNOWN: value_bounds = { value_start.start.begin(), string.end() }; break;
        }

        string_i = { value_bounds.end(), string_i.end() };

        {
            const char * i = string_i.begin();

            for (; i != string_i.end(); ++i)
                if (*i == '}' || *i == ',')
                    break;

            string_i = { i, string_i.end() };
        }

        if (string_i.size() == 0)
            break;

        if (string_i[0] == '}')
            break;
        else if (string_i[0] == ',')
        {
            string_i = { string_i.begin() + 1, string_i.end() };
            continue;
        }
        else
            assert(0);
    }

    indentation -= indentation_step;
    if (string_i.size() == 0 || string_i[0] != '}')
        return { string.begin(), string.end() };
    else if (string_i[0] == '}')
        return { string.begin(), string_i.begin() + 1};
    else
        assert(0);
}

//==============================================================================
bool init_recursive_test(string_view string)
{
    value type = identify_value(string);

    if (type.type != value::type::T_OBJECT)
        return false;

    string_view r = parse_object(type.start);

    if (r.size() == 0)
        return false;
    return r.begin() == type.start.begin() && *(r.end() - 1) == '}';
}

//==============================================================================
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " json_file_name" << std::endl;
        return 1;
    }

    std::string test_file_string = load_file(argv[1]);
    string_view sv{ test_file_string.data(), test_file_string.data() + test_file_string.size() };


    std::cout << init_recursive_test(sv) << std::endl;

    return 0;
}