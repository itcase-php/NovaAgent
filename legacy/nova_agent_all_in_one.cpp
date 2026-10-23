/**
 * =================================================================================
 * Project Name: NovaAgent (Micro High-Performance Agent Base)
 * Language: C++ (C++17 / C++20 Recommended)
 * Description: A lightweight, ultra-low latency, and production-ready Agent base
 * designed for embedded systems, game NPC brains, and high-concurrency
 * backend services. It provides core abstractions for Tool Registration,
 * Memory Management (Sliding Window), and a ReAct-style Agent Loop.
 * * Dependencies: nlohmann/json (Embedded in this file as a simulated structured data 
 * interface to ensure zero-setup compilation). In a production environment,
 * simply replace the mock JSON types with the real 'nlohmann/json.hpp' 
 * or 'simdjson'.
 * =================================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

// =================================================================================
// SIMULATED LIGHTWEIGHT JSON STRUCTURE (To guarantee compile-and-run out-of-the-box)
// =================================================================================
namespace mock_json {
    class Json {
    public:
        std::unordered_map<std::string, std::string> fields;
        std::vector<Json> array_items;
        std::string raw_value;
        bool is_array = false;

        Json() = default;
        Json(const std::string& val) : raw_value(val) {}

        std::string& operator[](const std::string& key) {
            return fields[key];
        }

        const std::string& at(const std::string& key) const {
            auto it = fields.find(key);
            if (it != fields.end()) return it->second;
            static const std::string empty = "";
            return empty;
        }

        void push_back(const Json& item) {
            is_array = true;
            array_items.push_back(item);
        }

        bool contains(const std::string& key) const {
            return fields.find(key) != fields.end();
        }

        std::string dump(int indent = 0) const {
            if (is_array) {
                std::string res = "[";
                for (size_t i = 0; i < array_items.size(); ++i) {
                    res += array_items[i].dump();
                    if (i + 1 < array_items.size()) res += ", ";
                }
                res += "]";
                return res;
            }
            if (!raw_value.empty()) {
                return "\"" + raw_value + "\"";
            }
            std::string res = "{";
            size_t i = 0;
            for (const auto& [k, v] : fields) {
                res += "\"" + k + "\": \"" + v + "\"";
                if (++i < fields.size()) res += ", ";
            }
            res += "}";
            return res;
        }
    };
}
using Json = mock_json::Json;

// =================================================================================
// MODULE 1: MEMORY MANAGEMENT (Sliding Window Context Buffer)  --------   模块一已完成！
// =================================================================================
struct Message {
    std::string role;    // "system", "user", "assistant", "tool"
    std::string content;
    std::string timestamp;

    Message(std::string r, std::string c) : role(std::move(r)), content(std::move(c)) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        timestamp = ss.str();
    }
};

class MemoryManager {
private:
    std::vector<Message> history_;
    size_t max_window_size_; // Maximum tokens/messages to prevent buffer overflow
    std::string system_prompt_;

public:
    explicit MemoryManager(size_t max_window_size = 20) 
        : max_window_size_(max_window_size) {
        system_prompt_ = "You are NovaAgent, a high-performance C++ native intelligent agent core.";
    }

    void setSystemPrompt(const std::string& prompt) {
        system_prompt_ = prompt;
    }

    void addMessage(const std::string& role, const std::string& content) {
        // Enforce memory bounds using sliding window mechanism
        if (history_.size() >= max_window_size_) {
            // Evict oldest conversation pair (user + assistant), preserving system prompt
            if (history_.size() > 2) {
                history_.erase(history_.begin(), history_.begin() + 2);
            } else {
                history_.erase(history_.begin());
            }
            std::cout << "[Memory] Window limit reached. Evicted old context molecules.\n";
        }
        history_.emplace_back(role, content);
    }

    const std::vector<Message>& getHistory() const {
        return history_;
    }

    void clear() {
        history_.clear();
    }

    void debugPrint() const {
        std::cout << "\n=================== MEMORY BUFFER STATE ===================\n";
        std::cout << "System Prompt: " << system_prompt_ << "\n";
        for (const auto& msg : history_) {
            std::cout << "[" << msg.timestamp << "] " << msg.role << ": " << msg.content << "\n";
        }
        std::cout << "===========================================================\n\n";
    }
};

// =================================================================================
// MODULE 2: NATIVE C++ TOOL REGISTRY (Function Calling Engine)
// =================================================================================
class ToolRegistry {
public:
    using ToolFunction = std::function<std::string(const Json&)>;

    struct ToolDefinition {
        std::string name;
        std::string description;
        std::string parameter_schema; // Simple text representation of expectations
        ToolFunction execute;
    };

private:
    std::unordered_map<std::string, ToolDefinition> registry_;

public:
    ToolRegistry() = default;

    // Registers a native C++ Lambda, Member Function, or Static Function
    void registerTool(const std::string& name, const std::string& description, 
                      const std::string& param_schema, ToolFunction func) {
        registry_[name] = ToolDefinition{name, description, param_schema, std::move(func)};
        std::cout << "[Registry] Successfully mounted native tool: [" << name << "]\n";
    }

    bool hasTool(const std::string& name) const {
        return registry_.find(name) != registry_.end();
    }

    std::string executeTool(const std::string& name, const Json& args) {
        auto it = registry_.find(name);
        if (it == registry_.end()) {
            return "Execution Error: Tool '" + name + "' is not registered in this base.";
        }
        
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

    void printAvailableTools() const {
        std::cout << "\n================= MOUNTED NATIVE CAPABILITIES =================\n";
        for (const auto& [name, tool] : registry_) {
            std::cout << " - " << name << " : " << tool.description << "\n";
            std::cout << "   Params Expected: " << tool.parameter_schema << "\n";
        }
        std::cout << "===============================================================\n\n";
    }
};

// =================================================================================
// MODULE 3: LLM CLIENT (Low-Latency Stream/Mock Layer)
// =================================================================================
class LLMClient {
public:
    struct LLMResponse {
        bool trigger_tool_call = false;
        std::string tool_name;
        Json tool_args;
        std::string text_content;
    };

    LLMClient() = default;

    // Simulation Engine that acts as the LLM parsing layer.
    // In production, this method utilizes standard sockets or libcurl to hit endpoints.
    LLMResponse query(const std::string& latest_input, const std::vector<Message>& history) {
        LLMResponse response;
        
        // Simple predictive heuristic to simulate LLM planning based on intent
        if (latest_input.find("状态") != std::string::npos || latest_input.find("status") != std::string::npos) {
            response.trigger_tool_call = true;
            response.tool_name = "query_system_status";
            response.tool_args["target_module"] = "core_engine";
        } 
        else if (latest_input.find("路径") != std::string::npos || latest_input.find("path") != std::string::npos) {
            response.trigger_tool_call = true;
            response.tool_name = "calculate_optimal_path";
            response.tool_args["start_node"] = "A01";
            response.tool_args["end_node"] = "G12";
        } 
        else {
            response.trigger_tool_call = false;
            response.text_content = "收到！我是基于 C++ 构建的微型智能体。当前没有触发特定本地工具，已为您转入纯文本决策层。";
        }
        return response;
    }
};

// =================================================================================
// MODULE 4: AGENT ENGINE CORE
// =================================================================================
class NovaAgent {
private:
    MemoryManager memory_;
    ToolRegistry tool_registry_;
    LLMClient client_;
    int max_execution_loops_; // Guard against Infinite Thought Loops

public:
    explicit NovaAgent(int max_loops = 3) : max_execution_loops_(max_loops) {}

    ToolRegistry& getRegistry() { return tool_registry_; }
    MemoryManager& getMemory() { return memory_; }

    std::string dispatch(const std::string& user_raw_input) {
        std::cout << "\n[Agent Loop] Received input stimulus: \"" << user_raw_input << "\"\n";
        memory_.addMessage("user", user_raw_input);

        int current_loop = 0;
        std::string final_agent_output = "";
        
        while (current_loop < max_execution_loops_) {
            current_loop++;
            std::cout << "[Agent Loop] Initiating Reasoning Cycle (Iteration " << current_loop << ")...\n";

            // Query the LLM model core with full memory matrix
            LLMClient::LLMResponse response = client_.query(user_raw_input, memory_.getHistory());

            if (response.trigger_tool_call) {
                std::cout << "[Agent Decision] Model decided to execute tool: [" << response.tool_name << "]\n";
                
                if (tool_registry_.hasTool(response.tool_name)) {
                    // Execute the C++ Tool
                    std::string result = tool_registry_.executeTool(response.tool_name, response.tool_args);
                    
                    // Store the action feedback back into the memory lane
                    memory_.addMessage("tool", "Result of [" + response.tool_name + "]: " + result);
                    
                    // Simulate the reflection step where Agent processes tool output to formulate final speech
                    final_agent_output = "根据底层工具 [" + response.tool_name + "] 的实时反馈成果：\n" + result + "\n系统诊断完成，状态处于健康水平。";
                    memory_.addMessage("assistant", final_agent_output);
                    return final_agent_output;
                } else {
                    std::string err = "Tool [" + response.tool_name + "] was requested but is missing.";
                    memory_.addMessage("system", err);
                    return err;
                }
            } else {
                // Regular textual response without specialized action loops
                final_agent_output = response.text_content;
                memory_.addMessage("assistant", final_agent_output);
                return final_agent_output;
            }
        }
        
        return "Error: Maximum execution safety threshold reached without termination.";
    }
};

// =================================================================================
// RUNTIME DEMONSTRATION & VERIFICATION
// =================================================================================

// Define mock business logic functions to act as native C++ capabilities
std::string NativeGetSystemStatus(const Json& args) {
    std::string module = args.at("target_module");
    return "Module [" + module + "] -> Memory Usage: 12.4MB, CPU Overhead: 0.8%, Status: ACTIVE_OPTIMAL.";
}

std::string NativeCalculatePath(const Json& args) {
    std::string start = args.at("start_node");
    std::string end = args.at("end_node");
    return "Pathfinding sequence generated successfully. Route: [" + start + " -> B04 -> F09 -> " + end + "]. Total weight/cost: 14.";
}

int main() {
    std::cout << "===============================================================\n";
    std::cout << "        Initializing NovaAgent High-Performance C++ Core       \n";
    std::cout << "===============================================================\n";

    // 1. Initialize our high-performance agent base
    NovaAgent agent;

    // 2. Register native C++ functions into the Agent's capability array
    agent.getRegistry().registerTool(
        "query_system_status",
        "Queries real-time hardware and software diagnostic metrics of the engine.",
        "target_module (string)",
        [](const Json& args) { return NativeGetSystemStatus(args); }
    );

    agent.getRegistry().registerTool(
        "calculate_optimal_path",
        "Computes the shortest path using a high-performance graph routing backend.",
        "start_node (string), end_node (string)",
        [](const Json& args) { return NativeCalculatePath(args); }
    );

    // Print configured state
    agent.getRegistry().printAvailableTools();

    // 3. Execution Run #1: Triggering System Status Verification Tool
    std::string result1 = agent.dispatch("帮我查看一下当前的系统核心引擎状态。");
    std::cout << "\n>>> Agent Final Reply:\n" << result1 << "\n\n";

    // 4. Execution Run #2: Triggering Pathfinding Tool
    std::string result2 = agent.dispatch("计算从A01到G12的最优路径");
    std::cout << "\n>>> Agent Final Reply:\n" << result2 << "\n\n";

    // 5. Execution Run #3: Standard Context handling
    std::string result3 = agent.dispatch("你好，向你问好！");
    std::cout << "\n>>> Agent Final Reply:\n" << result3 << "\n\n";

    // Inspect historical memory tracks inside the memory manager
    agent.getMemory().debugPrint();

    std::cout << "===============================================================\n";
    std::cout << "              NovaAgent Demo Execution Terminated              \n";
    std::cout << "===============================================================\n";
    return 0;
}
