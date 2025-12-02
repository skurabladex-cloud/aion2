#include "core.h"
#include "common.h"
#include "logger.h"
#include <windows.h>
#include <vector>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <chrono>
#include"../input/InputApplier.h"
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

int main()
{
    try {
        // 初始化核心系统
        core bot;
        bot.start();

        // 启动主循环（在单独线程中）
        std::thread loop_thread([&bot]() {
            bot.loop();
        });

        LOG_INFO("[main] Bot started. Press Ctrl+C to stop.");

        // 等待用户中断
        loop_thread.join();

        // 清理
        bot.stop();
        LOG_INFO("[main] Bot stopped.");
    }
    catch (const std::exception& e) {
        LOG_ERROR("[main] Fatal error: {}", e.what());
        return 1;
    }

    // interception::InterceptionManager& a= interception::manager() ;
    // while (1)
    // {
    //      a.press_key("2");
    //     std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // }




    return 0;
}
