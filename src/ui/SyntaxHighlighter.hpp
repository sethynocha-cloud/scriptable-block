#pragma once
#include <string>
#include <vector>
#include <cocos2d.h>

namespace scriptable {

struct HighlightSpan {
    size_t start;
    size_t length;
    cocos2d::ccColor3B color;
};

class SyntaxHighlighter {
public:
    static std::vector<HighlightSpan> highlight(const std::string& code);

    // Color scheme
    static constexpr cocos2d::ccColor3B COLOR_KEYWORD = {102, 217, 239};    // Cyan
    static constexpr cocos2d::ccColor3B COLOR_NUMBER = {174, 129, 255};     // Purple
    static constexpr cocos2d::ccColor3B COLOR_BUILTIN = {166, 226, 46};     // Green
    static constexpr cocos2d::ccColor3B COLOR_COMMENT = {117, 113, 94};     // Gray
    static constexpr cocos2d::ccColor3B COLOR_BOOL = {174, 129, 255};       // Purple
    static constexpr cocos2d::ccColor3B COLOR_DEFAULT = {248, 248, 242};    // Off-white
};

} // namespace scriptable
