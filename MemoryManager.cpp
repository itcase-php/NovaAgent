#include "MemoryManager.h"
#include <iostream>
#include <sstream>    // std::stringstream 内存字符串流，格式化时间用
#include <ctime>      // C语言时间库：time_t、tm结构体、localtime系列函数
#include <chrono>     // C++11时间库：system_clock 系统时钟
#include <iomanip>    // 输入输出操纵符：std::put_time 格式化时间

// 粗略估算输入文本信息所消耗的token数
size_t EstimateToken(const std::string& text) {
    // size_t ：无符号整数，专门用来表示数量、长度
    size_t chinese = 0;
    
    // 循环遍历text文本中的每一个字符
    for (char ch : text) {
        // 判断当前字符是否为 非ASCII字符（若是，则认为为中文字符，需要统计）
        if (static_cast<unsigned char>(ch) > 0x7F) {
            chinese++;
        }
    }
    
    // 计算除去中文字符以外，剩余的字符数
    size_t otherCh = text.size() - chinese;
    return chinese * 2 + otherCh; // 粗略估计 1个中文字节 ≈ 2个token，1个ASCII 字符 ≈ 1个token → 算出总 token 的估算值
}

// 工具函数：利用线程安全获取本地实时时间（改进的地方）
std::string getLocaleTime() {
    // 以下代码为获取本地实时时间的代码，使用线程安全获取本地时间字符串
    auto now = std::chrono::system_clock::now(); // 其中 system_clock 为系统时钟，该表达式返回一个静态成员对象
    
    // to_time_t()：把 system_clock::time_point（C++ 时间点） → 转为 C 语言老式类型 std::time_t
    auto in_put_time = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_buf{}; // 准备一个空的时间结构体盒，其中 std::tm 是 C/C++ 标准库定义的时间结构体，用来存放拆解的时间

    /*
    - std::tm tm_buf{} ：自己申请一个 “时间结果盒子”，零初始化
    - #if/#else/#endif ：编译时自动识别系统，选对应平台的安全函数
    - localtime_s / localtime_r ：把时间戳写入你自己的盒子里，不用全局变量，所以多线程同时调用也不会乱。
    */

    // 为了适应不同的操作系统，前者为Windows，后者为MacOS/Linux
    #if defined(_WIN32)
        localtime_s(&tm_buf, &in_put_time);
    #else
        localtime_r(&in_put_time, &tm_buf);
    #endif

    // stringstream 是内存字符串流，可以理解成一块内存缓冲区，就像内存里的 cout，此处执行 s << 是为了把文本写入到这个内存缓冲区 s 里面
    std::stringstream s;
    s << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return s.str();
}

Message::Message(std::string r, std::string c) 
    : role(std::move(r)), context(std::move(c)) {
    timestamp = getLocaleTime();
}

// 显式构造函数（用 explicit 修饰，不会发生隐式类型转换，且该关键词只会修饰构造函数）
MemoryManager::MemoryManager(size_t windowsMaxSize, size_t MaxToken) 
    : m_windowsMaxSize(windowsMaxSize), m_MaxToken(MaxToken) {
    // 默认上下文最大消息条数为20条，token最大消耗数为8192个，也可自行修改
    m_systemPrompt = "这里是 NovaAgent，一个高性能 C++ 原生智能 Agent 内核......";
}

// 计算全部消耗的token数：系统提示词的 token + 每条历史消息的角色 token + 内容 token = 总消耗
size_t MemoryManager::CalculateToken() const {
    size_t total = EstimateToken(m_systemPrompt);
    for (auto it = m_history.begin(); it != m_history.end(); ++it) {
        total += EstimateToken(it->role);
        total += EstimateToken(it->context);
    }
    return total;
}

// 为LLMMessage提供公共接口
std::vector<LLMMessage> MemoryManager::buildmessage() const {
    std::vector<LLMMessage> out;
    out.push_back({"system", m_systemPrompt});
    // 遍历输出
    for (auto it = m_history.begin(); it != m_history.end(); ++it) {
        out.push_back({it->role, it->context});
    }
    // out 是局部变量，函数结束就销毁，返回引用会崩溃，所以返回值就行
    return out;
}

// 获取系统提示词
void MemoryManager::SetsystemPrompt(const std::string& prompt) {
    this->m_systemPrompt = prompt;
}

// 获取上下文信息（滑动窗口核心实现逻辑）
void MemoryManager::addMessage(const std::string& role, const std::string& context) {
    // 当需要插入字符串/自定义类型的变量时，采用 emplace_back 而不是 push_back，若为内置类型（如int,double等）则二者没有区别
    m_history.emplace_back(role, context); // 原地构造 Message 对象，加入 vector 尾部

    // 当发现历史信息超出最大限度时进行清空操作（触发滑动窗口淘汰）
    while (m_history.size() > m_windowsMaxSize) {
        m_history.erase(m_history.begin());
    }

    // 当发现token消耗量超出最大限度时也进行清空操作（触发滑动窗口淘汰）
    while (CalculateToken() > m_MaxToken && !m_history.empty()) { // token超出上限并且对话历史不为空的时候才会清空
        m_history.erase(m_history.begin());
        std::cout << "[Memory] 上下文 Token 超出预算，触发滑动窗口机制淘汰历史记忆！" << std::endl;
    }
}

// 重新获取历史（末尾用const修饰代表为常函数），返回历史的 const 引用，不拷贝，外部只读访问，也不能修改内部 m_history
const std::vector<Message>& MemoryManager::getHistory() const {
    return m_history;
}

// 清空历史记录，但不会重置 m_systemPrompt
void MemoryManager::clearHistory() {
    m_history.clear();
}

// 调试输出信息（末尾用const修饰代表为常函数）
void MemoryManager::debugmessage() const {
    std::cout << "\n=================== MEMORY BUFFER STATE ===================\n";
    std::cout << "SystemPrompt : " << m_systemPrompt << std::endl;
    
    // 循环打印输出 Message 中的所有信息，auto为自动推导类型，此处等价于 Message，写法更加简洁
    for (auto it = m_history.begin(); it != m_history.end(); ++it) {
        std::cout << "[" << it->timestamp << "] " << it->role << ": " << it->context << std::endl;
    }
    
    std::cout << "=============================================================\n";
}

// 提供别名函数，兼容历史调用习惯
void MemoryManager::debugPrint() const {
    debugmessage();
}
