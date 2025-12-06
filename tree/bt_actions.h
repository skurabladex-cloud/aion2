
#pragma once
#include <cmath>
#include <ctime>
#include <iostream>
#include "bt_core.h"
#include "../core/core.h"
#include "../input/InputApplier.h"
using namespace std::chrono;
#include <chrono>
#include"../core/logger.h"
#include<random>




// struct InputIntent3D {
//     int move_fb = 0;   // forward/back:  +1=W, -1=S
//     int move_lr = 0;   // left/right:    +1=D, -1=A
//     bool sprint = false;   // Shift
//     bool crouch = false;   // Ctrl
//     bool jump   = false;   // Space
//     bool attack = false;   // 例如 "j"
//     int look_dx = 0;       // yaw
//     int look_dy = 0;       // pitch
//     bool lmb_click = false;
//     bool rmb_click = false;
//     bool lmb_hold = false;
//     bool rmb_hold = false;
//     std::vector<std::string> tap_keys;
//     std::vector<std::string> hold_keys;
//     bool stop_all = false;
// };
// ===================== 行为：正常打怪  =====================
class Attack : public BTAction {
public:
     Attack() : BTAction(" Attack") {}

    BTStatus do_tick(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        //鼠标左击，放技能
         bot_input::InputIntent3D it;
         it.lmb_click =true;

         using namespace std::chrono;

         const double now_sec =
             duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();

         for (size_t i = 0; i < bot->keys.size(); ++i) {
             const double dt_buff = now_sec - bot->t_last_buff_cast_[i]; // 秒
             const double dt_skill = now_sec - bot->t_last_skill_;       // 秒

             if (dt_buff >= bot->cds[i] && dt_skill > bot->action_cd) {
                 it.tap_keys.push_back(bot->keys[i]);
                 LOG_INFO("[Buff] Press buff skill key: {}", bot->keys[i]);

                 bot->t_last_buff_cast_[i] = now_sec;
                 bot->t_last_skill_ = now_sec;
                 break;
             }
         }
         bool f=bot->has_Objects();
         if (f) it.tap_keys.push_back("f");

         bot->input_->apply(it);
        BTStatus status = BTStatus::RUNNING;

         bot->Debounce_.store(0);
        return status;
    }
};

// ===================== 行为：接近怪物） =====================
class ApproachStep: public BTAction {

public:
    ApproachStep() : BTAction("ApproachStep") {}

    void on_enter(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        // bot->rune_solver_->reset();
        // is_attack_ = true;//是否允许攻击怪物
    }

    BTStatus do_tick(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        bot_input::InputIntent3D it;
        it.lmb_click =true;
        using namespace std::chrono;

        const double now_sec =
            duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();

        for (size_t i = 0; i < bot->keys.size(); ++i) {
            const double dt_buff = now_sec - bot->t_last_buff_cast_[i]; // 秒
            const double dt_skill = now_sec - bot->t_last_skill_;       // 秒

            if (dt_buff >= bot->cds[i] && dt_skill > bot->action_cd) {
                it.tap_keys.push_back(bot->keys[i]);
                LOG_INFO("[Buff] Press buff skill key: {}", bot->keys[i]);

                bot->t_last_buff_cast_[i] = now_sec;
                bot->t_last_skill_ = now_sec;
                break;
            }
        }

        bot->input_->apply(it);
        BTStatus status = BTStatus::RUNNING;

        bot->Debounce_.store(0);
        return status;
    }
};

// ===================== 行为：旋转视角找到挂物 =====================
class ScanRotate : public BTAction {
    std::time_t t_enter_ = 0;

public:
    ScanRotate() : BTAction("ScanRotate") {}


