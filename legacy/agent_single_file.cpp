#include <iostream>
#include <string>
#include <vector>
#include <functional>    // 包含 std::function，实现通用可调用对象封装
#include <unordered_map> // 底层实现为哈希表，用于工具注册表的 O(1) 快速寻址
#include <map>           // 底层实现为红黑树，C++17 标准下支持未完成类型（Incomplete Type）递归嵌套
// 以下头文件为获取本地实时时间及高精度性能打点所需的标准库组件
#include <sstream>       // std::stringstream 内存字符串流，格式化时间与输出字符串
#include <ctime>         // C语言时间库：time_t、tm结构体、平台特化的安全时间函数
#include <chrono>        // C++11高精度时间库：system_clock 系统时钟、high_resolution_clock 性能打点
#include <iomanip>       // 输入输出操纵符：std::put_time 格式化输出时间

// =================================================================================
// 轻量级原生 JSON 结构（保证零外部依赖，开箱即用，可无缝替换为 nlohmann/json 或 simdjson）
// =================================================================================
// 用 C++ 语言表示 Json 类，命名空间 mock_json：封装自定义类型，避免与第三方库重名冲突
namespace mock_json {
    class Json {
    private:
        // 私有工具：JSON 字符串特殊字符转义处理（如双引号、反斜杠、换行符）
        static std::string escapeString(const std::string& str){
            std::string res;
            for(char ch : str){
                switch(ch){
                    case '"':  res += "\\\""; break;
                    case '\\': res += "\\\\"; break;
                    case '\n': res += "\\n";  break;
                    case '\t': res += "\\t";  break;
                    case '\r': res += "\\r";  break;
                    default:   res += ch;     break;
                }
            }
            return res;
        }

    public:
        // 注意：C++17 标准明确支持 std::map 与 std::vector 的未完成类型自引用定义（P0175R1）
        std::map<std::string, Json> fields;           // 1. 作为“对象”时使用的键值对字典（利用红黑树天然保证键序）
        std::vector<Json> array_items;                // 2. 作为“数组”时使用的列表
        std::string raw_value;                        // 3. 作为“基本值”时使用的原始标量字符串
        bool is_array = false;                        // 4. 当前节点是否标记为数组

        Json() = default; // 默认构造函数，确保对象可零初始化
        Json(const std::string& val) : raw_value(val) {} // 有参构造函数，初始化为标量字符串
        Json(const char* val) : raw_value(val) {}        // 兼容字符串字面量（避免隐式构造摩擦）

        // 类型转换操作符重载：方便将 Json 标量节点无缝转换为 std::string（极大简化上层接口参数读取）
        operator std::string() const { return raw_value; }
        const std::string& as_string() const { return raw_value; }

        // 访问对象键值：强制确保当前是【对象状态】
        Json& operator[](const std::string& key){
            // 切换为对象节点：清空数组标记与残留标量值，保证内部状态互斥与单语义
            if(is_array || !raw_value.empty()){
                is_array = false;     // 重置数组标记
                raw_value.clear();    // 清理可能残留的原始字符串
                array_items.clear();  // 清理可能残留的数组项
            }
            return fields[key]; // 如果 key 不存在，会自动默认构造一个子 Json 并返回左值引用
        }

        // 追加数组项：强制确保当前是【数组状态】
        void push_back(const Json& item){
            // 切换为数组节点：清空对象字段和原始值，保证内部状态唯一
            if(!is_array){
                is_array = true;      // 标记为数组
                fields.clear();       // 清理对象字典
                raw_value.clear();    // 清理原始字符串
            }
            array_items.push_back(item);
        }

        // 通过 at 进行安全只读访问，不存在则返回全局静态空 Json 节点（避免抛异常与额外堆开销）
        const Json& at(const std::string& key) const {
            auto it = fields.find(key);
            // 找到对应键值迭代器，直接返回常引用
            if(it != fields.end()){
                return it->second;
            }
            // 没找到则返回静态常驻空节点，零开销保证接口安全
            static const Json empty;
            return empty;
        }

        // 判断指定键是否存在（常函数保证无副作用）
        bool contains(const std::string& key) const {
            return fields.find(key) != fields.end();
        }

