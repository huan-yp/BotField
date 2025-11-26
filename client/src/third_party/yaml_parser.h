#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

//  
// 这里的 Image tag 仅作示意，解释为何 UTF-8 在 C++ std::string 中是安全的：
// UTF-8 的多字节字符的所有字节都在 0x80-0xFF 之间，而符号如 #, :, " 都在 0x00-0x7F 之间。
// 因此简单的字节遍历永远不会错误地把中文字节当成符号。

class SimpleYamlParser {
private:
    std::map<std::string, std::string> data;

    // 辅助函数：修剪字符串两端的空白字符
    std::string trim(const std::string& str) {
        if (str.empty()) return "";
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    // 解析函数
    bool parse(const std::string& filename) {
        data.clear();
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Unable to open file " << filename << std::endl;
            return false;
        }

        std::string line;
        bool firstLine = true;

        while (std::getline(file, line)) {
            // 1. 处理 UTF-8 BOM (Byte Order Mark)
            // Windows 下的记事本保存 UTF-8 有时会带 BOM (0xEF, 0xBB, 0xBF)
            if (firstLine) {
                if (line.size() >= 3) {
                    auto uline = reinterpret_cast<const unsigned char*>(line.c_str());
                    if (uline[0] == 0xEF && uline[1] == 0xBB && uline[2] == 0xBF) {
                        line = line.substr(3);
                    }
                }
                firstLine = false;
            }

            // 2. 移除行尾的 \r (兼容 Windows 换行符在 Linux 环境读取的情况)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // 3. 简单的 trim 预处理
            std::string tempLine = trim(line);
            if (tempLine.empty() || tempLine[0] == '#') continue;

            // 4. 解析逻辑：处理引号和行内注释
            // 注意：UTF-8 中文的字节不会与 ASCII 的 " ' # 冲突，所以这里逐字节遍历是安全的。
            bool inSingleQuote = false;
            bool inDoubleQuote = false;
            size_t commentPos = std::string::npos;
            size_t colonPos = std::string::npos;

            for (size_t i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (c == '"' && !inSingleQuote) {
                    inDoubleQuote = !inDoubleQuote;
                } else if (c == '\'' && !inDoubleQuote) {
                    inSingleQuote = !inSingleQuote;
                } else if (c == '#' && !inSingleQuote && !inDoubleQuote) {
                    commentPos = i;
                    break; // 发现注释，停止这一行的遍历
                } else if (c == ':' && !inSingleQuote && !inDoubleQuote && colonPos == std::string::npos) {
                    colonPos = i; // 记录第一个冒号的位置
                }
            }

            // 如果发现了注释，截断 line
            std::string content = (commentPos != std::string::npos) ? line.substr(0, commentPos) : line;

            // 如果没有冒号，跳过
            if (colonPos == std::string::npos || colonPos >= content.size()) continue;

            // 再次确保冒号是在有效内容区内（虽然上面的循环已经处理了）
            // 分割 Key 和 Value
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1, (commentPos == std::string::npos ? std::string::npos : commentPos - colonPos - 1)));
            
            // 去除 value 可能存在的引号 (可选功能，YAML通常不需要手动去引号，但为了简单解析器可以做)
            if (value.size() >= 2) {
                if ((value.front() == '"' && value.back() == '"') || 
                    (value.front() == '\'' && value.back() == '\'')) {
                    value = value.substr(1, value.size() - 2);
                }
            }

            if (!key.empty()) {
                data[key] = value;
            }
        }
        file.close();
        return true;
    }

    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = data.find(key);
        if (it != data.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
};

#endif // YAML_PARSER_H