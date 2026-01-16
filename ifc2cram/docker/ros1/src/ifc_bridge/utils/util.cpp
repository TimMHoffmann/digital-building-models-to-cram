#include "util.hpp"
#include <sstream>
#include <stdexcept>

// Hez-value of a hex character, or -1 if invalid
static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;
}

// UTF-8-Encoding
static void decode_utf8(unsigned int codepoint, std::string& out) {
    if (codepoint < 0x80) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

std::string decode_ifc_string(const std::string& yytext) {
    size_t len = yytext.size();
    if (len < 2 || yytext.front() != '\'' || yytext.back() != '\'') {
        return yytext; // not an IFC string format
    }

    std::string out;

    for (size_t i = 1; i < len - 1;) {
        // Double ' → real '
        if (i + 1 < len - 1 && yytext[i] == '\'' && yytext[i + 1] == '\'') {
            out.push_back('\'');
            i += 2;
        }
        // \X2...Hex...\X0
        else if (i + 1 < len - 1 && yytext[i] == '\\' && yytext[i + 1] == 'X') {
            i += 2; // skip \X
            if (i < len && (yytext[i] == '2' || yytext[i] == '4')) {
                i++; // skip encoding marker
            }

            while (i < len - 1) {
                // End sequence: \X0
                if (i + 3 < len - 1 && yytext[i] == '\\' && yytext[i + 1] == 'X' &&
                    yytext[i + 2] == '0' && yytext[i + 3] == '\\') {
                    i += 4;
                    break;
                }

                if (i + 3 >= len - 1) break;

                int h1 = hexval(yytext[i]);
                int h2 = hexval(yytext[i + 1]);
                int h3 = hexval(yytext[i + 2]);
                int h4 = hexval(yytext[i + 3]);

                if (h1 < 0 || h2 < 0 || h3 < 0 || h4 < 0) {
                    i++;
                    continue;
                }

                unsigned int codepoint = (h1 << 12) | (h2 << 8) | (h3 << 4) | h4;
                decode_utf8(codepoint, out);
                i += 4;
            }
        }
        // normal character
        else {
            out.push_back(yytext[i]);
            i++;
        }
    }

    return out;
}
