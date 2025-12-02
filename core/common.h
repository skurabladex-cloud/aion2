
#pragma once
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <tuple>
#include <map>




namespace Common {
    using json = nlohmann::json;
    struct Match;

    // ---------- 系统判断 ----------
    bool IsWindows();
    bool IsMac();
    // ---------- 获取时间函数----------


    // ---------- 配置读写 ----------
    json LoadJson(const std::string& path);
    void SaveJson(const json& data, const std::string& path);
    json DiffJson(const json& base, const json& current);
    void OverrideJson(json& base, const json& override);

    // ---------- 图像处理 ----------
    cv::Mat load_image(const std::string& path,int mode=cv::IMREAD_COLOR);
    void screenshot(const cv::Mat& img, const std::string& suffix="screenshot");//默认参数只能在一处声明
    void draw_rectangle(cv::Mat& img, cv::Point topLeft, cv::Size size, cv::Scalar color, const std::string& text,
                        int thickness = 1, double text_scale = 0.6);
    cv::Mat pad_to_size(const cv::Mat& img, const cv::Size& target_size, int pad_value = 0);

    // ---------- 模板匹配与几何计算 ----------
    // 主函数：模板匹配 (SQDIFF_NORMED)
    std::tuple<cv::Point, double, bool> find_pattern_sqdiff(
                            const cv::Mat& img_input,//大图
                            const cv::Mat& pattern_input,//小图
                            const cv::Point* last_result = nullptr,//点
                            const cv::Mat& mask = cv::Mat(),//没设置通道数
                            int local_search_radius = 50,
                            double global_threshold = 0.4
                        );
    cv::Mat get_mask(const cv::Mat& img, const cv::Scalar& ignore_pixel_color);
    double get_iou(const cv::Rect& box1, const cv::Rect& box2);
    double IoU(const cv::Rect& a, const cv::Rect& b);
    std::vector<cv::Rect> NMS(const std::vector<cv::Rect>& boxes, double iouThreshold = 0.3);
    // ---------- Match结构体（用于nms_matches）----------
    // struct Match {
    //     int idx;
    //     cv::Point loc;
    //     double score;
    //     cv::Size shape;
    // };
    std::vector<std::tuple<int, cv::Point, double, cv::Size>>
    nms_matches(const std::vector<std::tuple<int, cv::Point, double, cv::Size>>& matches, double iou_thresh=0.0);

    // ---------- 颜色与色彩空间 ----------
    cv::Vec3b to_opencv_hsv(const std::array<double, 3>& color_hsv);
    std::array<double, 3> to_standard_hsv(const std::array<int, 3>& color_hsv);

    // ---------- 小地图与玩家识别 ----------
    std::optional<cv::Rect> get_minimap_loc_size(const cv::Mat& img_frame);
    std::optional<cv::Point> get_player_location_on_minimap(const cv::Mat& img_minimap,
                                                            const cv::Scalar& minimap_player_color= cv::Scalar(136, 255, 255));
    std::vector<cv::Point> get_all_other_player_locations_on_minimap(const cv::Mat& img_minimap,
                                                                     const cv::Scalar& red_bgr);
    void debug_minimap_colors(const cv::Mat& img_minimap, const cv::Scalar& target_color);

    // ---------- 进度条识别 ----------
    double get_bar_percent(const cv::Mat& img);

    // ---------- 工具函数 ----------
    bool is_img_16_to_9(const cv::Mat& img, const json& cfg);
    cv::Point normalize_pixel_coordinate(const cv::Point& coord, const cv::Size& window_size);
    std::pair<int,int> NormalizePixelCoord(std::pair<int,int> coord,
                                           std::pair<int,int> windowSize,
                                           std::pair<int,int> standardSize);

    // ---------- 窗口与输入 ----------
    void click_in_game_window(const std::string& window_title, int x, int y);
    bool activate_game_window(const std::string& window_title);
    std::wstring get_game_window_title_by_token(const std::wstring& token);
    bool resize_window(const std::wstring& window_title, int width = 1296, int height = 759);
    bool resize_window(const std::string& window_title, int width = 1296, int height = 759);
    std::string wstring_to_utf8_winapi(const std::wstring& wstr);
    std::wstring utf8_to_wstring_winapi(const std::string& str);

    // ---------- 路线图层处理 ----------
    cv::Mat mask_route_colors(const cv::Mat& img_map_input,
                              const cv::Mat& img_route_input,
                              const std::map<std::string, std::string>& color_code);





}

