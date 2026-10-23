#pragma once
#include "MemoryManager.h"
#include "ToolRegistry.h"
#include "LLMClient.h"
#include <string>
#include <cstddef>

// =================================================================================
// 模块4 ：智能体执行引擎核心 --- NovaAgent（Agent Engine Core）
// 核心架构：采用 ReAct（Reasoning + Acting）闭环决策范式：
// 感知（Perception） -> 意图推理（Reasoning） -> 动作分发（Acting） -> 反思生成（Reflection）
// =================================================================================
class NovaAgent {
private:
    MemoryManager m_memory;          // 智能体工作记忆组件（负责滑动窗口维护与Token容量管理）
    ToolRegistry m_toolRegistry;     // 原生工具注册中心（解耦挂载游戏引擎各项底层服务接口）
    LLMClient m_client;              // 智能体意图决策与大模型交互客户端
    size_t m_maxExecutionLoops;      // 最大迭代推理步数上限（防止智能体陷入无限死循环的硬防护机制）

public:
    // 显式构造函数：设置最大单次推理步数安全上限（默认 3 步），防止复杂决策链引发卡帧死锁
    explicit NovaAgent(size_t maxLoops = 3);

    // 获取工具注册中心的引用（提供非常量版本与常量只读版本，遵循 C++ 最佳设计规范）
    ToolRegistry& getRegistry();
    const ToolRegistry& getRegistry() const;

    // 获取工作记忆管理器的引用
    MemoryManager& getMemory();
    const MemoryManager& getMemory() const;

    // 核心调度驱动函数（Agent Main Dispatch Loop / ReAct Tick）：
    // 接收外部输入刺激（Stimulus），通过感知与多轮自省，驱动工具链并产生最终闭环反馈
    std::string dispatch(const std::string& userRawInput);
};
