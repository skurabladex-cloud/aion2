//
// Created by Administrator on 2025/11/27.
//

#include "common.h"

#include "logger.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <limits>
#include <algorithm>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX  // 避免 windows.h 定义的 min/max 宏污染 std 命名空间
#endif
#include <windows.h>
#endif

// #define LOG_DEBUG(...) Logger::GetInstance().Debug(__VA_ARGS__)
// #define LOG_INFO(...) Logger::GetInstance().Info(__VA_ARGS__ )
// #define LOG_WARNING(...) Logger::GetInstance().Warn( __VA_ARGS__)
// #define LOG_ERROR(...) Logger::GetInstance().Error( __VA_ARGS__)

using namespace std;
using namespace Common;
// ---------------- 判断环境 ----------------
bool Common::IsWindows() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}
//////////////////////////
bool Common::IsMac() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

// ---------------- JSON 操作 ----------------
json Common::LoadJson(const std::string& path) {
    ifstream f(path);
    if (!f.is_open())
        throw runtime_error("Cannot open file: " + path);

    json data;
    f >> data;
    LOG_INFO("Loaded JSON: {}", path);
    return data;
}

void Common::SaveJson(const json& data, const std::string& path) {
    ofstream f(path);
    if (!f.is_open())
        throw runtime_error("Cannot write file: " + path);

    f << data.dump(4);
    LOG_INFO("Saved JSON: {}", path);
}

json Common::DiffJson(const json& base, const json& current) {//比较两个json文件的不同
    json diff;
    for (auto& [key, val] : current.items()) {
        if (!base.contains(key)) diff[key] = val;
        else if (val.is_object() && base[key].is_object()) {
            json sub = DiffJson(base[key], val);
            if (!sub.empty()) diff[key] = sub;
        } else if (val != base[key]) diff[key] = val;
    }
    return diff;
}

void Common::OverrideJson(json& base, const json& override) {//覆盖
    for (auto& [k, v] : override.items()) {
        if (base.contains(k) && base[k].is_object() && v.is_object())
            OverrideJson(base[k], v);
        else
            base[k] = v;
    }
}

// ---------------- 图像处理 ----------------
// opencv2/imgcodecs.hpp
// enum ImreadModes {
//     IMREAD_UNCHANGED  = -1,
//     IMREAD_GRAYSCALE  = 0,
//     IMREAD_COLOR      = 1,
//     IMREAD_ANYDEPTH   = 2,
//     IMREAD_ANYCOLOR   = 4,
//     ...
// };
//Y=0.114×B+0.587×G+0.299×R
cv::Mat Common::load_image(const std::string& path,int mode) {
    cv::Mat img = cv::imread(path, mode);//三通道读取
    if (img.empty()) {
        LOG_ERROR("Image not found: {}", path);
        throw runtime_error("Failed to load image");//没有catch程序自动调用abort中断
    }
    LOG_INFO("Loaded image: {}", path);
    return img;
}
//保存图片
void Common::screenshot(const cv::Mat& img, const std::string& suffix){
    namespace fs = std::filesystem;
    fs::create_directories("screenshot");//创建目录

    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm buf{};
#ifdef _WIN32
    localtime_s(&buf, &t);
#else
    localtime_r(&t, &buf);
#endif

    char name[128];
    strftime(name, sizeof(name), "%Y-%m-%d_%H-%M-%S", &buf);
    string path = "screenshot/" + string(name) + "_" + suffix + ".png";//screenshot/2025-10-31_15-45-30_screenshot.png
    //filename = f"screenshot/{timestamp}_{suffix}.png"
    cv::imwrite(path, img);
    LOG_INFO("[Screenshot] Saved to {}", path);
}
//若图像尺寸小于目标 (h,w)，在四周对称补边。

// template<typename _Tp> class Rect_
// {
// public:
//     _Tp x;      // 左上角 X 坐标
//     _Tp y;      // 左上角 Y 坐标
//     _Tp width;  // 矩形宽度
//     _Tp height; // 矩形高度
//
//     // 构造函数
//     Rect_();
//     Rect_(_Tp _x, _Tp _y, _Tp _width, _Tp _height);
//     Rect_(const Point_<_Tp>& org, const Size_<_Tp>& sz);
//     ...
// };
//：绘制矩形及标签文字。

// template<typename _Tp> class Size_
// {
// public:
//     _Tp width;   // 宽度
//     _Tp height;  // 高度
//
//     // 构造函数
//     Size_();
//     Size_(_Tp _width, _Tp _height);
//     Size_(const Size_& sz);
//     ...
// };
//画边框
void Common::draw_rectangle(cv::Mat& img, cv::Point topLeft, cv::Size size, cv::Scalar color, const std::string& text,
    int thickness ,double text_scale ) {
    cv::Rect rect(topLeft, size);
    cv::rectangle(img, rect, color, 2);
    cv::putText(img, text, {topLeft.x, topLeft.y - 5},
                cv::FONT_HERSHEY_SIMPLEX, text_scale, color, thickness);
    // | thickness 值   | 含义          | 效果      |
    // | ------------- | ----------- | ------- |
    // | **正数 n**      | 画出 n 像素宽的边框 | 🔲 只有边框 |
    // | **0 或 -1**    | 填充整个矩形区域    | 🟥 填充矩形 |
    // | **省略时默认 = 1** | 边框线宽为 1 像素  | 🔲 细边框  |

}
//图像尺寸小于目标 (h,w)，在四周对称补边。
cv::Mat Common::pad_to_size(const cv::Mat& img, const cv::Size& target_size, int pad_value) {
    // 获取图像的高度和宽度
    int h_img = img.rows;
    int w_img = img.cols;

    // 获取目标图像的高度和宽度
    int h_target = target_size.height;
    int w_target = target_size.width;

    // 计算需要填充的高度和宽度
    int pad_h = std::max(0, h_target - h_img);
    int pad_w = std::max(0, w_target - w_img);

    cv::Mat padded_img = img;//浅拷贝，下面不执行就返回自身就好了

    // 如果需要填充
    if (pad_h > 0 || pad_w > 0) {
        // 在图像的四个边缘添加填充
        cv::copyMakeBorder(//padded_img会分配新的内存，这里的内存是data=new。。。。
            img,
            padded_img,
            pad_h / 2,                // 上边填充
            pad_h - pad_h / 2,        // 下边填充
            pad_w / 2,                // 左边填充
            pad_w - pad_w / 2,        // 右边填充
            cv::BORDER_CONSTANT,      // 填充类型：常数填充
            cv::Scalar(pad_value)     // 填充颜色（默认为0，黑色）
        );
    }

    return padded_img;//
}
// ---------------- 模板匹配与几何计算 ----------------

