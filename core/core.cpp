//
// Created by Administrator on 2025/11/29.
//

#include "core.h"
#include "../input/GameWindowCapturer.h"
#include "common.h"
#include "../input/InputApplier.h"
#include "logger.h"
#include "../tree/bt_core.h"  // 包含 build_main_bt_tree 的声明
#include <limits>
////////////////////////////////////
///


core::core()
{
    cfg_=Common::LoadJson("../config/config_default.json");

}


std::vector< cv::Point2f> core::get_minmap_loction_monster(cv::Mat& bgr)
{
        std::vector< cv::Point2f > monster;

        cv::Point top_left(1650 ,100);
        cv::Point bottom_right(1898,264);


        cv::Rect roi_rect(top_left, bottom_right);

        cv::Mat roi = bgr(roi_rect).clone();
        cv::Point2f midpoint  (roi.cols/2.0,roi.rows/2.0);




        cv::Mat gray_img;//灰度图
        cv::cvtColor(roi, gray_img, cv::COLOR_BGR2GRAY);


        // 2️⃣ 生成掩码：检测纯白像素（白色像素=255，其余=0）
        cv::Mat mask_white;
        cv::inRange(  gray_img , 255, 255, mask_white);//mask_white取值0:255

        // 竖向连接：用“竖线核”
        cv::Mat kV = cv::getStructuringElement(cv::MORPH_RECT, {1, 2});
        cv::morphologyEx(mask_white, mask_white, cv::MORPH_CLOSE, kV, {-1,-1}, 1);

        // cv::Mat connected = mask.clone();
        //
        // // 竖向连接：用“竖线核”
        // cv::Mat kV = cv::getStructuringElement(cv::MORPH_RECT, {1, 7});
        // cv::morphologyEx(connected, connected, cv::MORPH_CLOSE, kV, {-1,-1}, 1);
        //
        // // 横向连接（如需要）：
        // // cv::Mat kH = cv::getStructuringElement(cv::MORPH_RECT, {7, 1});
        // // cv::morphologyEx(connected, connected, cv::MORPH_CLOSE, kH, {-1,-1}, 1);
        // 调参：
        //
        // {1,7} 里的 7 越大，能跨越的“断口”越大（但也更容易误连）


        // 3) 连通域
        cv::Mat labels, stats, centroids;
        int n = cv::connectedComponentsWithStats(mask_white, labels, stats, centroids, 8, CV_32S);
        // ====== 4) 遍历连通域，计算外接圆 ======
        for (int i = 1; i < n; ++i) { // 0 是背景
            int area = stats.at<int>(i, cv::CC_STAT_AREA);
            if (area < 10) continue;              // 过滤小噪点：你可以调 10/20/50

            // 连通域的外接矩形（在 ROI 内的坐标）
            int x = stats.at<int>(i, cv::CC_STAT_LEFT);//
            int y = stats.at<int>(i, cv::CC_STAT_TOP);
            int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
            int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);




            // 2) 去竖条：细长竖向（高远大于宽）
           if (std::abs(h-w)>2) continue;

            // 3) （可选）去横条：细长横向


            // 只在这个小框里取像素点，速度快很多
            cv::Rect box(x, y, w, h);
            cv::Mat lbl_roi = labels(box);

            std::vector<cv::Point2f> pts;
            pts.reserve(area);

            for (int yy = 0; yy < lbl_roi.rows; ++yy) {
                const int* row = lbl_roi.ptr<int>(yy);
                for (int xx = 0; xx < lbl_roi.cols; ++xx) {
                    if (row[xx] == i) {
                        // 注意：点坐标要加回 box 的偏移
                        pts.emplace_back((float)(xx + box.x), (float)(yy + box.y));
                    }
                }
            }

            if (pts.size() < 5) continue; // 点太少没意义

            cv::Point2f center_roi;//圆心
            float radius;//半径
            cv::minEnclosingCircle(pts, center_roi, radius);

            // ROI -> 全图坐标
            cv::Point2f center_global = center_roi + cv::Point2f((float)roi_rect.x, (float)roi_rect.y);
            monster.push_back(center_global);


            std::cout << "label=" << i
                      << " area=" << area
                      << " center_roi=(" << center_roi.x << "," << center_roi.y << ")"
                      << " center_global=(" << center_global.x << "," << center_global.y << ")"
                      << " r=" << radius << "\n";
         // 可视化（在 ROI 上画）
            cv::circle(img_frame_debug, center_global, (int)std::round(radius), cv::Scalar(0,255,0), 2);
            cv::circle(img_frame_debug, center_global, 2, cv::Scalar(0,0,255), -1);
        }
    return  monster;

}
//get_player_location_by_party_red_bar(): 通过识别队伍血条来估算角色位置，返回角色坐标和血条位置。
// ======================= Party red bar =======================
std::vector<cv::Rect>
    core::get_player_and_monster_location_by_party_red_bar(cv::Mat & bgr)
{

        cv::Mat img_hsv;
        img_hsv=bgr.clone();
        cv::cvtColor(img_hsv, img_hsv, cv::COLOR_BGR2HSV);
        //
        std::vector<cv::Rect> contours2;

        cv::Mat mask = cv::Mat::zeros( bgr.rows,  bgr.cols, CV_8U);
        cv::Mat mask_red;

        cv::inRange(img_hsv, cv::Scalar(170,90,235), cv::Scalar(180,210,245), mask_red);

        cv::bitwise_or(mask, mask_red, mask);//按位或，算出所有颜色的掩码
        cv::inRange(img_hsv, cv::Scalar(38,208,130), cv::Scalar(47,214,140), mask_red);
        cv::bitwise_or(mask, mask_red, mask_red);
        //cv::GaussianBlur(mask_red, mask_red, cv::Size(5, 5), 0);
    //形态学运算
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        cv::morphologyEx(mask_red, mask_red, cv::MORPH_OPEN, kernel);
        // cv::inRange(img_hsv, cv::Scalar(86,60,240), cv::Scalar(86,604,240), mask_red);
        // cv::bitwise_or(mask, mask_red, mask);//按位或，算
        //cv::Mat kV = cv::getStructuringElement(cv::MORPH_RECT, {1, 3});
        // cv::Mat mask_white;
        // cv::morphologyEx(mask, mask_white, cv::MORPH_CLOSE, kV, {-1,-1}, 1);


        //找外接矩形
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask_red, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);


        for (auto& c : contours)
        {
            cv::Rect r = cv::boundingRect(c);

            contours2.emplace_back(r);
        }

        std::sort( contours2.begin(),  contours2.end(),[](const cv::Rect& a, const cv::Rect& b) {
                  return a.height< b.height;   // 上->下
              });
        for (auto& c : contours2)
        {
            cv::rectangle(img_frame_debug, c, cv::Scalar(255,255,255), -1);

        }
        return contours2;
}

