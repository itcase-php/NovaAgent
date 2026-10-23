#include "LLMClient.h"

// 模拟 LLM 意图识别与槽位提取（在真实引擎生产环境中，此处会封装 libcurl / WebSocket 发送网络请求）
LLMClient::LLMRespone LLMClient::query(const std::string& last_input, const std::vector<Message>& history) {
    (void)history; // 压制当前未直接引用历史记录的编译器警告（实际工程中由模型上下文接收）
    LLMRespone response;

    // 基于关键词匹配模拟模型的意图识别决策（Intent Recognition）
    if (last_input.find("状态") != std::string::npos || last_input.find("status") != std::string::npos) {
        response.tool_call = true;
        response.tool_name = "query_system_status";
        response.tool_args["target_module"] = "core_engine";
    }
    else if (last_input.find("路径") != std::string::npos || last_input.find("path") != std::string::npos) {
        response.tool_call = true;
        response.tool_name = "calculate_system_minpath";
        response.tool_args["start_node"] = "A01";
        response.tool_args["end_node"] = "G12";
    }
    else {
        response.tool_call = false;
        response.text_context = "收到！我是基于 C++ 构建的微型智能体。当前没有触发特定本地工具，已为您转入纯文本决策层。";
    }

    return response; // 完善返回值，修复之前遗漏的 return
}