// 主函数：模板匹配 (SQDIFF_NORMED)
std::tuple<cv::Point, double, bool> Common::find_pattern_sqdiff(
    const cv::Mat& img_input,//大图
    const cv::Mat& pattern_input,//小图
    const cv::Point* last_result ,//点
    const cv::Mat& mask,//没设置通道数
    int local_search_radius,
    double global_threshold)
{
    // Step 1: 确保图像已填充
    cv::Mat img = pad_to_size(img_input, pattern_input.size());//确保大图不小于匹配模板
    int h = pattern_input.rows;//h
    int w = pattern_input.cols;//w

    // Step 2: 如果提供了上次匹配位置，先进行局部搜索
    if (last_result && global_threshold > 0.0) {
        int lx = last_result->x;
        int ly = last_result->y;
        //扩大收缩范围，左上右下
        int x0 = std::max(0, lx - local_search_radius);
        int y0 = std::max(0, ly - local_search_radius);
        int x1 = std::min(img.cols, lx + local_search_radius + w);
        int y1 = std::min(img.rows, ly + local_search_radius + h);

        cv::Rect roi(x0, y0, x1 - x0, y1 - y0);//cv::Rect(50, 50, 100, 100)50, 50,起点坐标
        cv::Mat img_roi = img(roi);//不是复制，而是引用

        if (img_roi.rows >= h && img_roi.cols >= w) {//rows是高度,cols是宽度,一般来说是不会越界的，防止上一次匹配错误造成的越界
            cv::Mat res;
            cv::matchTemplate(img_roi, pattern_input, res, cv::TM_SQDIFF_NORMED, mask);
            //mask掩码图像（可选）👉 必须与模板 尺寸完全一致
            //(img.cols - templ.cols + 1, img.rows - templ.rows + 1)=res
//             把一个小矩阵（模板）在大矩阵（图像）上滑动，
//             在每一个位置计算它们“重叠区域”的相似度或差异度。
            //mask —— 模板掩码（mask），控制模板中哪些像素参与匹配计算。屏蔽背景可以


//    void cv::matchTemplate(
//     InputArray image,     // 原图（搜索区域）
//     InputArray templ,     // 模板图（要查找的内容）
//     OutputArray result,   // 匹配结果矩阵
//     int method,           // 匹配方法（比如 TM_SQDIFF_NORMED）
//     InputArray mask = noArray() // 掩码（可选）
            // // );在大图 image 中滑动小图 templ，
            // 计算每个位置的相似度或差异度，并把结果存在 result 里。

            double min_val, max_val;
            cv::Point min_loc, max_loc;
            cv::minMaxLoc(res, &min_val, &max_val, &min_loc, &max_loc);

            if (min_val < global_threshold) {//小图查找精准度够高就不用全局查
                return {cv::Point(x0 + min_loc.x, y0 + min_loc.y),min_val, true };
            }
        }
    }

    // Step 3: 全局匹配回退
    cv::Mat res;
    cv::matchTemplate(img, pattern_input, res, cv::TM_SQDIFF_NORMED, mask);
    //mask用于小图,
    // 替换 NaN、无穷值，防止数值错误
    cv::patchNaNs(res, 1.0);//防止全是nan
    // 处理正无穷和负无穷（OpenCV的patchNaNs只处理NaN）
    // 注意：OpenCV的Mat比较可能需要特殊处理，这里使用阈值方法更安全
    double max_val_check;
    cv::minMaxLoc(res, nullptr, &max_val_check);//如果结果矩阵的最大值是 NaN / inf / 超过 1.0
    if (!std::isfinite(max_val_check) || max_val_check > 1.0) {
        cv::Mat mask_inf = (res > 1.0);
        res.setTo(1.0, mask_inf);
    }
//     生成一个与 res 同大小的布尔矩阵：
//
// 每个位置比较：如果 res(y,x) > 1.0 → mask_inf(y,x) = 255；
//
// 否则 mask_inf(y,x) = 0。

    double min_val, max_val;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(res, &min_val, &max_val, &min_loc, &max_loc);

    return { min_loc, min_val, false };;
}


//对模板图中等于某颜色的像素生成“透明背景”遮罩（白色=保留区域，黑被忽略）
cv::Mat Common::get_mask(const cv::Mat& img, const cv::Scalar& ignore_pixel_color)
{//ignore_pixel_color背景色
    CV_Assert(img.channels() == 3); // 确保是三通道彩色图像

    // Step 1: 生成颜色匹配掩码
    cv::Mat mask;
    cv::inRange(img,
                ignore_pixel_color,  // lower bound
                ignore_pixel_color,  // upper bound（相等匹配）
                mask);

    // Step 2: 反转掩码（使被忽略区域=0，其余=255）
    cv::bitwise_not(mask, mask);

    return mask;
}
// 算两个边界框的交并比（Intersection over Union, IoU）。
double Common::get_iou(const cv::Rect& box1, const cv::Rect& box2) {

    int inter_x1 = std::max(box1.x, box2.x);
    int inter_y1 = std::max(box1.y, box2.y);
    int inter_x2 = std::min(box1.x + box1.width,  box2.x + box2.width);
    int inter_y2 = std::min(box1.y + box1.height, box2.y + box2.height);

    // 如果没有重叠
    if (inter_x2 <= inter_x1 || inter_y2 <= inter_y1)
        return 0.0;

    // 计算交集面积
    int inter_area = (inter_x2 - inter_x1) * (inter_y2 - inter_y1);

    // 计算各自面积
    int area1 = box1.width * box1.height;
    int area2 = box2.width * box2.height;

    // 并集面积
    int union_area = area1 + area2 - inter_area;

    // 返回 IoU
    return static_cast<double>(inter_area) / static_cast<double>(union_area);
}

