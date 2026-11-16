#ifndef TERMINAL_TEXT_HPP
#define TERMINAL_TEXT_HPP

#include <string>

namespace TerminalText {
// Return zero for malformed UTF-8 so individual bad bytes can be shown safely.
inline std::size_t characterLength(const std::string& text, std::size_t offset, unsigned int& codepoint) {
    const auto first = static_cast<unsigned char>(text[offset]);
    std::size_t length = 0;
    if (first < 0x80) length = 1;
    else if (first >= 0xc2 && first <= 0xdf) length = 2;
    else if (first >= 0xe0 && first <= 0xef) length = 3;
    else if (first >= 0xf0 && first <= 0xf4) length = 4;
    if (length == 0 || length > text.size() - offset) return 0;
    codepoint = first & (length == 1 ? 0x7f : (0x7f >> length));
    for (std::size_t index = 1; index < length; ++index) {
        const auto next = static_cast<unsigned char>(text[offset + index]);
        if ((next & 0xc0) != 0x80) return 0;
        codepoint = (codepoint << 6) | (next & 0x3f);
    }
    if ((length == 2 && codepoint < 0x80) || (length == 3 && codepoint < 0x800) ||
        (length == 4 && codepoint < 0x10000) || codepoint > 0x10ffff ||
        (codepoint >= 0xd800 && codepoint <= 0xdfff)) return 0;
    return length;
}

inline std::string escape(const std::string& value, bool multiline = false) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string escaped;
    for (std::size_t offset = 0; offset < value.size();) {
        unsigned int codepoint = 0;
        const std::size_t length = characterLength(value, offset, codepoint);
        if (multiline && value[offset] == '\n') {
            escaped += "\r\n";
            ++offset;
        } else if (length != 0 && codepoint >= 0x20 && codepoint != 0x7f &&
                   !(codepoint >= 0x80 && codepoint <= 0x9f)) {
            escaped.append(value, offset, length);
            offset += length;
        } else {
            const auto ch = static_cast<unsigned char>(value[offset++]);
            escaped += "\\x";
            escaped += hex[ch >> 4];
            escaped += hex[ch & 15];
        }
    }
    return escaped;
}

inline void popCharacter(std::string& value) {
    if (value.empty()) return;
    std::size_t start = value.size() - 1;
    while (start > 0 && (static_cast<unsigned char>(value[start]) & 0xc0) == 0x80) --start;
    unsigned int codepoint = 0;
    if (characterLength(value, start, codepoint) != value.size() - start) start = value.size() - 1;
    value.erase(start);
}
}

#endif // TERMINAL_TEXT_HPP