    BTStatus do_tick(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        bot_input::InputIntent3D it;
        //it.lmb_click =true;

        static thread_local std::mt19937 rng{ std::random_device{}() };

        auto rand_int=[](int a, int b) {
            std::uniform_int_distribution<int> d(a, b);
            return d(rng);
        };

        it.look_dx = rand_int(0, 8);
        it.look_dy = rand_int(-1, 1);
        bot->input_->apply(it);
        BTStatus status = BTStatus::RUNNING;

        return status;

    }
};

// ===================== 行为：按住tab找到怪物=====================
class tabdown : public BTAction {


public:
    tabdown() : BTAction("tabdown") {}


    BTStatus do_tick(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        bot_input::InputIntent3D it;

        it.tap_keys.push_back("tab");
        bot->input_->apply(it);
        bot->Debounce_.store(0);
        return BTStatus::RUNNING;



    }
};

// ===================== 行为：移动 =====================
class WanderStep : public BTAction {
    std::time_t t_enter_ = 0;

public:
    WanderStep() : BTAction("WanderStep") {}

    void on_enter(BTBlackboard& bb) override {
        move_fb=0; // 前后移动
        move_lr=0; // 左右移动
    }

    void on_exit(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        bot_input::InputIntent3D it;
        //it.lmb_click =true;
        bot->input_->apply(it);



    }

    BTStatus do_tick(BTBlackboard& bb) override {
        on_enter(bb);
        auto* bot = bb.get_bot();
        bot_input::InputIntent3D it;
        //it.lmb_click =true;


        auto getRandomInZeroMinusOneOne = []() {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, 2);
            return dis(gen) - 1;
        };




        it.move_fb = getRandomInZeroMinusOneOne();  // 前后移动
        it.move_lr = getRandomInZeroMinusOneOne();  // 左右移动
        move_fb= it.move_fb;
        move_lr= it.move_lr;
        bot->input_->apply(it);

        std::this_thread::sleep_for(std::chrono::milliseconds(500 + (rand() % 1001)));


        on_exit(bb);
        BTStatus status = BTStatus::RUNNING;

        return status;

    }
    int  move_fb; // 前后移动
    int move_lr; // 左右移动

};

// ===================== 行为：复活=====================
class clickLife : public BTAction {


public:
   clickLife() : BTAction("clickLife") {}



    BTStatus do_tick(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        bot_input::InputIntent3D it;

        //it.lmb_click =true;
       it.click_x = bot->re_life.x+100;
       it.click_y = bot->re_life.y+90;
       it.click_left = true;



        bot->input_->apply(it);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        BTStatus status = BTStatus::SUCCESS;

       bot->Debounce_.store(0);
        return status;

    }
};
// ===================== 行为：到怪物点=====================
class clickPath : public BTAction {


public:
   clickPath() : BTAction("clickPathwd") {}
    void on_enter(BTBlackboard& bb) {
       auto* bot = bb.get_bot();
       bot->Debounce_.fetch_add(1);

   }



    BTStatus do_tick(BTBlackboard& bb) override {
       auto* bot = bb.get_bot();

       bot_input::InputIntent3D it;
       //it.lmb_click =true;
       on_enter(bb) ;
       bot->input_->apply(it);
       if (bot->Debounce_.load()<3) {

           return BTStatus::RUNNING;
       }

       interception::InterceptionManager& t=interception::manager();

       {
            t.press_key("m");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
       }



       {
           bot_input::InputIntent3D it;


            //it.lmb_click =true;
            // it.click_x = 985;
            // it.click_y =568;
            // it.click_left = false;;
            t.mouse_click_at(986,568,false);



            //bot->input_->apply(it);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
       }
       {
            // bot_input::InputIntent3D it;
            // it.click_x = 1883;
            // it.click_y =42;
            // it.click_left = true;;
            t.mouse_click_at(1883,42,false);
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
       }

        BTStatus status = BTStatus::SUCCESS;
       LOG_INFO("");

        on_exit(bb);
        return status;

    }
    virtual void on_exit(BTBlackboard& bb) {

       auto* bot = bb.get_bot();
       bot->Debounce_.store(0);
   }
};



