#include "NovaAgent.h"
#include <iostream>

// =================================================================================
// 运行时能力模拟与业务验证（Runtime Demonstration & Verification）
// 模拟游戏引擎底层核心子系统：诊断监视器（Profiler）与寻路路由引擎（NavMesh/A*）
// =================================================================================

// 模拟游戏引擎底层硬件与运行时性能诊断函数（如渲染帧率、内存开销、CPU占用率）
std::string NativeGetSystemStatus(const Json& args) {
    // 利用 Json 类的类型转换运算符重载，安全解包输入参数
    std::string module = args.at("target_module");
    // 模拟从引擎底层性能探针（Profiler Core）抓取各项核心硬件与调度指标
    return "引擎子系统 [" + module + "] -> 运行时内存占用: 12.4MB, CPU开销: 0.8%, 渲染帧率: 144 FPS, 状态: ACTIVE_OPTIMAL.";
}

// 模拟游戏引擎寻路算法与空间路由计算（如 A* / NavMesh 寻路逻辑）
std::string NativeCalculatePath(const Json& args) {
    std::string start = args.at("start_node");
    std::string end = args.at("end_node");
    // 模拟基于网格拓扑图或导航网格（NavMesh）计算出的最优节点移动序列
    return "NavMesh 寻路序列计算成功. 导航路线: [" + start + " -> 节点_B04 -> 节点_F09 -> " + end + "], 估算总位移代价 (Cost): 14.0.";
}

int main() {
    std::cout << "===============================================================\n";
    std::cout << "        NovaAgent 高性能 C++ 游戏引擎智能体内核演示            \n";
    std::cout << "===============================================================\n";

    // 1. 初始化高性能智能体内核实例，设置最大推理单步上限为 3
    NovaAgent agent(3);

    // 2. 向智能体的工具注册中心挂载原生 C++ 引擎级功能（使用 Lambda 表达式实现零开销闭包传递）
    agent.getRegistry().registryTool(
        "query_system_status",
        "查询引擎底层的实时硬件诊断指标、内存占用与各子系统健康状态",
        "target_module (string)",
        [](const Json& args) { return NativeGetSystemStatus(args); }
    );

    agent.getRegistry().registryTool(
        "calculate_system_minpath",
        "基于引擎空间图论与 NavMesh 寻路算法，计算两点间的最优路径与位移代价",
        "start_node (string), end_node (string)",
        [](const Json& args) { return NativeCalculatePath(args); }
    );

    // 打印当前已挂载的所有原生系统级能力清单
    agent.getRegistry().printAvailabityTools();

    // 3. 测试用例 1: 触发底层引擎诊断工具 (query_system_status)
    std::cout << ">>> [测试用例 1] 触发底层引擎诊断工具...\n";
    std::string result1 = agent.dispatch("帮我查看一下当前的系统核心引擎状态。");
    std::cout << "\n>>> Agent 最终决策答复:\n" << result1 << "\n\n";

    // 4. 测试用例 2: 触发 A* / NavMesh 最优寻路工具 (calculate_system_minpath)
    std::cout << ">>> [测试用例 2] 触发引擎空间寻路工具...\n";
    std::string result2 = agent.dispatch("计算从A01到G12的最优路径");
    std::cout << "\n>>> Agent 最终决策答复:\n" << result2 << "\n\n";

    // 5. 测试用例 3: 触发常规自然语言交互（走纯文本推理分支，不调用任何本地工具）
    std::cout << ">>> [测试用例 3] 纯文本意图对话...\n";
    std::string result3 = agent.dispatch("你好，向你问好！");
    std::cout << "\n>>> Agent 最终决策答复:\n" << result3 << "\n\n";

    // 6. 检视记忆管理器内部的完整上下文轨迹（验证滑动窗口与时间戳机制）
    agent.getMemory().debugmessage();

    std::cout << "===============================================================\n";
    std::cout << "              NovaAgent 内核运行演示顺利结束                   \n";
    std::cout << "===============================================================\n";
    return 0;
}
