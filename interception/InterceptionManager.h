#pragma once
// Interception 驱动管理器（单例）

#include "Keyboard.h"
#include "Mouse.h"
#include <windows.h>
#include <mutex>
#include <unordered_set>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cctype>

namespace interception {

class InterceptionManager {
public:
    static InterceptionManager& instance() {
        static InterceptionManager inst;
        return inst;
    }

    // 初始化（线程安全）
    bool init() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (initialized_) return true;

        if (!keyboard_.init()) {
            return false;
        }
        if (!mouse_.init()) {
            keyboard_.shutdown();
            return false;
        }

        initialized_ = true;
        return true;
    }

    // 关闭
    void shutdown() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!initialized_) return;

        release_all_keys();
        keyboard_.shutdown();
        mouse_.shutdown();
        pressed_keys_.clear();
        initialized_ = false;
    }

    // 键盘操作
    void key_down(const std::string& key) {
        ensure_init();
        WORD vk = get_vk(key);
        if (vk == 0) return;

        {
            std::lock_guard<std::mutex> lk(mtx_);
            pressed_keys_.insert(vk);
        }
        keyboard_.key_down(key);
    }

    void key_up(const std::string& key) {
        ensure_init();
        WORD vk = get_vk(key);
        if (vk == 0) return;

        keyboard_.key_up(key);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            pressed_keys_.erase(vk);
        }
    }

    void press_key(const std::string& key, double duration_sec = 0.05) {
        key_down(key);
        interruptible_sleep_ms(static_cast<int>(duration_sec * 1000.0));
        key_up(key);
    }

    void release_all_keys() {
        ensure_init();
        std::unordered_set<WORD> copy;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            copy = pressed_keys_;
        }
        // 通过键盘对象释放所有按键
        for (WORD vk : copy) {
            keyboard_.key_up_vk(vk);
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            pressed_keys_.clear();
        }
    }

    // 鼠标操作
    void mouse_move_rel(int dx, int dy) {//鼠标旋转视角
        ensure_init();
        mouse_.move_rel(dx, dy);
    }

    void mouse_move_abs(int x, int y) {//鼠标移动到绝对位置
        ensure_init();
        mouse_.move_abs(x, y);
    }

    void mouse_left_down()  { ensure_init(); mouse_.left_down(); }
    void mouse_left_up()    { ensure_init(); mouse_.left_up(); }
    void mouse_right_down() { ensure_init(); mouse_.right_down(); }
    void mouse_right_up()   { ensure_init(); mouse_.right_up(); }

    void mouse_click_left(double sec = 0.15) {
        mouse_left_down();
        interruptible_sleep_ms(static_cast<int>(sec * 1000.0));
        mouse_left_up();
    }

    void mouse_click_right(double sec = 0.15) {
        mouse_right_down();
        interruptible_sleep_ms(static_cast<int>(sec * 1000.0));
        mouse_right_up();
    }

    // 在指定位置点击（先移动再点击）
    void mouse_click_at(int x, int y, bool left_button = true, double click_duration_sec = 0.15) {
        ensure_init();
        mouse_move_abs(x, y);
        interruptible_sleep_ms(10);  // 等待鼠标移动到位置
        if (left_button) {
            mouse_click_left(click_duration_sec);
        } else {
            mouse_click_right(click_duration_sec);
        }
    }

    // 获取键盘/鼠标对象（高级用法）
    Keyboard& keyboard() { ensure_init(); return keyboard_; }
    Mouse& mouse() { ensure_init(); return mouse_; }

private:
    InterceptionManager() = default;
    ~InterceptionManager() { shutdown(); }

    InterceptionManager(const InterceptionManager&) = delete;
    InterceptionManager& operator=(const InterceptionManager&) = delete;

    void ensure_init() {
        if (!initialized_) {
            if (!init()) {
                std::fprintf(stderr, "[InterceptionManager] Init failed. (driver installed? x64 match? dll near exe?)\n");
                std::exit(1);
            }
        }
    }

    WORD get_vk(const std::string& key) {
        return key_to_vk(key);
    }

    static WORD key_to_vk(const std::string& key) {
        std::string lower = key;
        for (auto& ch : lower)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        if (lower == "left")  return VK_LEFT;
        if (lower == "right") return VK_RIGHT;
        if (lower == "up")    return VK_UP;
        if (lower == "down")  return VK_DOWN;
        if (lower == "space") return VK_SPACE;
        if (lower == "shift") return VK_SHIFT;
        if (lower == "ctrl")  return VK_CONTROL;
        if (lower == "alt")   return VK_MENU;
        if (lower == "tab")   return VK_TAB;
        if (lower == "enter") return VK_RETURN;
        if (lower == "esc")   return VK_ESCAPE;

        if (lower == "f1")  return VK_F1;
        if (lower == "f2")  return VK_F2;
        if (lower == "f3")  return VK_F3;
        if (lower == "f4")  return VK_F4;
        if (lower == "f5")  return VK_F5;
        if (lower == "f6")  return VK_F6;
        if (lower == "f7")  return VK_F7;
        if (lower == "f8")  return VK_F8;
        if (lower == "f9")  return VK_F9;
        if (lower == "f10") return VK_F10;
        if (lower == "f11") return VK_F11;
        if (lower == "f12") return VK_F12;

        if (lower.size() == 1) {
            unsigned char c = static_cast<unsigned char>(lower[0]);
            if (std::isdigit(c)) return static_cast<WORD>(c);
            if (std::isalpha(c)) return static_cast<WORD>(std::toupper(c));
        }

        return 0;
    }

    void interruptible_sleep_ms(int total_ms) {
        const int step = 10;
        for (int t = 0; t < total_ms && !stop_flag_.load(std::memory_order_relaxed); t += step)
            ::Sleep(step);
    }

private:
    Keyboard keyboard_;
    Mouse mouse_;
    std::mutex mtx_;
    std::unordered_set<WORD> pressed_keys_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> stop_flag_{false};
};

// 便捷函数
inline InterceptionManager& manager() {
    return InterceptionManager::instance();
}

} // namespace interception

