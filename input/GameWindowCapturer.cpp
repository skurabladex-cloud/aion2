#include "GameWindowCapturer.h"
#include <chrono>
#include <thread>
#include "../core/common.h" // 包含 get_game_window_title_by_token, resize_window, load_image 等声明
#include "../core/logger.h"
using namespace Common;


GameWindowCapturer::GameWindowCapturer(const nlohmann::json& cfg, const std::string& test_image_name)
    : cfg_(cfg), is_terminated_(false), fps_(0), t_last_run_(0.0)
{
    //const std::string& test_image_name 这个参数的作用是——在不连接真实游戏窗口的情况下，用一张测试图片来模拟捕获画面
    fps_limit_ = cfg_["system"]["fps_limit_window_capturor"].get<double>();
    if (!test_image_name.empty()) {
        frame_ = load_image( test_image_name);
        LOG_INFO("[GameWindowCapturor] Test image mode, capture thread disabled.");
        return;
    }

    std::wstring  game_window_title_= get_game_window_title_by_token(utf8_to_wstring_winapi(cfg_["game_window"]["title"]));
    window_title_ = wstring_to_utf8_winapi(game_window_title_);
    if (window_title_.empty()) {
        throw std::runtime_error("[GameWindowCapturor] Unable to find window title containing: " + std::string(cfg_["game_window"]["title"]));
    }

    LOG_INFO("[GameWindowCapturor] Found game window title: {}", window_title_);
//     HWND FindWindowA(
//     LPCSTR lpClassName,    // 窗口类名（可nullptr）
//     LPCSTR lpWindowName    // 窗口标题
// );
    hwnd_ = FindWindowW(nullptr, game_window_title_.c_str());

    if (!hwnd_) throw std::runtime_error("[GameWindowCapturor] Window handle not found.");

    resize_window(window_title_, 1920, 1080);
    activate_game_window(window_title_);
    LOG_INFO("[GameWindowCapturor] Window resized.");


    // 启动捕获线程

    // 标准语法：传递成员函数指针 + 对象指针
    //std::thread thread_obj(&Class::MemberFunction, object_ptr, arg1, arg2...);
    capture_thread_ = std::thread(&GameWindowCapturer::captureLoop, this);
    LOG_INFO("[GameWindowCapturor] Init done.");
}

GameWindowCapturer::~GameWindowCapturer() {
    stop();
}

void GameWindowCapturer::captureLoop() {//
    while (!is_terminated_) {
        cv::Mat& new_frame = buffer_.write_buffer(); // 写 back
        new_frame= CaptureVirtualScreenBGRA();           // 填数据（可写多项）
                      // 原子发布：front/back 交换
        // cv::Mat new_frame = CaptureVirtualScreenBGRA();//获取三通道图片
        cv::cvtColor(new_frame, new_frame, cv::COLOR_BGRA2BGR);
        frame_ = new_frame;
        buffer_.publish();
        new_data_available_.store(true, std::memory_order_release);
        // if (new_frame.empty()) continue;
        //
        // {
        //     std::lock_guard<std::mutex> lock(frame_mutex_);//枷锁，主线程要不停去读取这个照片
        //     frame_ = new_frame;
        //    // Common::screenshot(new_frame, "img_frame");
        //
        //
        // }

       limitFPS();
    }
    LOG_INFO("[GameWindowCapturor] Capture thread exited.");
}
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

// 强化版 PrintWindow（不再支持 Unity / DirectX 子窗口，仅使用句柄抓取）

cv::Mat GameWindowCapturer::CaptureVirtualScreenBGRA()
{
    // 虚拟屏幕（多显示器）范围
    const int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HDC hScreenDC = GetDC(nullptr);
    if (!hScreenDC) throw std::runtime_error("GetDC(nullptr) failed");

    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    if (!hMemDC) { ReleaseDC(nullptr, hScreenDC); throw std::runtime_error("CreateCompatibleDC failed"); }

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
    if (!hBitmap) {
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreenDC);
        throw std::runtime_error("CreateCompatibleBitmap failed");
    }

    HGDIOBJ oldObj = SelectObject(hMemDC, hBitmap);

    // 从屏幕拷贝到内存位图
    if (!BitBlt(hMemDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY | CAPTUREBLT)) {
        SelectObject(hMemDC, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreenDC);
        throw std::runtime_error("BitBlt failed");
    }

    // 读取像素：32bpp BGRA
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // 负数=top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> buffer(static_cast<size_t>(w) * h * 4);

    if (!GetDIBits(hMemDC, hBitmap, 0, h, buffer.data(), &bmi, DIB_RGB_COLORS)) {
        SelectObject(hMemDC, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreenDC);
        throw std::runtime_error("GetDIBits failed");
    }

    // 清理 GDI
    SelectObject(hMemDC, oldObj);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(nullptr, hScreenDC);

    // 转 Mat（注意：cv::Mat 引用外部buffer会悬空，所以 clone 一份）
    cv::Mat bgra(h, w, CV_8UC4, buffer.data());
    return bgra.clone();
}


// 上一帧时间 t_last_run_
//        ↓
// [ 计算 now = 当前时间 ]
//        ↓
// frame_duration = now - t_last_run_
//        ↓
// 若 < 目标帧间隔 → sleep(差值)
//        ↓
// current = 当前时间
// fps_ = 1 / (current - t_last_run_)
// t_last_run_ = current

void GameWindowCapturer::limitFPS() {//执行第二次才有意义

//     steady_clock::now()	当前时间点（稳定单调）
// .time_since_epoch()	距离起点的时间差
// std::chrono::duration<double>(...)	转换为以秒为单位的时间长度
// .count()	得到 double 数值（秒数）


    double target_duration = 1.0 / fps_limit_;//fps_limit_ = 60，则 target_duration ≈ 0.01667 秒（16.67ms）
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    // auto t = std::chrono::steady_clock::now();
    // auto dur = t.time_since_epoch();   // 某个纳秒数，比如 12,345,678,900ns
    // auto sec = std::chrono::duration<double>(dur); // 换成 12.3456789 s
    // double start = sec.count();        // start = 12.3456789

    double frame_duration = now - t_last_run_;

    if (frame_duration < target_duration)
        std::this_thread::sleep_for(std::chrono::duration<double>(target_duration - frame_duration));

    double current = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    fps_ = static_cast<int>(1.0 / (current - t_last_run_));
    t_last_run_ = current;
}

cv::Mat GameWindowCapturer::getFrame() {//主线程要不停去截取frame
    // std::lock_guard<std::mutex> lock(frame_mutex_);
    // if (frame_.empty()) return {};
    // return frame_.clone();
    if (!new_data_available_.load(std::memory_order_acquire))
        return cv::Mat();

        const cv::Mat r = buffer_.read_buffer(); // 读 front（快照）
        if (r.empty()) throw std::runtime_error("Empty buffer");
             return r;
     new_data_available_.store(false, std::memory_order_release);

}

void GameWindowCapturer::stop() {//主线程调用
//     joinable() 返回 true 表示：
//
// 线程已启动且正在运行
//
// 或者线程已运行完成但尚未被 join/detach
//
// 线程对象关联着一个有效的执行线程
    if (!is_terminated_) {
        is_terminated_ = true;
        if (capture_thread_.joinable()) capture_thread_.join();
        LOG_INFO("[GameWindowCapturor] Terminated.");
    }
}