// 头文件声明为 IoU，这里提供与声明一致的封装，内部复用 get_iou
double Common::IoU(const cv::Rect& a, const cv::Rect& b) {
    return Common::get_iou(a, b);
}
// 图像检测中（比如识别怪物、人物、人脸等），算法通常会输出多个重叠的检测框（bounding boxes），这些框往往都指向同一个目标。
//

// 为了去掉这些重复框，只保留最“可信”或“最核心”的那个，我们要做的就是 NM
std::vector<cv::Rect> Common::NMS(const vector<cv::Rect>& boxes, double iouThreshold) {
    //对怪物检测结果做 NMS。输入每项包含
    vector<cv::Rect> result;
    vector<bool> removed(boxes.size(), false);
    for (size_t i = 0; i < boxes.size(); ++i) {
        if (removed[i]) continue;
        result.push_back(boxes[i]);

        for (size_t j = i + 1; j < boxes.size(); ++j) {
            if (get_iou(boxes[i], boxes[j]) > iouThreshold)
                removed[j] = true;
        }
    }
    return result;
}



std::vector<std::tuple<int, cv::Point, double, cv::Size>>
Common::nms_matches(const std::vector<std::tuple<int, cv::Point, double, cv::Size>>& matches, double iou_thresh) {
    std::vector<std::tuple<int, cv::Point, double, cv::Size>> filtered = matches;

    size_t i = 0;
    while (i < filtered.size()) {
        size_t j = i + 1;
        while (j < filtered.size()) {
            const auto& mi = filtered[i];
            const auto& mj = filtered[j];
            // std::get<0>从元组中取得元素
            const cv::Point& li = std::get<1>(mi);
            const cv::Point& lj = std::get<1>(mj);
            const cv::Size&  si = std::get<3>(mi);
            const cv::Size&  sj = std::get<3>(mj);
            // 计算两个匹配框的矩形区域
            cv::Rect box_i(li.x, li.y, si.width, si.height);
            cv::Rect box_j(lj.x, lj.y, sj.width, sj.height);

            if (Common::get_iou(box_i, box_j) > iou_thresh) {
                double score_i = std::get<2>(mi);
                double score_j = std::get<2>(mj);

                if (score_i > score_j) {
                    filtered.erase(filtered.begin() + j);
                    continue;
                } else {
                    filtered.erase(filtered.begin() + i);
                    if (i > 0) --i;
                    break;
                }
            }
            ++j;
        }
        ++i;
    }

    return filtered;
}


// ----------------颜色与色彩空间 ----------------
//将标准 HSV（H:0-360, S/V:0-100）转换为 OpenCV HSV（H:0-179, S/V:0-255）。
cv::Vec3b Common::to_opencv_hsv(const std::array<double, 3>& color_hsv) {
// | 通道              | 标准HSV范围 | OpenCV内部范围 |
// | --------------   | -------    | ---------- |
// | Hue（色调）        | 0–360     | 0–179      |
// | Saturation（饱和度| 0–100      |  0–255      |
// | Value（亮度）       0–100      | 0–255      |

    double h = color_hsv[0];
    double s = color_hsv[1];
    double v = color_hsv[2];

    // 转换比例：
    // H: 0–360 → 0–179
    // S/V: 0–100 → 0–255
    int h_opencv = static_cast<int>(std::round(h / 360.0 * 179.0));//td::round() 会将浮点数四舍五入为最接近的整数值（返回浮点型）。
    int s_opencv = static_cast<int>(std::round(s / 100.0 * 255.0));
    int v_opencv = static_cast<int>(std::round(v / 100.0 * 255.0));

    // 限制取值范围
    h_opencv = std::clamp(h_opencv, 0, 179);
    s_opencv = std::clamp(s_opencv, 0, 255);
    v_opencv = std::clamp(v_opencv, 0, 255);

    return cv::Vec3b(static_cast<uchar>(h_opencv),
                     static_cast<uchar>(s_opencv),
                     static_cast<uchar>(v_opencv));
}

std::array<double, 3> Common::to_standard_hsv(const std::array<int, 3>& color_hsv) {
    // OpenCV HSV → 标准 HSV
    int h = color_hsv[0];
    int s = color_hsv[1];
    int v = color_hsv[2];

    double h_std = static_cast<double>(h) / 179.0 * 360.0;
    double s_std = static_cast<double>(s) / 255.0 * 100.0;
    double v_std = static_cast<double>(v) / 255.0 * 100.0;

    // 限制范围防止数值误差导致越界
    h_std = std::clamp(h_std, 0.0, 360.0);
    s_std = std::clamp(s_std, 0.0, 100.0);
    v_std = std::clamp(v_std, 0.0, 100.0);

    return {h_std, s_std, v_std};
}

