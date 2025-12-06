#pragma once
#include <opencv2/opencv.hpp>
#include <windows.h>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>
#include "../core/buffer.h"
class GameWindowCapturer {
public:
    GameWindowCapturer(const nlohmann::json& cfg, const std::string& test_image_name = "");//E:/photo/16dsasda
    ~GameWindowCapturer();

    cv::Mat getFrame() ;     // 获取当前帧（线程安全）主线程调用
    void stop();              // 停止捕获线程
    const std::string& window_title() const { return window_title_; }

private:
    void captureLoop();       // 捕获主循环
    cv::Mat CaptureVirtualScreenBGRA();
    void limitFPS();          // 限制 FPS


private:
    nlohmann::json cfg_;//窗口标题、捕获区域、FPS限制等参数
    std::mutex frame_mutex_;//线程安全的状态标志，用于控制捕获线程的退出
    std::atomic<bool> is_terminated_;//1个线程安全的终止标志，用于控制后台线程的优雅退出。
    std::atomic<int> fps_;//实时记录当前的捕获帧率（Frames Per Second）
    double fps_limit_;
    double t_last_run_;//记录上一次捕获的时间戳，用于计算帧间隔

    std::thread capture_thread_;//后台捕获线程的句柄
    cv::Mat frame_;//存储当前捕获到的图像帧（OpenCV矩阵格式）
    std::string window_title_;//用：要捕获的游戏窗口标题
    SPSC_DoubleBuffer<cv::Mat> buffer_;//图片双缓冲区
    std::atomic<bool> new_data_available_{false};;

    HWND hwnd_ = nullptr;
};
