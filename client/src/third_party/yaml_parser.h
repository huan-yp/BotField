#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>

class SimpleYamlParser {
private:
    std::map<std::string, std::string> data;

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    bool parse(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // 移除行首/尾空白
            line = trim(line);
            if (line.empty()) continue;

            // 移除行内注释（忽略引号内的 #）
            bool inSingleQuote = false;
            bool inDoubleQuote = false;
            for (size_t i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (c == '"' && !inSingleQuote) {
                    inDoubleQuote = !inDoubleQuote;
                } else if (c == '\'' && !inDoubleQuote) {
                    inSingleQuote = !inSingleQuote;
                } else if (c == '#' && !inSingleQuote && !inDoubleQuote) {
                    // 剪掉注释起始位置及其后的内容
                    line = trim(line.substr(0, i));
                    break;
                }
            }

            // 跳过空行或以 # 开头的整行注释
            if (line.empty() || line[0] == '#') continue;

            // 解析 key: value
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = trim(line.substr(0, colonPos));
                std::string value = trim(line.substr(colonPos + 1));
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
