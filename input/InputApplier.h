#pragma once
// 输入意图执行器：将 InputIntent3D 转换为实际的按键/鼠标操作
// 使用单独线程异步执行输入，避免阻塞主循环

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdint>
#include "../interception/InterceptionManager.h"
#include "core/logger.h"

namespace bot_input {

// ===================== 3D 输入意图（行为树输出） =====================
struct InputIntent3D {
    int move_fb = 0;   // forward/back:  +1=W, -1=S
    int move_lr = 0;   // left/right:    +1=D, -1=A
    bool sprint = false;   // Shift
    bool crouch = false;   // Ctrl
    bool jump   = false;   // Space
    bool attack = false;   // 例如 "j"
    int look_dx = 0;       // yaw
    int look_dy = 0;       // pitch
    bool lmb_click = false;
    bool rmb_click = false;
    bool lmb_hold = false;
    bool rmb_hold = false;
    std::vector<std::string> tap_keys;
    std::vector<std::string> hold_keys;
    bool stop_all = false;
    
    // 绝对位置点击（屏幕坐标，-1 表示不使用）
    int click_x = -1;      // 点击位置 X 坐标
    int click_y = -1;      // 点击位置 Y 坐标
    bool click_left = true;  // true=左键, false=右键
};
// ===================== 执行器 =====================
class InputApplier {
public:
    InputApplier() {
        interception::manager().init();
        is_terminated_ = false;
        input_thread_ = std::thread(&InputApplier::input_thread_loop, this);
    }

    ~InputApplier() {
        stop();
        release_all();
    }

    // 键位配置
    std::string key_w = "w";
    std::string key_a = "a";
    std::string key_s = "s";
    std::string key_d = "d";
    std::string key_shift = "shift";
    std::string key_ctrl = "ctrl";
    std::string key_space = "space";
    std::string key_attack = "j";
    double tap_sec = 0.03;
    double mouse_tap_sec = 0.02;
    int mouse_move_delay_ms = 10;  // 鼠标移动延迟（毫秒）
    double key_cooldown_sec = 0.1;  // 按键冷却时间（秒），防止频繁触发

    // 更新输入意图
    void apply(const InputIntent3D& it) {
        std::lock_guard<std::mutex> lk(intent_mtx_);
        current_intent_ = it;
        intent_version_++;  // 每次更新意图时递增版本号
    }

    // 停止输入线程
    void stop() {
        if (is_terminated_) return;
        is_terminated_ = true;
        if (input_thread_.joinable()) {
            input_thread_.join();
        }
    }

