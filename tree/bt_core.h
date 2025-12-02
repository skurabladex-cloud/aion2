
#pragma once
#include <memory>
#include <vector>
#include <string>
#include <chrono>

class core;

// ===================== 基础类型 =====================
enum class BTStatus {
    SUCCESS,
    FAILURE,
    RUNNING
};

// ===================== 黑板 =====================
struct BTBlackboard {
    class core* bot = nullptr;

    class core *get_bot() const { return bot; }
    
    // 行为树执行路径跟踪
    std::vector<std::string> execution_path_;
    
    // 获取当前执行路径（以 " -> " 分隔）
    std::string get_path_string() const {
        if (execution_path_.empty()) {
            return "";
        }
        std::string path;
        for (size_t i = 0; i < execution_path_.size(); ++i) {
            if (i > 0) path += " -> ";
            path += execution_path_[i];
        }
        return path;
    }
    
    // 推入路径节点
    void push_path(const std::string& node_name) {
        execution_path_.push_back(node_name);
    }
    
    // 弹出路径节点
    void pop_path() {
        if (!execution_path_.empty()) {
            execution_path_.pop_back();
        }
    }
};

// ===================== 节点基类 =====================
class BTNode {
public:
    explicit BTNode(std::string name) : name_(std::move(name)) {}
    virtual ~BTNode() = default;

    virtual BTStatus tick(BTBlackboard& bb) = 0;//定义所有行为树节点的统一执行接口

    const std::string& name() const { return name_; }

protected:
    std::string name_;
};

// ===================== 条件节点基类 =====================
class BTCondition : public BTNode {
public:
    explicit BTCondition(const std::string& name) : BTNode(name) {}

    BTStatus tick(BTBlackboard& bb) override {
        return evaluate(bb) ? BTStatus::SUCCESS : BTStatus::FAILURE;
    }

    virtual bool evaluate(BTBlackboard& bb) = 0;
};

// ===================== 行为节点基类 =====================
class BTAction : public BTNode {
public:
    explicit BTAction(const std::string& name) : BTNode(name) {}

    // 可选：进入/退出钩子
//     当节点 从非活动状态 → 活动状态 时调用
// 也就是：
//
// 第一次 tick 到这个节点
//
// 或者重新选择了这个节点
//
// 或者父节点重新激活它
//
// 或者 Selector 切换到这个分支
//
// 常用于：
//
// 初始化
//
// 重置变量
//
// 打开某些开关
//
// 计算初始方向
//
// 进入状态时播放动画
//
// 清空路径
//
// 初始化计时器
    virtual void on_enter(BTBlackboard&) {}
    virtual void on_exit(BTBlackboard&) {}
//     对，这个 on_exit() 是 结束该行为节点时执行的清理逻辑。
// 我现在把它在你 ActFindingRune（寻找符文） 这个动作里的含义讲清楚：

    // 默认 tick：执行 do_tick（路径记录和日志输出在子类中处理）
    BTStatus tick(BTBlackboard& bb) override {
        return do_tick(bb);
    }

    virtual BTStatus do_tick(BTBlackboard& bb) = 0;
};

// ===================== 组合节点：Sequence =====================
class BTSequence final : public BTNode {
public:
    explicit BTSequence(const std::string& name = "Sequence")
        : BTNode(name) {}

    void add_child(std::shared_ptr<BTNode> node) {
        children_.push_back(std::move(node));
    }

    BTStatus tick(BTBlackboard& bb) override {
        // 推入当前节点到路径
        bb.push_path(name_);
        
        // 所有子节点顺序执行：遇到 FAILURE / RUNNING 就停
        BTStatus result = BTStatus::SUCCESS;
        for (auto& child : children_) {
            BTStatus s = child->tick(bb);
            if (s != BTStatus::SUCCESS) {
                result = s;
                break;
            }
        }
        
        // 弹出当前节点
        bb.pop_path();
        
        return result;
    }

private:
    std::vector<std::shared_ptr<BTNode>> children_;
};

// ===================== 组合节点：Selector =====================
class BTSelector final : public BTNode {
public:
    explicit BTSelector(const std::string& name = "Selector")
        : BTNode(name) {}

    void add_child(std::shared_ptr<BTNode> node) {
        children_.push_back(std::move(node));
    }

    BTStatus tick(BTBlackboard& bb) override {
        // 推入当前节点到路径
        bb.push_path(name_);
        
        // 优先级从前到后：找到第一个不是 FAILURE 的就返回
        BTStatus result = BTStatus::FAILURE;
        for (auto& child : children_) {
            BTStatus s = child->tick(bb);
            if (s != BTStatus::FAILURE) {
                result = s; // SUCCESS or RUNNING
                break;
            }
        }
        
        // 弹出当前节点
        bb.pop_path();
        
        return result;
    }

private:
    std::vector<std::shared_ptr<BTNode>> children_;
};

std::shared_ptr<BTNode> build_main_bt_tree() ;
