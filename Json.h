#pragma once
#include <string>
#include <vector>
#include <map>

// 用C++语言表示Json类，命名空间mock_json：把自己写的 JSON 类包起来，避免和其他库的 JSON 类重名冲突，是 C++ 工程的基本规范
namespace mock_json {
    class Json {
    private:
        // 私有工具：JSON字符串转义，处理双引号、反斜杠、换行等特殊字符
        static std::string escapeString(const std::string& str);

    public:
        // C++17标准中，std::map支持不完整类型定义（P0175R1），而std::unordered_map在部分编译器下会触发static_assert限制
        std::map<std::string, Json> fields;           // 1. 作为“对象”时使用的字典（利用有序map天然实现嵌套结构）
        std::vector<Json> array_items;                // 2. 作为“数组”时使用的列表
        std::string raw_value;                        // 3. 作为“基本值”时使用的原始字符串
        bool is_array = false;                        // 4. 当前是否被标记为数组

        Json() = default;                             // 创建默认构造函数，防止重命名错误
        Json(const std::string& val);                 // 有参构造函数，初始化字符串
        Json(const char* val);                        // 兼容字符串字面量（避免隐式构造摩擦）

        // 类型转换操作符重载：方便直接将 Json 节点当作 std::string 读取（宿主函数参数无缝转换）
        operator std::string() const;
        const std::string& as_string() const;

        // 访问对象键值：强制确保当前是【对象状态】
        Json& operator[](const std::string& key);

        // 追加数组项：强制确保当前是【数组状态】
        void push_back(const Json& item);

        // 通过 at ，安全只读访问，不存在则返回空Json节点
        const Json& at(const std::string& key) const;

        // 判断键值的迭代器是否存在
        bool contains(const std::string& key) const;

        // 输出信息的关键代码（分为了三种：数组类、字符串类型、fields字典类型）
        std::string dump(int indent = 0) const;
    };
}

using Json = mock_json::Json; // 以后使用 Json对象 的时候，就说明使用的是 mock_json 作用域下的 Json
