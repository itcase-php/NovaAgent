#include "ToolRegistry.h"
#include <iostream>
#include <chrono>

// 向注册中心登记一个新的原生工具
void ToolRegistry::registryTool(const std::string& name, const std::string& description, const std::string& parameter_schema, ToolFunction func) {
    t_registry[name] = ToolDefinion{name, description, parameter_schema, std::move(func)};
    std::cout << "[Registry] 成功挂载原生引擎工具: [" << name << "]" << std::endl;
}

void ToolRegistry::registerTool(const std::string& name, const std::string& description, const std::string& parameter_schema, ToolFunction func) {
    registryTool(name, description, parameter_schema, std::move(func));
}

// 检查工具名称是否存在于注册表中（只读常函数，保证无副作用）
bool ToolRegistry::IsName(const std::string& name) const {
    return t_registry.find(name) != t_registry.end();
}

bool ToolRegistry::hasTool(const std::string& name) const {
    return IsName(name);
}

// 执行工具：包含高精度性能统计与异常熔断保护（游戏引擎核心关键要求）
std::string ToolRegistry::executeTool(const std::string& name, const Json& args) {
    auto it = t_registry.find(name);
    if (it == t_registry.end()) {
        return "Execution Error: Tool '" + name + "' is not registered in this base.";
    }

    // 使用 high_resolution_clock 测量底层原生系统调用的微秒/毫秒耗时，用于性能热点排查
    auto start = std::chrono::high_resolution_clock::now();
    std::string output;
    try {
        output = it->second.execute(args);
    } catch (const std::exception& e) {
        output = "Runtime Error during execution: " + std::string(e.what());
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "[Registry] Executed tool [" << name << "] in " << elapsed.count() << " ms\n";
    return output;
}

// 打印当前已挂载的所有原生能力列表
void ToolRegistry::printAvailabityTools() const {
    std::cout << "\n================= MOUNTED NATIVE CAPABILITIES =================\n";
    for (const auto& [name, tool] : t_registry) {
        std::cout << " - " << name << " : " << tool.t_description << "\n";
        std::cout << "   Params Expected: " << tool.t_parameter_schema << "\n";
    }
    std::cout << "===============================================================\n\n";
}

void ToolRegistry::printAvailableTools() const {
    printAvailabityTools();
}