        // 输出序列化 JSON 文本（支持数组、纯值、对象三种拓扑）
        std::string dump(int indent = 0) const {
            (void)indent; // 抑制未使用的形参警告
            // 1. 如果当前是数组类型
            if(is_array){
                if(array_items.empty()) return "[]";
                std::string res = "[";
                for(size_t i = 0; i < array_items.size(); ++i){
                    res += array_items[i].dump();
                    if(i + 1 < array_items.size()){
                        res += ", ";
                    }
                }
                res += "]";
                return res;
            }

            // 2. 如果当前是基础纯字符串类型
            if(!raw_value.empty()){
                return ("\"" + escapeString(raw_value) + "\""); // 输出双引号包裹的转义字符串
            }

            // 3. 如果当前是键值对对象类型
            if(fields.empty()) return "{}";
            std::string res = "{";
            size_t i = 0;
            for(const auto& [k, v] : fields){
                res += ("\"" + escapeString(k) + "\": " + v.dump());
                if(++i < fields.size()){
                    res += ", ";
                }
            }
            res += "}";
            return res;
        }
    };
}
using Json = mock_json::Json; // 类型别名导出：后续全局代码直接使用 mock_json::Json

// =================================================================================
// 模块1 ：记忆管理模块 --- Sliding Window Context Buffer（滑动窗口上下文缓冲区）
// =================================================================================

// 粗略估算输入文本信息所消耗的 Token 数（用于内存预算与时延控制）
size_t EstimateToken(const std::string &text){
    // size_t：标准无符号整数类型，专门用于表示内存容量与元素计数
    size_t chinese = 0;
    
    // 循环遍历文本字节流
    for(char ch : text){
        // 判断当前字符是否为非 ASCII 字节（UTF-8 中文多字节高位为 1）
        if(static_cast<unsigned char>(ch) > 0x7F){
            chinese++;
        }
    }
    
    // 计算除去多字节之外的单字节字符数
    size_t otherCh = text.size() - chinese;
    // 经验估算：1 个中文 UTF-8 字节序列综合折算约 2 个 Token，ASCII 字符折算约 1 个 Token
    return chinese * 2 + otherCh;
}

// 工具函数：跨平台线程安全获取本地格式化时间戳
std::string getLocaleTime(){
    // 获取标准系统高精度时钟的当前时间点
    auto now = std::chrono::system_clock::now();
    // 转换为 C 语言底层时间戳类型 std::time_t
    auto in_put_time = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_buf{}; // 局部栈分配时间结构体盒子，零初始化，避免全局静态缓冲区竞争

    /*
    - std::tm tm_buf{} ：栈上申请独立内存，避免竞态条件
    - 预编译宏切换：Windows 平台采用 localtime_s，POSIX/Linux/MacOS 平台采用 localtime_r
    - 保证在多线程游戏引擎渲染/物理并发调度下的绝对线程安全
    */
    #if defined(_WIN32)
        localtime_s(&tm_buf, &in_put_time);
    #else
        localtime_r(&in_put_time, &tm_buf);
    #endif

    // 使用内存字符串流配合 put_time 进行格式化输出
    std::stringstream s;
    s << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return s.str();
}

// 对外输出给大模型交互层的数据传输对象（DTO）
struct LLMMessage{
    std::string role;    // 角色标识：system / user / assistant / tool
    std::string context; // 具体消息载荷内容
};

// 内部存储使用的消息历史实体结构
struct Message{
    std::string role;      // 消息角色
    std::string context;   // 消息文本正文
    std::string timestamp; // 消息生成时间戳

    // 构造函数：利用 std::move 触发移动语义，避免深拷贝，构造时自动捕获当前系统时间
    Message(std::string r, std::string c) : role(std::move(r)), context(std::move(c)){
        timestamp = getLocaleTime();
    }
};

// 核心类：基于滑动窗口机制的短期工作记忆管理器（Memory Manager）
class MemoryManager{
private:
    std::string m_systemPrompt;     // 系统人格/引擎基座提示词
    std::vector<Message> m_history; // 环形/线性历史消息存储缓冲区
    size_t m_windowsMaxSize;        // 最大消息条数上限（硬上限限制）
    size_t m_MaxToken;              // 最大允许消耗 Token 数（预算控制）

public:
    // 计算当前历史缓冲区总计占用的 Token 预算
    size_t CalculateToken() const {
        size_t total = EstimateToken(m_systemPrompt);
        for(auto it = m_history.begin(); it != m_history.end(); ++it){
            total += EstimateToken(it->role);
            total += EstimateToken(it->context);
        }
        return total;
    }

    // 显式构造函数（explicit 修饰防隐式转换），配置窗口容量与 Token 预算
    explicit MemoryManager(size_t windowsMaxSize = 20, size_t MaxToken = 8192) 
        : m_windowsMaxSize(windowsMaxSize), m_MaxToken(MaxToken){
        // 初始化默认引擎级系统提示词
        m_systemPrompt = "这里是 NovaAgent，一个高性能 C++ 原生游戏引擎智能 Agent 内核。";
    }
    