// ---------------- 小地图识别与玩家他人定位 ----------------
// 外层大白框（最外圈）满足所有条件（上下左右全白）；
//
// 它内部当然也有“非白内容”（地图）；
//
// 所以算法在这一步：
std::optional<cv::Rect> Common::get_minimap_loc_size(const cv::Mat& img_frame) {
    CV_Assert(img_frame.channels() == 3);

    // 1️⃣ 纯白色阈值范围
    cv::Scalar white(255, 255, 255);

    // 2️⃣ 生成掩码：检测纯白像素（白色像素=255，其余=0）
    cv::Mat mask_white;
    cv::inRange(img_frame, white, white, mask_white);//mask_white取值0:255


    // 3️⃣ 连通域分析（connected components）
    cv::Mat labels, stats, centroids;
    // cv::Mat label;
    // std::cout << label.empty();       // true
    // std::cout << label.type();        // 0（未定义）
    // std::cout << label.channels();    // 结果是 0
    // std::cout << label.depth();       // 结果是 0
    //mask_white 255 255 0 0
    //           255 0   0 255
    //           0   0   0 255

    int num_labels = cv::connectedComponentsWithStats(mask_white, labels, stats, centroids, 8);
    //lables 1 1 0 0
    //       1 0 0 2
    //       0 0 0 2
// | label | LEFT | TOP | WIDTH | HEIGHT | AREA |           stats         |
// | :---: | :--: | :-: | :---: | :----: | :--: | ------------------ |
// |   0   |   0  |  0  |   6   |    6   |  28  | ← 背景像素总数（0像素区域的数量） |
// |   1   |   1  |  1  |   2   |    2   |   4  | ← 左上白块 (2×2)       |
// |   2   |   4  |  3  |   2   |    2   |   4  | ← 右下白块 (2×2)       |

    // | label | centerX | centerY |        centroids     |
    // | :---: | :-----: | :-----: | ----------- |
    // |   0   |   2.5   |   2.5   | 背景中心（整个图中点） |
    // |   1   |   1.5   |   1.5   | 左上白块中心      |
    // |   2   |   4.5   |   3.5   | 右下白块中心      |

    // 4️⃣ 遍历每个连通区域（跳过背景 label=0）
//     template<typename _Tp>
// _Tp& cv::Mat::at(int row, int col);

    for (int i = 1; i < num_labels; ++i) {
        int x0 = stats.at<int>(i, cv::CC_STAT_LEFT);//连通区域的边界，左顶点
        int y0 = stats.at<int>(i, cv::CC_STAT_TOP);//右顶点
        int rw = stats.at<int>(i, cv::CC_STAT_WIDTH);//宽度
        int rh = stats.at<int>(i, cv::CC_STAT_HEIGHT);//高度
        int area = stats.at<int>(i, cv::CC_STAT_AREA);//面积

        // 过滤太小的噪声区域
        if (rw < 100 || rh < 100)
            continue;

        int x1 = x0 + rw - 1;
        int y1 = y0 + rh - 1;
        //这里的检测是检测能不能构成一个边框连通区域
        // 5️⃣ 检查上、下边框是否为纯白线
        cv::Mat top_row = img_frame(cv::Rect(x0, y0, rw, 1));
        cv::Mat bottom_row = img_frame(cv::Rect(x0, y1, rw, 1));
        cv::Mat top_mask, bottom_mask;
        cv::inRange(top_row, white, white, top_mask);
        cv::inRange(bottom_row, white, white, bottom_mask);
        if (cv::countNonZero(top_mask) != rw || cv::countNonZero(bottom_mask) != rw)
            continue;

        // 6️⃣ 检查左、右边框是否为纯白线
        cv::Mat left_col = img_frame(cv::Rect(x0, y0, 1, rh));
        cv::Mat right_col = img_frame(cv::Rect(x1, y0, 1, rh));
        cv::Mat left_mask, right_mask;
        cv::inRange(left_col, white, white, left_mask);
        cv::inRange(right_col, white, white, right_mask);
        if (cv::countNonZero(left_mask) != rh || cv::countNonZero(right_mask) != rh)
            continue;

        // 7️⃣ 创建一个mask，找出内部非白色区域
        cv::Mat roi = img_frame(cv::Rect(x0, y0, rw, rh));//引用
        cv::Mat mask_minimap;
        cv::inRange(roi, white, white, mask_minimap);
        cv::bitwise_not(mask_minimap, mask_minimap); // 白→0，非白→255
        //防止全白,通常只有一个白块（整张小地图内容）
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask_minimap, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
//         cv::findContours 就是把二值图像中的白色区域边界提取出来，让你能够：
//
// 定位游戏中的各种物体
//
// 分析物体的形状和大小
//
// 区分玩家、怪物、NPC等不同实体
    //     void cv::findContours(
    // InputOutputArray image,           // 输入图像（必须是单通道二值图像）
    // OutputArrayOfArrays contours,     // 输出：轮廓集合
    // OutputArray hierarchy,            // 可选：轮廓层级结构
    // int mode,                         // 检测模式（RETR_*）
    // int method,                       // 近似方法（CHAIN_*）
    // Point offset = Point()            // 坐标偏移
    // );

//         原图坐标：
            // (0,0)
            //   ├──> 裁剪区域从 (x0=200, y0=100) 开始
            //         ├──> 内部轮廓矩形 bbox=(10,20,50,60)
            //         ├──> 转回原图 = (210,120,50,60)

//         mask → 输入上述二值图
//
// cv::RETR_EXTERNAL → 只找最外层轮廓（忽略内洞）
//
// cv::CHAIN_APPROX_SIMPLE → 轮廓压缩，只保留拐点坐标
        //默认只取一个轮廓，最外层的大轮廓
        if (contours.empty())//这个防止找到的是一个大白块
            continue;

        // 8️⃣ 计算最小外接矩形（minimap 的真实内容区域）
        cv::Rect bbox = cv::boundingRect(contours[0]);

        // 偏移回原图坐标
        bbox.x += x0;
        bbox.y += y0;

        return bbox;
    }

    // 没找到 minimap
    return std::nullopt;

//下面的是计算全部的小白框的外接矩形
    // cv::findContours(mask_minimap, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    //
    // if (contours.empty())
    //     continue;
    //
    // // 计算所有轮廓的联合外接矩形
    // cv::Rect bbox_all;
    // for (size_t i = 0; i < contours.size(); ++i) {
    //     cv::Rect r = cv::boundingRect(contours[i]);
    //     if (i == 0)
    //         bbox_all = r;
    //     else
    //         bbox_all |= r;  // “或等于”运算符表示矩形并集
    // }
    //
    // // 偏移回原图
    // bbox_all.x += x0;
    // bbox_all.y += y0;
    //
    // return bbox_all;
}

