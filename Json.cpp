#include "Json.h"

namespace mock_json {

    // 私有工具：JSON字符串转义，处理双引号、反斜杠、换行等特殊字符
    std::string Json::escapeString(const std::string& str) {
        std::string res;
        for (char ch : str) {
            switch (ch) {
                case '"':  res += "\\\""; break;
                case '\\': res += "\\\\"; break;
                case '\n': res += "\\n";  break;
                case '\t': res += "\\t";  break;
                case '\r': res += "\\r";  break;
                default:   res += ch;     break;
            }
        }
        return res;
    }

    // 有参构造函数，初始化字符串
    Json::Json(const std::string& val) : raw_value(val) {}

    // 兼容字符串字面量
    Json::Json(const char* val) : raw_value(val) {}

    // 类型转换操作符重载：方便直接将 Json 节点转换为 std::string
    Json::operator std::string() const {
        return raw_value;
    }

    const std::string& Json::as_string() const {
        return raw_value;
    }

    // 访问对象键值：强制确保当前是【对象状态】
    Json& Json::operator[](const std::string& key) {
        // 切换为对象节点：清空数组和原始值，保证状态互斥
        if (is_array || !raw_value.empty()) {
            is_array = false;     // 重置数组标记
            raw_value.clear();    // 清理可能残留的原始字符串
            array_items.clear();  // 清理可能残留的数组项
        }
        return fields[key]; // 如果 key 不存在，会自动默认构造一个子 Json 并返回引用
    }

    // 追加数组项：强制确保当前是【数组状态】
    void Json::push_back(const Json& item) {
        // 切换为数组节点：清空对象字段和原始值，保证状态唯一
        if (!is_array) {
            is_array = true;      // 标记为数组
            fields.clear();       // 清理对象字段
            raw_value.clear();    // 清理原始字符串
        }
        array_items.push_back(item);
    }

    // 通过 at ，安全只读访问，不存在则返回空Json节点
    const Json& Json::at(const std::string& key) const {
        auto it = fields.find(key);
        // 找到这个键值对应的迭代器，就返回字符串/节点
        if (it != fields.end()) {
            return it->second;
        }
        // 没找到就通过静态成员变量，只生成一次，返回空Json节点
        static const Json empty;
        return empty;
    }

    // 判断键值的迭代器是否存在
    bool Json::contains(const std::string& key) const {
        return fields.find(key) != fields.end();
    }

    // 输出信息的关键代码（分为了三种）
    std::string Json::dump(int indent) const {
        (void)indent; // 抑制未使用的形参警告

        // 1、如果是数组类
        if (is_array) {
            if (array_items.empty()) return "[]";
            std::string res = "[";
            for (size_t i = 0; i < array_items.size(); ++i) {
                res += array_items[i].dump();
                if (i + 1 < array_items.size()) {
                    res += ", ";
                }
            }
            res += "]";
            return res;
        }

        // 2、如果是单纯的字符串类型，其中 "\"" 输出结果为 "" - 双引号
        if (!raw_value.empty()) {
            return ("\"" + escapeString(raw_value) + "\""); // 输出字符串，形如："name"
        }

        // 3、如果是fields字典类型
        if (fields.empty()) return "{}";
        std::string res = "{";
        size_t i = 0;
        for (const auto& [k, v] : fields) {
            res += ("\"" + escapeString(k) + "\": " + v.dump());
            if (++i < fields.size()) {
                res += ", ";
            }
        }
        res += "}";
        return res;
    }

} // namespace mock_json