    // 释放所有按键
    void release_all() {
        auto& mgr = interception::manager();
        mgr.release_all_keys();
        if (heldLmb_) { mgr.mouse_left_up(); heldLmb_ = false; }
        if (heldRmb_) { mgr.mouse_right_up(); heldRmb_ = false; }
    }

private:
    void input_thread_loop() {
        auto& mgr = interception::manager();
        uint64_t last_processed_version = 0;  // 记录上次处理的版本号
        
        while (!is_terminated_) {
            InputIntent3D it;
            uint64_t current_version = 0;
            {
                std::lock_guard<std::mutex> lk(intent_mtx_);
                current_version = intent_version_;

                it = current_intent_;

            }

            if (it.stop_all) {
                release_all();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            // WASD 移动
            bool wantW = (it.move_fb > 0);
            bool wantS = (it.move_fb < 0);
            bool wantD = (it.move_lr > 0);
            bool wantA = (it.move_lr < 0);
            if (wantW) wantS = false;
            if (wantS) wantW = false;
            if (wantA) wantD = false;
            if (wantD) wantA = false;

            if (wantW && !heldW_) { mgr.key_down(key_w); heldW_ = true; }
            if (!wantW && heldW_) { mgr.key_up(key_w); heldW_ = false; }
            if (wantS && !heldS_) { mgr.key_down(key_s); heldS_ = true; }
            if (!wantS && heldS_) { mgr.key_up(key_s); heldS_ = false; }
            if (wantA && !heldA_) { mgr.key_down(key_a); heldA_ = true; }
            if (!wantA && heldA_) { mgr.key_up(key_a); heldA_ = false; }
            if (wantD && !heldD_) { mgr.key_down(key_d); heldD_ = true; }
            if (!wantD && heldD_) { mgr.key_up(key_d); heldD_ = false; }

            // Shift/Ctrl
            if (it.sprint && !heldShift_) { mgr.key_down(key_shift); heldShift_ = true; }
            if (!it.sprint && heldShift_) { mgr.key_up(key_shift); heldShift_ = false; }
            if (it.crouch && !heldCtrl_) { mgr.key_down(key_ctrl); heldCtrl_ = true; }
            if (!it.crouch && heldCtrl_) { mgr.key_up(key_ctrl); heldCtrl_ = false; }

            // 动态按住键
            for (const auto& k : it.hold_keys) {
                if (k.empty()) continue;
                bool found = false;
                for (const auto& hk : dynamic_held_) {
                    if (hk == k) { found = true; break; }
                }
                if (!found) {
                    mgr.key_down(k);
                    dynamic_held_.push_back(k);
                }
            }
            for (size_t i = 0; i < dynamic_held_.size();) {
                bool in_want = false;
                for (const auto& k : it.hold_keys) {
                    if (dynamic_held_[i] == k) { in_want = true; break; }
                }
                if (!in_want) {
                    mgr.key_up(dynamic_held_[i]);
                    dynamic_held_.erase(dynamic_held_.begin() + static_cast<long long>(i));
                } else {
                    ++i;
                }
            }

            // 键盘点按（带冷却时间控制）
            double current_time_sec = std::chrono::duration_cast<std::chrono::duration<double>>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            
            if (it.jump) {
                auto it_time = key_last_press_time_.find(key_space);
                if (it_time == key_last_press_time_.end() || 
                    (current_time_sec - it_time->second) >= key_cooldown_sec) {
                    mgr.press_key(key_space, tap_sec);
                    key_last_press_time_[key_space] = current_time_sec;
                }
            }
            if (it.attack) {
                auto it_time = key_last_press_time_.find(key_attack);
                if (it_time == key_last_press_time_.end() || 
                    (current_time_sec - it_time->second) >= key_cooldown_sec) {
                    mgr.press_key(key_attack, tap_sec);
                    key_last_press_time_[key_attack] = current_time_sec;
                }
            }
            // 检查是否有新的意图更新
            bool has_new_intent = (current_version != last_processed_version);
            
            // 处理 tap_keys（只有在版本更新时才执行）
            if (has_new_intent) {
                for (const auto& k : it.tap_keys) {
                    if (k.empty()) continue;
                    mgr.press_key(k, tap_sec);
                    LOG_INFO("Press buffer_keys:{}", k);
                }
            }

            // 鼠标移动（只有在版本更新时才执行，避免重复执行相同的移动）
            if ( (it.look_dx != 0 || it.look_dy != 0)) {
                mgr.mouse_move_rel(it.look_dx, it.look_dy);
                std::this_thread::sleep_for(std::chrono::milliseconds(mouse_move_delay_ms));
            }
            
            // 更新版本号（在所有处理完成后）
            if (has_new_intent) {
                last_processed_version = current_version;
            }

            // 鼠标按住
            if (it.lmb_hold && !heldLmb_) { mgr.mouse_left_down(); heldLmb_ = true; }
            if (!it.lmb_hold && heldLmb_) { mgr.mouse_left_up(); heldLmb_ = false; }
            if (it.rmb_hold && !heldRmb_) { mgr.mouse_right_down(); heldRmb_ = true; }
            if (!it.rmb_hold && heldRmb_) { mgr.mouse_right_up(); heldRmb_ = false; }

            // 鼠标点击
            if (it.lmb_click) mgr.mouse_click_left();
            if (it.rmb_click) mgr.mouse_click_right();

            // 绝对位置点击（优先于相对点击）
            if (it.click_x >= 0 && it.click_y >= 0) {
                mgr.mouse_click_at(it.click_x, it.click_y, it.click_left, mouse_tap_sec);
            }

            // 输入线程循环间隔
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    std::thread input_thread_;
    std::atomic<bool> is_terminated_{false};

    InputIntent3D current_intent_{};
    std::mutex intent_mtx_;
    std::atomic<uint64_t> intent_version_{0};  // 意图版本号，用于检测是否有新意图

    bool heldW_{false}, heldA_{false}, heldS_{false}, heldD_{false};
    bool heldShift_{false}, heldCtrl_{false};
    bool heldLmb_{false}, heldRmb_{false};
    std::vector<std::string> dynamic_held_;
    std::map<std::string, double> key_last_press_time_;  // 记录每个按键的上次触发时间
};

} // namespace bot_inputWDDDAWDSA