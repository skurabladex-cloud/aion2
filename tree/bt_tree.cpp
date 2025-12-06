#include "bt_core.h"
#include "bt_conditions.h"
#include "bt_actions.h"

// 构建主行为树：Selector + 若干 Sequence/Action
std::shared_ptr<BTNode> build_main_bt_tree() {
    auto root = std::make_shared<BTSelector>("Main");
    // 0) 复活（判断死亡 + 复活）
    auto seq_Relife= std::make_shared<BTSequence>("Relif");
    seq_Relife->add_child(std::make_shared<needLife>());
    seq_Relife->add_child(std::make_shared<clickLife>());
    root->add_child(seq_Relife);


    // 1) 打怪（有怪物点 + 已居中 + 攻击）
    auto seq_solving = std::make_shared<BTSequence>("EngageAttack");
    seq_solving->add_child(std::make_shared<HasDot>());
    seq_solving->add_child(std::make_shared<DotCenteredStable>());
    seq_solving->add_child(std::make_shared<Attack>());
    root->add_child(seq_solving);

    // 2) 靠近目标（有血条）
    auto seq_near = std::make_shared<BTSequence>("ApproachTarget");
    //seq_near->add_child(std::make_shared<HasDot>());
    seq_near->add_child(std::make_shared<HasHpBar>());
    seq_near->add_child(std::make_shared<ApproachStep>());
    root->add_child(seq_near);

    // 3) 扫视找血条（有怪物点但没血条）
    // auto seq_scan = std::make_shared<BTSequence>("AcquireHpBar");
    // seq_scan->add_child(std::make_shared<HasDot>());
    // seq_scan->add_child(std::make_shared<ScanRotate>());
    // root->add_child(seq_scan);
    // 3) 按住shift找血条（有怪物点但没血条）
    auto seq_scan = std::make_shared<BTSequence>("AcquireHpBar");
    seq_scan->add_child(std::make_shared<HasDot>());
    seq_scan->add_child(std::make_shared<tabdown>());
    root->add_child(seq_scan);


    // 4) 搜索（没怪物点）
    auto seq_search = std::make_shared<BTSequence>("Search");
    seq_search->add_child(std::make_shared<NoDot>());
    seq_search->add_child(std::make_shared<clickPath>());
    root->add_child(seq_search);

    return root;
}
