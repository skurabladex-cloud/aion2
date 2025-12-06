//
// Created by Administrator on 2025/12/3.
//

#ifndef AILON2_BUFFER_H
#define AILON2_BUFFER_H

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdint>

template <class T>
class SPSC_DoubleBuffer {
public:
    T& write_buffer() noexcept {
        return buffers_[1u - front_.load(std::memory_order_relaxed)];
    }

    void publish() noexcept {
        auto cur = front_.load(std::memory_order_relaxed);
        front_.store(1u - cur, std::memory_order_release);
    }

    const T read_buffer() noexcept {
        auto idx = front_.load(std::memory_order_acquire);
        return buffers_[idx];
    }

private:
    T buffers_[2]{};
    std::atomic<uint32_t> front_{0};
};

// ===== 示例数据 =====
// struct Frame {
//     int value = 0;
// };
//
// int main() {
//     SPSC_DoubleBuffer<Frame> db;
//     std::atomic<bool> stop{false};
//
//     std::thread writer([&]{
//         for (int i = 1; i <= 20; ++i) {
//             Frame& w = db.write_buffer(); // 写 back
//             w.value = i;                  // 填数据（可写多项）
//             db.publish();                 // 原子发布：front/back 交换
//             std::this_thread::sleep_for(std::chrono::milliseconds(50));
//         }
//         stop.store(true, std::memory_order_release);
//     });
//
//     std::thread reader([&]{
//         int last = -1;
//         while (!stop.load(std::memory_order_acquire)) {
//             const Frame& r = db.read_buffer(); // 读 front（快照）
//             if (r.value != last) {
//                 std::cout << "Read value = " << r.value << "\n";
//                 last = r.value;
//             }
//             std::this_thread::sleep_for(std::chrono::milliseconds(80));
//         }
//     });
//
//     writer.join();
//     reader.join();
//     return 0;
// }

#endif //AILON2_BUFFER_H