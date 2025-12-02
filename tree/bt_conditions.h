
#pragma once
#include "bt_core.h"
#include <cmath>

#include "../core/core.h"
#include "../core/common.h"
// ===================== 条件：是否在小地图上出现怪物=====================
class HasDot : public BTCondition {
public:
    HasDot() : BTCondition("HasDot") {}

    bool evaluate(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        return !bot->monster_minmap.empty();
    }
};

// ===================== 条件：屏幕上是否有怪物血条 =====================
class HasHpBar : public BTCondition {
public:
    HasHpBar() : BTCondition(" HasHpBar") {}

    bool evaluate(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        return bot->screen_monster_player.size()>1;
    }
};

// ===================== 条件：小地图上怪物是否居中=====================
class DotCenteredStable : public BTCondition {
public:
    DotCenteredStable() : BTCondition("DotCenteredStable") {}

    bool evaluate(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        auto& [a, b] = bot->closet_monster_minmap;
        return b<8;
    }
};

// ===================== 条件：小地图上没有怪物点 =====================
class NoDot : public BTCondition {
public:
    NoDot() : BTCondition("NoDot") {}

    bool evaluate(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        return bot->monster_minmap.empty();
    }
};
// ===================== 条件：是否需要复活 =====================
class needLife : public BTCondition {
public:
    needLife() : BTCondition("needLife") {}

    bool evaluate(BTBlackboard& bb) override {
        auto* bot = bb.get_bot();
        auto [postion,score,_]=Common::find_pattern_sqdiff(bot->img_frame,bot->bool_is_life);
        bot->re_life=postion;
        return score<0.2;

    }
};