//通过精确颜色匹配找玩家点（默认 BGR 为 (136,255,255)），取所有匹配像素的几何中心。
//小地图上找到玩家坐标，只能找具体的某一个人
std::optional<cv::Point> Common::get_player_location_on_minimap(
    const cv::Mat& img_minimap,
    const cv::Scalar& minimap_player_color ) // 默认玩家颜色
{
    CV_Assert(img_minimap.channels() == 3);

    // 1️⃣ 生成掩码：只保留等于玩家颜色的像素
    cv::Mat mask;
    cv::inRange(img_minimap,
                minimap_player_color,  // 下界
                minimap_player_color,  // 上界（相等匹配）
                mask);

    // 2️⃣ 查找非零点（即匹配颜色的像素坐标）
    std::vector<cv::Point> coords;
    cv::findNonZero(mask, coords);//这里必须是单通道,遍历mask矩阵

    if (coords.empty() || coords.size() < 4) {
        // 少于4个像素 → 可能是噪声，不算有效
        return std::nullopt;
    }

    // 3️⃣ 计算匹配点的平均坐标
    // class Scalar {
    // public:
    //     double val[4];
    //     ...
    // };
    cv::Scalar mean = cv::mean(coords);//cv::Scalar cv::mean(InputArray src, InputArray mask = noArray());
                                            //Mat(N×1, CV_32SC2)
    cv::Point loc_player_minimap(
        static_cast<int>(std::round(mean[0])),                  //std::round四舍五入
        static_cast<int>(std::round(mean[1]))
    );
    // // 转换为 N×1 CV_32SC2 矩阵
    // cv::Mat coords(player_positions.size(), 1, CV_32SC2);
    // for (int i = 0; i < player_positions.size(); ++i) {
    //     coords.at<cv::Vec2i>(i, 0) = cv::Vec2i(
    //         player_positions[i].x,
    //         player_positions[i].y
    //     );
    // }

    return loc_player_minimap;
}




/**
 * @brief 检测小地图中的红色点（代表其他玩家位置）
 *
 * @param img_minimap 小地图图像 (cv::Mat, BGR格式)
 * @param red_bgr 红色目标值 (B, G, R)，默认(0, 0, 255)
 * @return std::vector<cv::Point> 返回检测到的红点坐标列表
 */
