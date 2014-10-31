#pragma once

#include <map>
#include <list>
#include <string>

namespace handy {

struct Conf {
    // 0 success
    // -1 IOERROR
    // >0 line no of error
    int parse(const std::string& filename1);

    // Get a string value from INI file, returning default_value if not found.
    std::string get(std::string section, std::string name, std::string default_value);

    // Get an integer (long) value from INI file, returning default_value if
    // not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    long getInteger(std::string section, std::string name, long default_value);

    // Get a real (floating point double) value from INI file, returning
    // default_value if not found or not a valid floating point value
    // according to strtod().
    double getReal(std::string section, std::string name, double default_value);

    // Get a boolean value from INI file, returning default_value if not found or if
    // not a valid true/false value. Valid true values are "true", "yes", "on", "1",
    // and valid false values are "false", "no", "off", "0" (not case sensitive).
    bool getBoolean(std::string section, std::string name, bool default_value);

    // Get a string value from INI file, returning empty list if not found.
    std::list<std::string> getStrings(std::string section, std::string name);

    std::map<std::string, std::list<std::string>> values_;
    std::string filename;
};

}