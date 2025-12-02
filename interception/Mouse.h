#pragma once
// Interception 鼠标驱动封装


#include <interception.h>

namespace interception {

class Mouse {
public:
    Mouse() : ctx_(nullptr), dev_(0) {}

    bool init() {
        ctx_ = interception_create_context();
        if (!ctx_) return false;
        dev_ = INTERCEPTION_MOUSE(0);
        return true;
    }

    void shutdown() {
        if (ctx_) {
            interception_destroy_context(ctx_);
            ctx_ = nullptr;
        }
    }

    // 相对移动
    void move_rel(int dx, int dy) {
        if (!ctx_) return;
        InterceptionMouseStroke m{};
        m.x = dx;
        m.y = dy;
        m.rolling = 0;
        m.state = 0;
        m.flags = INTERCEPTION_MOUSE_MOVE_RELATIVE;
        interception_send(ctx_, dev_, reinterpret_cast<InterceptionStroke*>(&m), 1);
    }

    // 绝对移动（屏幕坐标，0,0 在左上角）
    void move_abs(int pixel_x, int pixel_y) {
        if (!ctx_) return;

        // 动态获取分辨率，适应不同屏幕
        int screen_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int screen_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        // 边界检查
        pixel_x = std::clamp(pixel_x, 0, screen_width - 1);
        pixel_y = std::clamp(pixel_y, 0, screen_height - 1);

        InterceptionMouseStroke m{};
        m.x = (pixel_x * 65535) / (screen_width - 1);
        m.y = (pixel_y * 65535) / (screen_height - 1);
        m.rolling = 0;
        m.state = 0;
        m.flags = INTERCEPTION_MOUSE_MOVE_ABSOLUTE | INTERCEPTION_MOUSE_VIRTUAL_DESKTOP;
        interception_send(ctx_, dev_, reinterpret_cast<InterceptionStroke*>(&m), 1);
    }

    // 左键按下/释放
    void left_down()  { button_state(INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN); }
    void left_up()    { button_state(INTERCEPTION_MOUSE_LEFT_BUTTON_UP); }

    // 右键按下/释放
    void right_down() { button_state(INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN); }
    void right_up()   { button_state(INTERCEPTION_MOUSE_RIGHT_BUTTON_UP); }

    // 中键按下/释放
    void middle_down(){ button_state(INTERCEPTION_MOUSE_MIDDLE_BUTTON_DOWN); }
    void middle_up()  { button_state(INTERCEPTION_MOUSE_MIDDLE_BUTTON_UP); }

    // 滚轮
    void wheel(int delta) {
        if (!ctx_) return;
        InterceptionMouseStroke m{};
        m.x = 0;
        m.y = 0;
        m.rolling = static_cast<short>(delta);
        m.state = 0;
        m.flags = INTERCEPTION_MOUSE_WHEEL;
        interception_send(ctx_, dev_, reinterpret_cast<InterceptionStroke*>(&m), 1);
    }

private:
    void button_state(unsigned short st) {
        if (!ctx_) return;
        InterceptionMouseStroke m{};
        m.x = 0;
        m.y = 0;
        m.rolling = 0;
        m.state = st;
        m.flags = 0;
        interception_send(ctx_, dev_, reinterpret_cast<InterceptionStroke*>(&m), 1);
    }

private:
    InterceptionContext ctx_{nullptr};
    InterceptionDevice  dev_{0};
};

} // namespace interception

