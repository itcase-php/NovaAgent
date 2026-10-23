#pragma once
#include "Json.h"
#include "MemoryManager.h"
#include <string>
#include <vector>

// =================================================================================
// 模块3 大模型客户端抽象与意图解析层
// =================================================================================
class LLMClient {
public:
    // 大模型返回的决策数据载荷
    struct LLMRespone {
        Json tool_args;             // 工具调用的参数字典
        bool tool_call = false;     // 是否决策触发本地工具调用
        std::string text_context;   // 纯文本决策或反思生成内容
        std::string tool_name;      // 拟调用的工具唯一名称
    };

    // 兼容别名
    using LLMResponse = LLMRespone;

    LLMClient() = default;

    // 模拟 LLM 意图识别与槽位提取（在真实引擎生产环境中，此处会封装 libcurl / WebSocket 发送网络请求）
    LLMRespone query(const std::string& last_input, const std::vector<Message>& history);
};