std::vector<cv::Point> Common::get_all_other_player_locations_on_minimap(
    const cv::Mat& img_minimap,
    const cv::Scalar& red_bgr
) {
    // 容差列表：从小到大依次尝试（类似 Python 中的 [10,20,30,40]）
    std::vector<int> tolerances = {10, 20, 30, 40};

    // 遍历容差值，从严格到宽松依次尝试检测红色点
    for (int tolerance : tolerances) {
        // 计算下界与上界（防止越界 0~255）
        cv::Scalar lower_bgr(
            std::max(0, (int)red_bgr[0] - tolerance),  // B下限
            std::max(0, (int)red_bgr[1] - tolerance),  // G下限
            std::max(0, (int)red_bgr[2] - tolerance)   // R下限
        );
        cv::Scalar upper_bgr(
            std::min(255, (int)red_bgr[0] + tolerance),  // B上限
            std::min(255, (int)red_bgr[1] + tolerance),  // G上限
            std::min(255, (int)red_bgr[2] + tolerance)   // R上限
        );

        // 生成掩膜图像：白色区域表示在颜色范围内的像素
        cv::Mat mask;
        cv::inRange(img_minimap, lower_bgr, upper_bgr, mask);//

        // 查找所有非零点坐标
        std::vector<cv::Point> coords;
        cv::findNonZero(mask, coords);

        // 如果找到了足够多的红色像素（至少3个）
        if (!coords.empty() && coords.size() >= 3) {
            LOG_DEBUG("Found {} red pixels with tolerance {}", coords.size(), tolerance);
            LOG_DEBUG("Color range: [{},{},{}] to [{},{},{}]",
                lower_bgr[0], lower_bgr[1], lower_bgr[2],
                upper_bgr[0], upper_bgr[1], upper_bgr[2]);

            return coords; // 直接返回坐标点
        }
    }

    // 如果所有容差都失败
    LOG_DEBUG("Red dot detection failed with all tolerances: [10, 20, 30, 40]");
    return {}; // 空结果
}
//作用：调试小地图颜色分布，帮助确定他人点颜色阈值。
// 比较器：保证 cv::Vec3b 可作为 std::map 的键（移到函数内部或作为匿名lambda）
void Common::debug_minimap_colors(const cv::Mat& img_minimap,
                          const cv::Scalar& target_color)
{
    CV_Assert(img_minimap.channels() == 3);

    // 1️⃣ 保存原始小地图
    cv::imwrite("debug_minimap_original.png", img_minimap);

    int h = img_minimap.rows;
    int w = img_minimap.cols;
    // 使用自定义比较器的map
    auto vec3b_less = [](const cv::Vec3b& a, const cv::Vec3b& b) {
        if (a[0] != b[0]) return a[0] < b[0];
        if (a[1] != b[1]) return a[1] < b[1];
        return a[2] < b[2];
    };
    std::map<cv::Vec3b, int, decltype(vec3b_less)> color_count(vec3b_less);

    // 2️⃣ 遍历整张小地图，每隔2个像素采样一次，提高效率
    for (int y = 0; y < h; y += 2) {
        for (int x = 0; x < w; x += 2) {
            cv::Vec3b color = img_minimap.at<cv::Vec3b>(y, x);
            color_count[color]++;
        }
    }

    // 3️⃣ 将 map 转为 vector 并按出现次数排序
    std::vector<std::pair<cv::Vec3b, int>> sorted_colors(color_count.begin(), color_count.end());
    std::sort(sorted_colors.begin(), sorted_colors.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::cout << "=== Minimap Color Analysis ===\n";
    std::cout << "Target color (BGR): (" << target_color[0] << ", "
              << target_color[1] << ", " << target_color[2] << ")\n";
    std::cout << "Top 10 most common colors:\n";

    int printed = 0;
    for (const auto& [color, count] : sorted_colors) {
        if (color == cv::Vec3b(0, 0, 0) || color == cv::Vec3b(255, 255, 255))
            continue; // 跳过纯黑/纯白

        std::cout << "  " << printed + 1 << ". BGR("
                  << (int)color[0] << ", "
                  << (int)color[1] << ", "
                  << (int)color[2] << "): "
                  << count << " pixels\n";

        // 判断是否接近目标颜色
        int diff = std::abs(color[0] - target_color[0]) +
                   std::abs(color[1] - target_color[1]) +
                   std::abs(color[2] - target_color[2]);
        if (diff < 50)
            std::cout << "    *** Close to target color! Difference: " << diff << " ***\n";

        if (++printed >= 10)
            break;
    }

    // 4️⃣ 生成不同容差下的掩码调试图
    for (int tolerance : {10, 20, 30, 40, 50}) {
        cv::Scalar lower_bgr(
            std::max(0, (int)target_color[0] - tolerance),
            std::max(0, (int)target_color[1] - tolerance),
            std::max(0, (int)target_color[2] - tolerance));
        cv::Scalar upper_bgr(
            std::min(255, (int)target_color[0] + tolerance),
            std::min(255, (int)target_color[1] + tolerance),
            std::min(255, (int)target_color[2] + tolerance));

        cv::Mat mask;
        cv::inRange(img_minimap, lower_bgr, upper_bgr, mask);

        std::vector<cv::Point> coords;
        cv::findNonZero(mask, coords);

        std::cout << "Tolerance " << tolerance << ": Found "
                  << coords.size() << " pixels\n";

        std::string filename =
            "debug_red_detection_tolerance_" + std::to_string(tolerance) + ".png";
        cv::imwrite(filename, mask);
    }

    std::cout << "Color analysis complete.\n";
}
// ---------------- 进度条识别 ----------------
double Common::get_bar_percent(const cv::Mat& img)
{
    CV_Assert(!img.empty());
    CV_Assert(img.channels() >= 3); // 确保是彩色条形图（HP/MP/EXP条）

    int h = img.rows;
    int w = img.cols;

    // 取中间那一行像素（与 Python 的 img[h//2, :] 等价）
    cv::Mat line = img.row(h / 2);//从图像中取出 第 h/2 行 的所有像素（即图像的中间一行）

    // 1️⃣ 获取左边界（跳过全白区域）
    int lb = 0;
    while (lb < w) {
        cv::Vec3b color = line.at<cv::Vec3b>(0, lb);
        if (!(color[0] >= 255 && color[1] >= 255 && color[2] >= 255))
            break;
        lb++;
    }
 //像素序列：[0,0,0][0,0,0][255,0,0][255,0,0][128,128,128][128,128,128][0,0,0]
 //            ↑       ↑       ↑         ↑         ↑           ↑         ↑
 //          黑边框  黑边框   红色填充   红色填充   灰色未填充   灰色未填充  黑边框
    // 2️⃣ 获取右边界（同样跳过全白区域）
    int rb = w - 1;
    while (rb > lb) {
        cv::Vec3b color = line.at<cv::Vec3b>(0, rb);
        if (!(color[0] >= 255 && color[1] >= 255 && color[2] >= 255))
            break;
        rb--;
    }

    // 3️⃣ 边界检查
    if (rb <= lb) return 0.0;

    // 4️⃣ 统计“未填充”像素数量（近似灰色的部分）
    int unfill_pixel_cnt = 0;
    int tolerance = 10;

    for (int i = lb; i <= rb; ++i) {
        cv::Vec3b color = line.at<cv::Vec3b>(0, i);
        int r = color[2];
        int g = color[1];
        int b = color[0];
        if (std::abs(r - g) <= tolerance &&
            std::abs(r - b) <= tolerance &&
            r > 0) {//防止黑色像素但是有点问题这里,因为边框大小已经算出来了，但是这里又弄黑色背景就会出现小问题
            unfill_pixel_cnt++;
            }
//     灰色 = R、G、B三个分量值非常接近
// // 容差范围通常为 10-30
    }

    // 5️⃣ 计算填充比例
    int total_width = rb - lb + 1;
    int fill_width = total_width - unfill_pixel_cnt;
    double fill_ratio = (total_width > 0) ? static_cast<double>(fill_width) / total_width : 0.0;

    return fill_ratio * 100.0; // 返回百分比（0~100）
}
// ----------------窗口与输入（跨平台差异） ----------------
//mac
#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>

struct WindowRegion {
    int left;
    int top;
    int width;
    int height;
};

WindowRegion get_window_region_mac(const std::string& window_title)
{
    CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID
    );

    if (!windowList) throw std::runtime_error("无法获取窗口列表");

    CFIndex count = CFArrayGetCount(windowList);
    for (CFIndex i = 0; i < count; i++) {
        CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

        CFStringRef titleRef = (CFStringRef)CFDictionaryGetValue(window, kCGWindowName);
        if (!titleRef) continue;

        char title[256];
        if (CFStringGetCString(titleRef, title, sizeof(title), kCFStringEncodingUTF8)) {
            if (window_title == title) {
                CFDictionaryRef boundsRef = (CFDictionaryRef)CFDictionaryGetValue(window, kCGWindowBounds);
                CGRect bounds;
                CGRectMakeWithDictionaryRepresentation(boundsRef, &bounds);

                CFRelease(windowList);
                return {
                    static_cast<int>(bounds.origin.x),
                    static_cast<int>(bounds.origin.y),
                    static_cast<int>(bounds.size.width),
                    static_cast<int>(bounds.size.height)
                };
            }
        }
    }

    CFRelease(windowList);
    throw std::runtime_error("未找到指定窗口: " + window_title);
}
#endif
//在指定游戏窗口内的相对坐标点击
void Common::click_in_game_window(const std::string& window_title, int x, int y)
{
#ifdef __APPLE__
    // 🔹 macOS Quartz 方式
    WindowRegion region = get_window_region_mac(window_title);

    // 修正坐标（Python 版本 coord/2 + 10）
    x = x / 2;
    y = y / 2 + 10;

    CGPoint click_point;
    click_point.x = region.left + x;
    click_point.y = region.top + y;

    CGEventRef click1 = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, click_point, kCGMouseButtonLeft);
    CGEventRef click2 = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp, click_point, kCGMouseButtonLeft);

    CGEventPost(kCGHIDEventTap, click1);
    CGEventPost(kCGHIDEventTap, click2);

    CFRelease(click1);
    CFRelease(click2);

    std::cout << "[click_in_game_window] click at (" << click_point.x << ", " << click_point.y << ")\n";

