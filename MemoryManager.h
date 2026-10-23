#pragma once
#include <string>
#include <vector>
#include <cstddef>

// =================================================================================
// 模块1 ：记忆管理模块 --- Sliding Window Context Buffer（滑动窗口上下文缓冲区）
// =================================================================================

// 粗略估算输入文本信息所消耗的token数
size_t EstimateToken(const std::string& text);

// 工具函数：利用线程安全获取本地实时时间（改进的地方）
std::string getLocaleTime();

// 对外输出给大模型的消息结构（改进的地方）
struct LLMMessage {
    std::string role;    // 角色：system/user/assistant/tool
    std::string context; // 上下文内容
};

// 内部存储用的消息结构
struct Message {
    std::string role;      // 角色：user/assistant/tool
    std::string context;   // 上下文内容
    std::string timestamp; // 时间戳

    // 构造函数：利用 std::move 触发移动语义，避免深拷贝，构造时自动捕获当前系统时间
    Message(std::string r, std::string c);
};

// 核心类：滑动窗口内存管理器类
class MemoryManager {
private:
    std::string m_systemPrompt;     // 系统提示词
    std::vector<Message> m_history; // 消息历史列表
    size_t m_windowsMaxSize;        // 最大消息条数上限
    size_t m_MaxToken;              // 最大限制消耗token数

public:
    // 显式构造函数（用 explicit 修饰，不会发生隐式类型转换，且该关键词只会修饰构造函数）
    // 默认上下文最大消息条数为20条，token最大消耗数为8192个，也可自行修改
    explicit MemoryManager(size_t windowsMaxSize = 20, size_t MaxToken = 8192);

    // 计算全部消耗的token数：系统提示词的 token + 每条历史消息的角色 token + 内容 token = 总消耗
    size_t CalculateToken() const;

    // 为LLMMessage提供公共接口
    std::vector<LLMMessage> buildmessage() const;

    // 获取/设置系统提示词
    void SetsystemPrompt(const std::string& prompt);

    // 获取上下文信息（滑动窗口核心实现逻辑）
    void addMessage(const std::string& role, const std::string& context);

    // 重新获取历史（末尾用const修饰代表为常函数），返回历史的 const 引用，不拷贝，外部只读访问，也不能修改内部 m_history
    const std::vector<Message>& getHistory() const;

    // 清空历史记录，但不会重置 m_systemPrompt
    void clearHistory();

    // 调试输出信息（末尾用const修饰代表为常函数）
    void debugmessage() const;

    // 提供别名函数，兼容历史调用习惯
    void debugPrint() const;
};
