#pragma once
// Interception 键盘驱动封装

#include <windows.h>
#include <interception.h>
#include <string>
#include <cctype>

namespace interception {

class Keyboard {
public:
    Keyboard() : ctx_(nullptr), dev_(0) {}

    bool init() {
        ctx_ = interception_create_context();
        if (!ctx_) return false;
        dev_ = INTERCEPTION_KEYBOARD(0);
        return true;
    }

    void shutdown() {
        if (ctx_) {
            interception_destroy_context(ctx_);
            ctx_ = nullptr;
        }
    }

    // 按键按下
    void key_down(const std::string& key) {
        WORD vk = key_to_vk(key);
        if (vk == 0 || !ctx_) return;
        key_down_vk(vk);
    }

    // 按键释放
    void key_up(const std::string& key) {
        WORD vk = key_to_vk(key);
        if (vk == 0 || !ctx_) return;
        key_up_vk(vk);
    }

    // 按键按下（使用 VK）
    void key_down_vk(WORD vk) {
        if (!ctx_ || vk == 0) return;

        unsigned short sc = 0;
        bool e0 = false;
        vk_to_scancode(vk, sc, e0);

        InterceptionKeyStroke k{};
        k.code = sc;
        k.state = INTERCEPTION_KEY_DOWN;
        if (e0) k.state |= INTERCEPTION_KEY_E0;

        interception_send(ctx_, dev_, reinterpret_cast<InterceptionStroke*>(&k), 1);
    }

    // 按键释放（使用 VK）
    void key_up_vk(WORD vk) {
        if (!ctx_ || vk == 0) return;

        unsigned short sc = 0;
        bool e0 = false;
        vk_to_scancode(vk, sc, e0);

        InterceptionKeyStroke k{};
        k.code = sc;
        k.state = INTERCEPTION_KEY_UP;
        if (e0) k.state |= INTERCEPTION_KEY_E0;

        interception_send(ctx_, dev_, reinterpret_cast<InterceptionStroke*>(&k), 1);
    }

private:
    // 键名转虚拟键码
    static WORD key_to_vk(std::string key) {
        for (auto& ch : key)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        if (key == "left")  return VK_LEFT;
        if (key == "right") return VK_RIGHT;
        if (key == "up")    return VK_UP;
        if (key == "down")  return VK_DOWN;

        if (key == "space") return VK_SPACE;
        if (key == "shift") return VK_SHIFT;
        if (key == "ctrl")  return VK_CONTROL;
        if (key == "alt")   return VK_MENU;

        if (key == "tab")   return VK_TAB;
        if (key == "enter") return VK_RETURN;
        if (key == "esc")   return VK_ESCAPE;

        if (key == "f1")  return VK_F1;
        if (key == "f2")  return VK_F2;
        if (key == "f3")  return VK_F3;
        if (key == "f4")  return VK_F4;
        if (key == "f5")  return VK_F5;
        if (key == "f6")  return VK_F6;
        if (key == "f7")  return VK_F7;
        if (key == "f8")  return VK_F8;
        if (key == "f9")  return VK_F9;
        if (key == "f10") return VK_F10;
        if (key == "f11") return VK_F11;
        if (key == "f12") return VK_F12;

        // 单个字符: a-z / 0-9
        if (key.size() == 1) {
            unsigned char c = static_cast<unsigned char>(key[0]);
            if (std::isdigit(c)) return static_cast<WORD>(c);
            if (std::isalpha(c)) return static_cast<WORD>(std::toupper(c));
        }

        return 0;
    }

    // VK 转扫描码
    //这个函数就是将软件层的虚拟键码转换为硬件层的原始扫描码。
    static void vk_to_scancode(WORD vk, unsigned short& sc, bool& isE0) {
        UINT sc_ex = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC_EX);
        sc = static_cast<unsigned short>(sc_ex & 0xFF);
        isE0 = ((sc_ex & 0xFF00) == 0xE000);
    }

private:
    InterceptionContext ctx_{nullptr};
    InterceptionDevice  dev_{0};
};

} // namespace interception