#elif _WIN32
    // 🔹 Windows 实现：使用 WinAPI
    HWND hwnd = FindWindowA(nullptr, window_title.c_str());
    if (!hwnd)
        throw std::runtime_error("未找到窗口: " + window_title);

    RECT rect;
    GetWindowRect(hwnd, &rect);

    int abs_x = rect.left + x;//x,y要点击的区域
    int abs_y = rect.top + y;

    // 模拟鼠标点击
    SetCursorPos(abs_x, abs_y);
    //将鼠标光标移动到屏幕坐标 (abs_x, abs_y)
    // 按下
    INPUT press = {};
    press.type = INPUT_MOUSE;
    press.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &press, sizeof(INPUT));

    Sleep(50);  // 按下持续50ms，更像真人点击

    // 释放
    INPUT release = {};
    release.type = INPUT_MOUSE;
    release.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &release, sizeof(INPUT));

    std::cout << "[click_in_game_window] click at (" << abs_x << ", " << abs_y << ")\n";
#endif
}

bool Common::activate_game_window(const std::string& window_title)
{
    // 查找窗口
    HWND hwnd = FindWindowW(NULL,  utf8_to_wstring_winapi(window_title).c_str());
    if (!hwnd) {
        std::cerr << "Cannot find window with title: " << window_title << std::endl;
        return false;
    }

    // 尝试恢复窗口（如果被最小化）
    ShowWindow(hwnd, SW_RESTORE);

    // 尝试置前台
    if (SetForegroundWindow(hwnd)) {
        std::cout << "[activate_game_window] Set game window to foreground" << std::endl;
        return true;
    }

    // 如果 SetForegroundWindow 失败，尝试备用方案
    std::cerr << "[activate_game_window] SetForegroundWindow failed, trying alternatives..." << std::endl;
    ShowWindow(hwnd, SW_SHOW);
    BringWindowToTop(hwnd);
    SetActiveWindow(hwnd);

    return true;
}


// 用全局静态变量保存匹配句柄（更安全，也可封装进结构体）
static HWND g_matched_hwnd = nullptr;

// 枚举回调函数
// typedef LONG_PTR LPARAM;      // 32位/64位自适应
// typedef _W64 long LONG_PTR;   // 总是32位或64位
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)//回调函数
{
    // 获取窗口标题
    wchar_t title[256];
    GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));

    // 转小写以方便匹配
    std::wstring titleStr = title;
    std::transform(titleStr.begin(), titleStr.end(), titleStr.begin(), ::towlower);

    // 获取传入的 token
    const std::wstring& token = *(const std::wstring*)lParam;
    std::wstring tokenLower = token;
    std::transform(tokenLower.begin(), tokenLower.end(), tokenLower.begin(), ::towlower);

    // 如果匹配
    if (titleStr.find(tokenLower) != std::wstring::npos)
    {
        std::wcout << L"[Matched] " << titleStr << std::endl;//wchar_t是双字

        // ✅ 保存句柄（不再用 SetLastError）
        g_matched_hwnd = hwnd;

        return FALSE; // 停止枚举
    }

    return TRUE; // 继续枚举
}

// 查找包含 token 的窗口标题,
std::wstring Common::get_game_window_title_by_token(const std::wstring& token)
{
    // 清理上一次结果
    g_matched_hwnd = nullptr;

    // 枚举所有顶层窗口
    EnumWindows(EnumWindowsProc, (LPARAM)&token);

    // ✅ 安全返回结果
    if (!g_matched_hwnd)
            return L""; // 没找到

    wchar_t title[256];
    GetWindowTextW(g_matched_hwnd, title, sizeof(title) / sizeof(wchar_t));
    return std::wstring(title);//返回完整的·窗口标题
}

//判断图像宽高比是否约等于 16:9（容差来自 cfg["game_window"]["ratio_tolerance"]）。
// cfg["game_window"]["ratio_tolerance"] 对应的配置参数


bool Common::is_img_16_to_9(const cv::Mat& img, const json& cfg)
{
    // 防御性检查
    if (img.empty()) {
        std::cerr << "Error: Image is empty.\n";
        return false;
    }

    int h = img.rows;
    int w = img.cols;

    double actual_ratio = static_cast<double>(w) / h;
    double target_ratio = 16.0 / 9.0;

    return std::abs(actual_ratio - target_ratio) <= cfg["game_window"]["ratio_tolerance"];
}


/*
// 日志打印函数，可替代 Python 的 logger.info

// 作用：将某窗口尺寸下的像素坐标归一化到标准尺寸 (693,1282)（用于跨分辨率复用坐标）。
//当前窗口 → 标准尺寸*/
cv::Point Common::normalize_pixel_coordinate(const cv::Point& coord, const cv::Size& window_size)
{
    // 当前窗口尺寸
    int h_win = window_size.height;
    int w_win = window_size.width;

    // 标准分辨率
    const int h_std = 693;
    const int w_std = 1282;

    // 如果窗口就是标准尺寸，无需缩放
    if (h_win == h_std && w_win == w_std) {
        LOG_INFO("Standard size, no normalization needed.");
        return coord;
    }

    // 计算缩放比例
    double scale_y = static_cast<double>(h_std) / h_win;
    double scale_x = static_cast<double>(w_std) / w_win;

    // 应用缩放
    int norm_x = static_cast<int>(std::round(coord.x * scale_x));
    int norm_y = static_cast<int>(std::round(coord.y * scale_y));

    LOG_INFO("Normalized coord({}, {}) → ({}, {})", coord.x, coord.y, norm_x, norm_y);

    return cv::Point(norm_x, norm_y);
}