std::tuple<cv::Point2f, double> core::findClosestmonster(const std::vector<cv::Point2f>& contours)
{
    cv::Point2f top_left(1650, 100);
    cv::Point2f bottom_right(1898, 264);
    cv::Point2f midpoint = (top_left + bottom_right) / 2.0f;

    if (contours.empty()) {
        // 没有怪物时返回中心点和最大距离
        return std::make_tuple(midpoint, std::numeric_limits<double>::max());
    }

    double min_distance = std::numeric_limits<double>::max();
    cv::Point2f position = midpoint;  // 默认为中心点

    for (const auto& c : contours)
    {
        double distance = cv::norm(c - midpoint);

        if (distance < min_distance) {
            min_distance = distance;
            position = c;
        }
    }

    return std::make_tuple(position, min_distance);
}
////////////////////////复活


// ======================= Lifecycle =======================
void core::start() {
    // 初始化窗口捕获器
    capture_ = std::make_unique<GameWindowCapturer>(cfg_);

    // 初始化输入执行器
    input_ = std::make_unique<bot_input::InputApplier>();

    // 初始化行为树
    root_ = build_main_bt_tree();
    bb_.bot = this;  // 设置黑板中的 bot 指针

    // 初始化技能配置
    keys = cfg_["buff_skill"]["keys"].get<std::vector<std::string>>();
    cds = cfg_["buff_skill"]["cooldown"].get<std::vector<double>>();
    action_cd = cfg_["buff_skill"]["action_cooldown"].get<double>();
    auto buff_keys = cfg_["buff_skill"]["keys"];
    t_last_buff_cast_.resize(buff_keys.size(), 0.0);
    t_last_skill_ = 0.0;
    is_terminated_ = false;
    bool_is_life =cv::imread("../msvc/life.png", cv::IMREAD_COLOR);
}

int core::run_once()
{
    if (!capture_ || !root_) {
        return -1;  // 未初始化
    }

    cv::Mat bgr = capture_->getFrame();
    if (bgr.empty()) {
        return -1;
    }



    img_frame = bgr;

    img_frame_debug = img_frame.clone();
   // cv::imwrite("E:/photo/18.png",  img_frame);



    // 更新感知数据
    monster_minmap = get_minmap_loction_monster(bgr);
    screen_monster_player = get_player_and_monster_location_by_party_red_bar(bgr);
    closet_monster_minmap = findClosestmonster(monster_minmap);


    // 执行行为树
    root_->tick(bb_);

    return 1;
}
// ======================= Background loop =======================
void core::loop() {
    using namespace std::chrono_literals;

    while (!is_terminated_) {
        try {
            // 每帧更新图像、玩家位置、怪物、小地图等
            int r = run_once();
            if (r < 0) {
                // 获取帧失败，稍等再试
                std::this_thread::sleep_for(50ms);
                continue;
            }

           std::this_thread::sleep_for(5ms);
        }
        catch (const std::exception& e) {
            LOG_ERROR("[loop] Exception: {}", e.what());
            std::this_thread::sleep_for(50ms);
        }
    }
}

void core::stop() {
    is_terminated_ = true;
    if (capture_) {
        capture_->stop();
    }
    if (input_) {
        input_->stop();  // 停止输入线程
        input_->release_all();
    }
    // InterceptionManager 会在析构时自动 shutdown
}



