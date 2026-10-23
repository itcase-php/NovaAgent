#pragma once
#include "Json.h"
#include <string>
#include <unordered_map> // 底层实现是 哈希表
#include <functional>    // 包含 std::function

// =================================================================================
// 模块2 自然C++工具调用
// =================================================================================
class ToolRegistry {
public:
    using ToolFunction = std::function<std::string(const Json&)>;

    struct ToolDefinion {
        std::string t_name;             // 姓名/工具名
        std::string t_description;      // 给模型的描述
        std::string t_parameter_schema; // 模型调用时需要的参数
        ToolFunction execute;           // 真正的调用函数
    };

    // 兼容别名
    using ToolDefinition = ToolDefinion;

private:
    std::unordered_map<std::string, ToolDefinion> t_registry; // 内部哈希表实现，保证 O(1) 查找速度

public:
    ToolRegistry() = default;

    // 向注册中心登记一个新的原生工具
    void registryTool(const std::string& name, const std::string& description, const std::string& parameter_schema, ToolFunction func);

    // 提供 registerTool 兼容别名
    void registerTool(const std::string& name, const std::string& description, const std::string& parameter_schema, ToolFunction func);

    // 检查工具名称是否存在于注册表中（只读常函数，保证无副作用）
    bool IsName(const std::string& name) const;

    // 提供 hasTool 别名，提升对外接口兼容性
    bool hasTool(const std::string& name) const;

    // 执行底层工具：包含高精度性能统计与异常熔断保护（游戏引擎核心关键要求）
    std::string executeTool(const std::string& name, const Json& args);

    // 打印当前已挂载的所有原生能力列表
    void printAvailabityTools() const;

    // 兼容别名接口
    void printAvailableTools() const;
};
