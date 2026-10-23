# NovaAgent —— 轻量级 C++ 原生智能体调度核心

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build](https://img.shields.io/badge/Build-CMake-orange.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Zero Dependencies](https://img.shields.io/badge/Dependencies-Zero-brightgreen.svg)]()

`NovaAgent` 是一个基于现代 **C++17** 开发的轻量级原生智能体调度框架。项目模拟了游戏引擎与高性能客户端中智能 NPC 与系统服务的调度逻辑，支持意图解析、本地 C++ 函数动态挂载与执行、高精度耗时统计，并结合滑动窗口机制管理上下文内存。

---

## 📌 核心功能与特性

1. **ReAct 决策闭环与防死循环保护**
   - 实现“感知（输入入队）➔ 推理（意图解析）➔ 行动（工具调度）➔ 反思（结果包装）”的完整调度闭环；
   - 内置最大执行步数限制（默认 3 次），防止模型异常或复杂链条陷入死循环导致程序卡死。

2. **本地函数注册与耗时统计（ToolRegistry）**
   - 使用 `std::function` 与 C++11 Lambda 表达式构建工具注册中心，支持动态挂载本地 C++ 函数（如 A* 寻路算法、系统硬件诊断）；
   - 集成 `std::chrono::high_resolution_clock` 统计毫秒级执行耗时，并设置 `try-catch` 异常安全保护，防止工具异常导致进程崩溃。

3. **滑动窗口上下文记忆管理（MemoryManager）**
   - 设计历史消息队列，结合消息条数上限（20条）与 Token 预算估算，超出阈值时采用先进先出（FIFO）自动淘汰旧记忆；
   - 消息流转全链路运用移动语义（`std::move`）与 `emplace_back` 原地构造，最大化减少不必要的字符串深拷贝；采用线程安全的 `localtime_s` 记录时间戳。

4. **轻量数据基础设施与工程模块化**
   - 零第三方依赖，手写轻量级 Json 嵌套类，利用 `std::map` 实现键值对存储与安全参数解包；
   - 规范拆分为头文件（`.h`）与源文件（`.cpp`），使用 CMake 统一构建，在 `-Wall -Wextra` 选项下实现 0 错误 0 警告编译。

---

## 📂 项目结构

```text
NovaAgent/
├── CMakeLists.txt          # 标准 CMake (3.15+) 构建脚本
├── README.md               # 项目技术说明文档
├── .gitignore              # Git 忽略配置
├── Json.h / Json.cpp       # 基础数据结构：轻量级 Json 树结构与参数解析
├── MemoryManager.h / .cpp  # 模块1：滑动窗口工作记忆与 Token 预算管理
├── ToolRegistry.h / .cpp   # 模块2：本地函数注册中心与高精度耗时统计
├── LLMClient.h / .cpp      # 模块3：意图匹配与槽位提取决策层
├── NovaAgent.h / .cpp      # 模块4：ReAct 闭环调度核心驱动引擎
├── main.cpp                # 宿主装配入口：挂载业务工具、多场景用例验证
└── legacy/                 # 历史演进原型归档
    ├── nova_agent_all_in_one.cpp  # 最初单文件原型
    └── agent_single_file.cpp      # 单文件重构版源码
```

---

## 🛠️ 编译与运行

### 方式 1：使用 CMake 构建（推荐）

```bash
# 1. 生成构建目录
cmake -B build

# 2. 编译项目
cmake --build build

# 3. 运行可执行程序
./build/NovaAgent        # Linux / MacOS
.\build\Debug\NovaAgent.exe # Windows MSVC
.\build\NovaAgent.exe       # Windows MinGW
```

### 方式 2：使用 g++ 命令行直接编译

```bash
g++ -std=c++17 -Wall -Wextra *.cpp -o NovaAgent
.\NovaAgent.exe
```

---

## 🖥️ 运行效果示例

```text
===============================================================
        NovaAgent 高性能 C++ 游戏引擎智能体内核演示            
===============================================================
[Registry] 成功挂载原生引擎工具: [query_system_status]
[Registry] 成功挂载原生引擎工具: [calculate_system_minpath]

================= MOUNTED NATIVE CAPABILITIES =================
 - calculate_system_minpath : 基于引擎空间图论与 NavMesh 寻路算法，计算两点间的最优路径与位移代价
   Params Expected: start_node (string), end_node (string)
 - query_system_status : 查询引擎底层的实时硬件诊断指标、内存占用与各子系统健康状态
   Params Expected: target_module (string)
===============================================================

>>> [测试用例 1] 触发底层引擎诊断工具...
[Agent Loop] 接收到底层输入刺激 (Input Stimulus): "帮我查看一下当前的系统核心引擎状态。"
[Agent Loop] 触发第 1 轮推理决策周期 (Reasoning Cycle)...
[Agent Decision] 模型决策触发本地引擎工具调用: [query_system_status]
[Registry] Executed tool [query_system_status] in 0 ms

>>> Agent 最终决策答复:
根据底层工具 [query_system_status] 的实时反馈成果：
引擎子系统 [core_engine] -> 运行时内存占用: 12.4MB, CPU开销: 0.8%, 渲染帧率: 144 FPS, 状态: ACTIVE_OPTIMAL.
系统诊断与任务执行完成，状态处于健康水平。
```

---

## 📄 License

本项目基于 [MIT](LICENSE) 协议开源。
