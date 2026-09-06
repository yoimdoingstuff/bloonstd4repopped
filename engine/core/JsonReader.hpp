#pragma once
#include <string>
#include <string_view>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace btd4::detail {
// JSON is UTF-8. Reject overlong encodings, surrogates and truncated sequences.
inline bool validUtf8(std::string_view text) {
    for (size_t i = 0; i < text.size();) {
        unsigned c = static_cast<unsigned char>(text[i++]);
        if (c < 0x80) continue;
        unsigned count = c >= 0xc2 && c <= 0xdf ? 1 :
            c >= 0xe0 && c <= 0xef ? 2 : c >= 0xf0 && c <= 0xf4 ? 3 : 0;
        if (!count || i + count > text.size()) return false;
        unsigned cp = c & (0x7f >> count);
        for (unsigned j = 0; j < count; ++j) {
            unsigned next = static_cast<unsigned char>(text[i++]);
            if ((next & 0xc0) != 0x80) return false;
            cp = (cp << 6) | (next & 0x3f);
        }
        if ((count == 1 && cp < 0x80) || (count == 2 && cp < 0x800) ||
            (count == 3 && cp < 0x10000) || cp > 0x10ffff ||
            (cp >= 0xd800 && cp <= 0xdfff)) return false;
    }
    return true;
}


class JsonReader {
public:
    explicit JsonReader(std::string_view input) : source(input) {}
protected:
    std::string_view source;
    size_t pos = 0;
    [[noreturn]] void fail(const char* message) const {
        throw std::runtime_error(std::string(message) + " at byte " + std::to_string(pos));
    }
    void space() {
        while (pos < source.size() && (source[pos] == ' ' || source[pos] == '\t' ||
               source[pos] == '\r' || source[pos] == '\n')) ++pos;
    }
    bool take(char c) {
        space();
        if (pos < source.size() && source[pos] == c) { ++pos; return true; }
        return false;
    }
    void expect(char c) { if (!take(c)) fail("Unexpected token"); }
    template<class F> void array(size_t limit, F element) {
        expect('[');
        if (take(']')) return;
        size_t count = 0;
        do {
            if (++count > limit) fail("JSON collection limit exceeded");
            element();
        } while (take(','));
        expect(']');
    }
    unsigned hex4() {
        unsigned result = 0;
        for (int i = 0; i < 4; ++i) {
            if (pos == source.size()) fail("Truncated Unicode escape");
            char c = source[pos++];
            unsigned digit = c >= '0' && c <= '9' ? c - '0' :
                c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
            if (digit == 16) fail("Invalid Unicode escape");
            result = result * 16 + digit;
        }
        return result;
    }
    std::string string() {
        expect('"');
        std::string value;
        while (pos < source.size()) {
            unsigned char c = source[pos++];
            if (c == '"') return value;
            if (c < 32) fail("Control character in string");
            if (c == '\\') {
                if (pos == source.size()) fail("Truncated string escape");
                c = source[pos++];
                switch (c) {
                case '"': case '\\': case '/': value += char(c); break;
                case 'b': value += '\b'; break;
                case 'f': value += '\f'; break;
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                case 'u': {
                    unsigned cp = hex4();
                    if (cp >= 0xd800 && cp <= 0xdbff) {
                        if (pos + 2 > source.size() || source.substr(pos, 2) != "\\u")
                            fail("Missing low surrogate");
                        pos += 2;
                        unsigned low = hex4();
                        if (low < 0xdc00 || low > 0xdfff) fail("Invalid low surrogate");
                        cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
                    } else if (cp >= 0xdc00 && cp <= 0xdfff) fail("Unpaired surrogate");
                    if (cp < 0x80) value += char(cp);
                    else {
                        if (cp >= 0x10000) value += char(0xf0 | (cp >> 18));
                        if (cp >= 0x800) value += char((cp >= 0x10000 ? 0x80 : 0xe0) | ((cp >> 12) & 0x3f));
                        value += char((cp >= 0x800 ? 0x80 : 0xc0) | ((cp >> 6) & 0x3f));
                        value += char(0x80 | (cp & 0x3f));
                    }
                    break;
                }
                default: fail("Invalid string escape");
                }
            } else value += char(c);
            if (value.size() > 1024) fail("JSON string limit exceeded");
        }
        fail("Unterminated string");
    }
    double number() {
        space();
        const size_t start = pos;
        if (pos < source.size() && source[pos] == '-') ++pos;
        auto digit = [&] { return pos < source.size() && source[pos] >= '0' && source[pos] <= '9'; };
        if (!digit()) fail("Expected number");
        if (source[pos] == '0') ++pos;
        else while (digit()) ++pos;
        if (pos < source.size() && source[pos] == '.') {
            ++pos;
            if (!digit()) fail("Expected fractional digits");
            while (digit()) ++pos;
        }
        if (pos < source.size() && (source[pos] == 'e' || source[pos] == 'E')) {
            ++pos;
            if (pos < source.size() && (source[pos] == '+' || source[pos] == '-')) ++pos;
            if (!digit()) fail("Expected exponent digits");
            while (digit()) ++pos;
        }
        if (pos - start > 64) fail("Number too long");
        std::istringstream stream(std::string(source.substr(start, pos - start)));
        stream.imbue(std::locale::classic());
        double value = 0;
        if (!(stream >> value) || !std::isfinite(value)) fail("Invalid finite number");
        return value;
    }
};
}

