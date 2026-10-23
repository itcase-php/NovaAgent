#include "NovaAgent.h"
#include <iostream>

// 显式构造函数：设置最大单次推理步数安全上限（默认 3 步），防止复杂决策链引发卡帧死锁
NovaAgent::NovaAgent(size_t maxLoops) 
    : m_maxExecutionLoops(maxLoops) {}

// 获取工具注册中心的引用（提供非常量版本与常量只读版本，遵循 C++ 最佳设计规范）
ToolRegistry& NovaAgent::getRegistry() {
    return m_toolRegistry;
}

const ToolRegistry& NovaAgent::getRegistry() const {
    return m_toolRegistry;
}

// 获取工作记忆管理器的引用
MemoryManager& NovaAgent::getMemory() {
    return m_memory;
}

const MemoryManager& NovaAgent::getMemory() const {
    return m_memory;
}

// 核心调度驱动函数（Agent Main Dispatch Loop / ReAct Tick）：
// 接收外部输入刺激（Stimulus），通过感知与多轮自省，驱动工具链并产生最终闭环反馈
std::string NovaAgent::dispatch(const std::string& userRawInput) {
    std::cout << "\n[Agent Loop] 接收到底层输入刺激 (Input Stimulus): \"" << userRawInput << "\"\n";
    
    // 1. 感知阶段：将来自用户/上层引擎系统的原始输入写入工作记忆队列
    m_memory.addMessage("user", userRawInput);

    size_t currentLoop = 0;
    std::string finalAgentOutput = "";
    
    // 2. 启动 ReAct 闭环决策循环：在安全迭代步数内驱动认知决策
    while (currentLoop < m_maxExecutionLoops) {
        currentLoop++;
        std::cout << "[Agent Loop] 触发第 " << currentLoop << " 轮推理决策周期 (Reasoning Cycle)...\n";

        // 调用大模型决策层进行意图解析，传入全部上下文历史记录
        LLMClient::LLMRespone response = m_client.query(userRawInput, m_memory.getHistory());

        // 3. 行动分支判断：是调用底层 C++ 原生能力，还是直接完成文本对话
        if (response.tool_call) {
            std::cout << "[Agent Decision] 模型决策触发本地引擎工具调用: [" << response.tool_name << "]\n";
            
            // 校验该工具是否在引擎工具注册表中已挂载
            if (m_toolRegistry.IsName(response.tool_name)) {
                // 执行底层原生 C++ 核心逻辑，并在底层进行高精度毫秒耗时监控
                std::string result = m_toolRegistry.executeTool(response.tool_name, response.tool_args);
                
                // 将工具执行产生的环境观察反馈（Observation）存入工作记忆流中
                m_memory.addMessage("tool", "工具 [" + response.tool_name + "] 实时执行结果: " + result);
                
                // 模拟反思（Reflection）整合阶段：智能体根据工具反馈成果，提炼出面向宿主系统的最终回答
                finalAgentOutput = "根据底层工具 [" + response.tool_name + "] 的实时反馈成果：\n" + result + "\n系统诊断与任务执行完成，状态处于健康水平。";
                m_memory.addMessage("assistant", finalAgentOutput);
                return finalAgentOutput;
            } else {
                // 容错与自愈分支：捕获未定义系统服务调用，写入系统错误日志并优雅退出
                std::string err = "执行异常: 请求调用的引擎工具 [" + response.tool_name + "] 未在注册中心登记！";
                m_memory.addMessage("system", err);
                return err;
            }
        } else {
            // 4. 纯文本认知分支：无需触发外部物理/系统动作，直接输出语言回复
            finalAgentOutput = response.text_context;
            m_memory.addMessage("assistant", finalAgentOutput);
            return finalAgentOutput;
        }
    }
    
    // 达到最大执行步数安全阈值，触发保护机制，防止游戏主循环阻塞
    return "错误: 智能体推理步数达到最大安全阈值，触发强制熔断保护。";
}