    // 构建对外供给 LLM 接口规范的只读序列
    std::vector<LLMMessage> buildmessage() const{
        std::vector<LLMMessage> out;
        // 首条注入系统基座人格
        out.push_back({"system", m_systemPrompt});
        // 遍历并压入所有历史交互
        for(auto it = m_history.begin(); it != m_history.end(); ++it){
            out.push_back({it->role, it->context});
        }
        return out; // 借助 C++11 RVO/NRVO 实现零拷贝返回值优化
    }

    // 设置或动态覆写系统提示词
    void SetsystemPrompt(const std::string &prompt){
        this->m_systemPrompt = prompt;
    }

    // 追加交互消息（滑动窗口核心 FIFO 淘汰策略逻辑）
    void addMessage(const std::string &role, const std::string &context){
        // 使用 emplace_back 在 vector 尾部直接原地构造 Message 对象，消除临时对象拷贝损耗
        m_history.emplace_back(role, context);
        
        // 1. 条数维度限制：当历史消息条数超出窗口容量时，从队头弹出最旧的历史记录
        while(m_history.size() > m_windowsMaxSize){
            m_history.erase(m_history.begin());
        }
        
        // 2. Token 容量维度限制：当 Token 开销超出分配预算时，继续从队头进行动态淘汰
        while(CalculateToken() > m_MaxToken && !m_history.empty()){
            m_history.erase(m_history.begin());
            std::cout << "[Memory] 上下文 Token 超出预算，触发滑动窗口机制淘汰历史记忆！" << std::endl;
        }
    }

    // 获取历史记录常引用：末尾 const 修饰代表常成员函数，只读暴露，外部零拷贝且不可篡改内部状态
    const std::vector<Message>& getHistory() const{
        return m_history;
    }

    // 清空对话上下文历史（保留系统提示词）
    void clearHistory(){
        m_history.clear();
    }

    // 打印当前内存缓冲区状态（常函数），用于开发者诊断和引擎内部调试
    void debugmessage() const{
        std::cout << "\n=================== MEMORY BUFFER STATE ===================\n";
        std::cout << "SystemPrompt : " << m_systemPrompt << std::endl;
        
        // 遍历打印当前存活的每一条消息
        for(auto it = m_history.begin(); it != m_history.end(); ++it){
            std::cout << "[" << it->timestamp << "] " << it->role << ": " << it->context << std::endl;
        }
        
        std::cout << "=============================================================\n";
    }

    // 提供兼容别名函数，提升接口友好度
    void debugPrint() const {
        debugmessage();
    }
};

// =================================================================================
// 模块2 ：自然 C++ 工具调用与注册中心（Native C++ Tool Registry）
// 模拟游戏引擎底层的子系统反射与服务总线（Service Locator / Reflection Dispatch）
// =================================================================================
class ToolRegistry{
public:
    // 类型别名：使用 std::function 泛化封装原生可调用对象（支持 Lambda、静态函数、成员函数绑定）
    using ToolFunction = std::function<std::string(const Json&)>;

    // 工具定义元数据结构体
    struct ToolDefinion{
        std::string t_name;             // 工具全局唯一标识名
        std::string t_description;      // 提供给决策层进行语义匹配的自然语言描述
        std::string t_parameter_schema; // 入参规范协议（Schema）
        ToolFunction execute;           // 底层真正执行该能力的 C++ 回调函数
    };

    // 兼容标准英文拼写的类型别名
    using ToolDefinition = ToolDefinion;

private:
    std::unordered_map<std::string, ToolDefinion> t_registry; // 内部哈希表实现，保证 O(1) 查找速度

public:
    ToolRegistry() = default;

    // 向注册中心挂载原生 C++ 工具能力
    void registryTool(const std::string& name, const std::string& description, const std::string& parameter_schema, ToolFunction func){
        // 利用 std::move 将回调函数移动入映射表，避免 std::function 的深层控制块引用计数开销
        t_registry[name] = ToolDefinion{name, description, parameter_schema, std::move(func)};
        std::cout << "[Registry] 成功挂载原生引擎工具: [" << name << "]" << std::endl;
    }

    // 提供 registerTool 兼容别名函数
    void registerTool(const std::string& name, const std::string& description, const std::string& parameter_schema, ToolFunction func){
        registryTool(name, description, parameter_schema, std::move(func));
    }

    // 判断指定名称的工具是否已登记（常函数，零副作用）
    bool IsName(const std::string& name) const {
        return t_registry.find(name) != t_registry.end(); 
    }

    // 提供 hasTool 别名，提升对外接口的语义自解释性
    bool hasTool(const std::string& name) const {
        return IsName(name);
    }