// 头文件工具函数适配：NormalizePixelCoord((h,w), (h,w), (h_std,w_std))
std::pair<int,int> Common::NormalizePixelCoord(std::pair<int,int> coord,
                                              std::pair<int,int> windowSize,
                                              std::pair<int,int> standardSize) {
    // 输入：coord = (x, y)
    // 窗口：windowSize = (h_win, w_win)
    // 标准：standardSize = (h_std, w_std)
    const int x = coord.first;
    const int y = coord.second;
    const int h_win = windowSize.first;
    const int w_win = windowSize.second;
    const int h_std = standardSize.first;
    const int w_std = standardSize.second;

    if (h_win == 0 || w_win == 0) return {x, y};

    const double scale_y = static_cast<double>(h_std) / static_cast<double>(h_win);
    const double scale_x = static_cast<double>(w_std) / static_cast<double>(w_win);

    const int nx = static_cast<int>(std::round(x * scale_x));
    const int ny = static_cast<int>(std::round(y * scale_y));
    return {nx, ny};
}
#include <string>
#include <windows.h>

std::wstring Common::utf8_to_wstring_winapi(const std::string& str) {
    const int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len <= 1) return L"";

    std::wstring ws(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, ws.data(), len);
    return ws;
}


std::string Common::wstring_to_utf8_winapi(const std::wstring& wstr) {
    const int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return "";

    std::string str(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, str.data(), len, nullptr, nullptr);
    return str;
}

bool Common::resize_window(const std::string& window_title, int width, int height )
{

    return resize_window(utf8_to_wstring_winapi(window_title),  width,  height );
}

//Windows 下调整目标窗口尺寸（保留左上角位置）。
bool Common::resize_window(const std::wstring& window_title, int width, int height )
{
    // 1️⃣ 查找窗口句柄
    HWND hwnd = FindWindowW(nullptr, window_title.c_str());
    if (hwnd == nullptr)
    {
        std::wcout << L" 找不到窗口: " << window_title << std::endl;
        return false;
    }

    // 2️⃣ 获取当前窗口位置（保持左上角不变）
    // typedef struct tagRECT {
    //     LONG left;     // 矩形左上角X坐标
    //     LONG top;      // 矩形左上角Y坐标
    //     LONG right;    // 矩形右下角X坐标
    //     LONG bottom;   // 矩形右下角Y坐标
    // } RECT;
    RECT rect;
    if (!GetWindowRect(hwnd, &rect))
    {
        std::wcerr << L" 获取窗口位置失败。" << std::endl;
        return false;
    }

    int x = rect.left;
    int y = rect.top;

    // 3️⃣ 调整窗口大小
    BOOL success = MoveWindow(hwnd, x, y, width, height, TRUE);
    if (!success)
    {
        std::wcerr << L" 调整窗口大小失败。" << std::endl;
        return false;
    }

    std::wcout << L" 已将「" << window_title
               << L"调整为 " << width << L"x" << height << std::endl;

    return true;
}
// ---------------- 路线图层处理 ----------------
//在 minimap（img_map）上，找到与路线颜色（color_code）相同的像素 → 这些点是“杂点/干扰” → 从 img_route 里把这些位置删掉（置黑）
cv::Mat Common::mask_route_colors(
    const cv::Mat& img_map_input,
    const cv::Mat& img_route_input,
    const std::map<std::string, std::string>& color_code)
{
    // std::map<std::string, std::string> color_code = {
    //     {"255,0,0", "红色"},
    //     {"0,255,0", "绿色"},
    //     {"0,0,255", "蓝色"},
    //     {"128,128,128", "灰色"}
    // };颜色字典

    // 拷贝输入（以免修改原图）
    cv::Mat img_map = img_map_input.clone();
    cv::Mat img_route = img_route_input.clone();

    // 1️⃣ 解析颜色字典键为 BGR 三元组
    std::vector<cv::Vec3b> target_colors;
    for (const auto& kv : color_code) {
        const std::string& color_str = kv.first; // 比如 "255,0,0"
        int b, g, r;
        if (std::sscanf(color_str.c_str(), "%d,%d,%d", &b, &g, &r) == 3) {
            target_colors.emplace_back(cv::Vec3b(b, g, r));
        }
    }

    // 2️⃣ 检查尺寸是否一致，不一致则缩放
    if (img_map.size() != img_route.size()) {
        std::cout << "[mask_route_colors] Resizing img_map from ("
                  << img_map.cols << "x" << img_map.rows << ") to ("
                  << img_route.cols << "x" << img_route.rows << ")\n";
        cv::resize(img_map, img_map, img_route.size());
    }

    // 3️⃣ 创建 mask（单通道，初始为 0）
    cv::Mat mask = cv::Mat::zeros(img_map.rows, img_map.cols, CV_8U);

    // 4️⃣ 针对每种颜色生成掩码并叠加
    for (const auto& color : target_colors) {
        cv::Mat mask_color;
        cv::inRange(img_map, color, color, mask_color);
        cv::bitwise_or(mask, mask_color, mask);//按位或，算出所有颜色的掩码
    }

    // 5️⃣ 应用掩码：在 mask=255 的像素上，将 img_route 置为黑色
    img_route.setTo(cv::Scalar(0, 0, 0), mask);//在 mask 图像中值为非零（通常是255）的位置，把 img_route 的对应像素改成黑色 (0,0,0)。

    return img_route;
}









