#include "SyntaxHighlighter.hpp"
#include <unordered_set>
#include <cctype>

namespace scriptable {

std::vector<HighlightSpan> SyntaxHighlighter::highlight(const std::string& code) {
    static const std::unordered_set<std::string> keywords = {
        "let", "if", "else", "while", "repeat", "and", "or"
    };
    static const std::unordered_set<std::string> builtins = {
        "player", "camera", "obj", "color", "wait",
        "abs", "min", "max", "sin", "cos"
    };
    static const std::unordered_set<std::string> booleans = {
        "true", "false"
    };

    std::vector<HighlightSpan> spans;
    size_t i = 0;
    size_t len = code.size();

    while (i < len) {
        // Comments
        if (i + 1 < len && code[i] == '/' && code[i + 1] == '/') {
            size_t start = i;
            while (i < len && code[i] != '\n') i++;
            spans.push_back({start, i - start, COLOR_COMMENT});
            continue;
        }

        // Numbers
        if (std::isdigit(code[i]) || (code[i] == '.' && i + 1 < len && std::isdigit(code[i + 1]))) {
            size_t start = i;
            while (i < len && (std::isdigit(code[i]) || code[i] == '.')) i++;
            spans.push_back({start, i - start, COLOR_NUMBER});
            continue;
        }

        // Identifiers and keywords
        if (std::isalpha(code[i]) || code[i] == '_') {
            size_t start = i;
            while (i < len && (std::isalnum(code[i]) || code[i] == '_')) i++;
            std::string word = code.substr(start, i - start);

            if (keywords.count(word)) {
                spans.push_back({start, i - start, COLOR_KEYWORD});
            } else if (builtins.count(word)) {
                spans.push_back({start, i - start, COLOR_BUILTIN});
            } else if (booleans.count(word)) {
                spans.push_back({start, i - start, COLOR_BOOL});
            }
            // Default colored text doesn't need a span
            continue;
        }

        i++;
    }

    return spans;
}

} // namespace scriptable