    // 执行底层工具：集成高精度性能分析器（Profiler）打点与异常安全熔断机制
    std::string executeTool(const std::string& name, const Json& args){
        auto it = t_registry.find(name);
        if(it == t_registry.end()){
            return "Execution Error: Tool '" + name + "' is not registered in this base.";
        }

        // 使用高精度时钟精确测量原生工具逻辑执行耗时（游戏引擎中用于监控单帧 CPU 耗时尖刺）
        auto start = std::chrono::high_resolution_clock::now();
        std::string output;
        try {
            // 执行实际挂载的原生函数
            output = it->second.execute(args);
        }catch(const std::exception& e) {
            // 异常捕获兜底，防止引擎因脚本或工具逻辑崩溃
            output = "Runtime Error during execution: " + std::string(e.what());
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;

        std::cout << "[Registry] Executed tool [" << name << "] in " << elapsed.count() << " ms\n";
        return output;
    }

    // 格式化输出当前注册表中所有可见的原生能力规范
    void printAvailabityTools() const{
        std::cout << "\n================= MOUNTED NATIVE CAPABILITIES =================\n";
        
        for (const auto& [name, tool] : t_registry) {
            std::cout << " - " << name << " : " << tool.t_description << "\n";
            std::cout << "   Params Expected: " << tool.t_parameter_schema << "\n";
        }
        
        std::cout << "===============================================================\n\n";
    }

    // 兼容别名接口
    void printAvailableTools() const {
        printAvailabityTools();
    }
};

// =================================================================================
// 模块3 ：大模型客户端抽象与意图解析层（LLM Client Mock / Stream Layer）
// 在工业级引擎架构中，该层负责与远程服务端（如 OpenAI/Claude）或本地推理后端（llama.cpp）通信
// =================================================================================
class LLMClient{
public:
    // 决策响应实体：封装模型对当前用户意图的分析与调度决断
    struct LLMRespone{
        Json tool_args;             // 工具调用所附带的结构化参数字典
        bool tool_call = false;     // 决策布尔标记：是否需要触发本地 C++ 引擎级工具
        std::string text_context;   // 决策产生的自然语言文本回复
        std::string tool_name;      // 拟调用的原生工具名称标识
    };

    // 兼容别名
    using LLMResponse = LLMRespone;

    LLMClient() = default;

    // 意图解析查询：接收最新用户输入与历史记忆流，返回决策指令
    LLMRespone query(const std::string& last_input, const std::vector<Message>& history){
        (void)history; // 压制当前未直接引用历史记录的编译器警告（实际工程中由模型上下文接收）
        LLMRespone response;

        // 规则意图识别器（Simulated Intent Parser）：模拟大模型对意图的模式匹配与槽位填充
        if(last_input.find("状态") != std::string::npos || last_input.find("status") != std::string::npos){
            response.tool_call = true;
            response.tool_name = "query_system_status";
            response.tool_args["target_module"] = "core_engine";
        }
        else if(last_input.find("路径") != std::string::npos || last_input.find("path") != std::string::npos){
            response.tool_call = true;
            response.tool_name = "calculate_system_minpath";
            response.tool_args["start_node"] = "A01";
            response.tool_args["end_node"] = "G12";
        }
        else {
            response.tool_call = false;
            response.text_context = "收到！我是基于 C++ 构建的微型智能体。当前没有触发特定本地工具，已为您转入纯文本决策层。";
        }

        return response; // 返回意图决策对象
    }
};

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
    explicit NovaAgent(size_t maxLoops = 3) : m_maxExecutionLoops(maxLoops) {}

    // 获取工具注册中心的引用（提供非常量版本与常量只读版本，遵循 C++ 最佳设计规范）
    ToolRegistry& getRegistry() { return m_toolRegistry; }
    const ToolRegistry& getRegistry() const { return m_toolRegistry; }

    // 获取工作记忆管理器的引用
    MemoryManager& getMemory() { return m_memory; }
    const MemoryManager& getMemory() const { return m_memory; }

    // 核心调度驱动函数（Agent Main Dispatch Loop / ReAct Tick）：
    // 接收外部输入刺激（Stimulus），通过感知与多轮自省，驱动工具链并产生最终闭环反馈
    std::string dispatch(const std::string& userRawInput) {
        std::cout << "\n[Agent Loop] 接收到底层输入刺激 (Input Stimulus): \"" << userRawInput << "\"\n";
        
        // 1. 感知阶段：将来自用户/上层引擎系统的原始输入写入工作记忆队列
        m_memory.addMessage("user", userRawInput);

        size_t currentLoop = 0;
        std::string finalAgentOutput = "";
        
        // 2. 启动 ReAct 闭环决策循环：在安全迭代步数内驱动认知决策
        while (currentLoop < m_maxExecutionLoops) {
            currentLoop++;
            std::cout << "[Agent Loop] 触发第 " << currentLoop << " 轮推理决策周期 (Reasoning Cycle)...\n";

            // 调用认知推理层，基于全量上下文历史做出意图与行动决策
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
};

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