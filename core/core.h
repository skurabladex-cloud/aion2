//
// Created by Administrator on 2025/11/29.
//

#ifndef AILON2_CORE_H
#define AILON2_CORE_H
#include <vector>
#include <atomic>
#include <thread>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include "../input/InputApplier.h"
#include "../input/GameWindowCapturer.h"
#include "../tree/bt_core.h"
class core
{
public:
    /////s
    core();
    std::vector< cv::Point2f>  get_minmap_loction_monster_white(cv::Mat& bgr);//怪物在小地图上的坐标
    std::vector< cv::Point2f> get_minmap_loction_monster_red_yellow(cv::Mat& bgr);

    std::vector<cv::Rect>
        get_player_and_monster_location_by_party_red_bar(cv::Mat & bgr);//怪物和玩家在屏幕上的坐标


    std::tuple<cv::Point2f, double> findClosestmonster(const std::vector<cv::Point2f>& contours);//小地图上最接近中间的位置

    int     run_once();//单次循环
    void    start();
    void    loop();
    void    stop();  // 停止循环
    bool    has_Objects() ;//检查是否需要商品

    cv::Mat img_frame;
    cv::Mat img_frame_debug;
    cv::Mat bool_is_life;
    cv::Point re_life;//是否需要复活坐标
    cv::Mat img_F;


    nlohmann::json cfg_;//窗口标题、捕获区域、FPS限制等参数
    std::vector<cv::Point2f> monster_minmap;//怪物在小地图上的坐标
    std::vector<cv::Rect> screen_monster_player ;//怪物和玩家在屏幕上的坐标
    std::tuple<cv::Point2f, double> closet_monster_minmap;//小地图上最接近中间的位置


    std::unique_ptr<GameWindowCapturer> capture_;
    std::unique_ptr< bot_input::InputApplier> input_;

    std::shared_ptr<BTNode> root_;
    BTBlackboard bb_;
    std::atomic<bool> is_terminated_{false};
    int monster_nums_last_frame;

    std::vector<std::string>  keys;//技能数组
    std::vector<double> cds;//技能冷却
    std::vector<double> t_last_buff_cast_;
    double action_cd;//技能动作间隔
    double t_last_skill_;
    size_t last_skill_index_{0};  // 上次释放的技能索引，用于轮换
    //SPSC_DoubleBuffer<bot_input:: InputIntent3D> buffer_;//指令双双缓冲区


    std::atomic<int> Debounce_{0};









};


#endif //AILON2_CORE_H