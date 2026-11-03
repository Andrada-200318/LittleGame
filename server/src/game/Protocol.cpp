#include "Protocol.h"

#include <charconv>
#include <system_error>

namespace Protocol {

std::vector<std::string> split(const std::string& line, int fieldCount) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    for (int i = 0; i < fieldCount; ++i) {
        const std::size_t pipe = line.find('|', start);
        if (pipe == std::string::npos) {
            break;
        }
        parts.push_back(line.substr(start, pipe - start));
        start = pipe + 1;
    }
    parts.push_back(line.substr(start));
    return parts;
}

std::string format(const std::string& command, const std::vector<std::string>& fields) {
    std::string line = command;
    for (const std::string& field : fields) {
        line += '|';
        line += field;
    }
    return line;
}

bool parseRoundId(const std::string& text, int& roundId) {
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, roundId);
    return result.ec == std::errc() && result.ptr == end;
}

}
