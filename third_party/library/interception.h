#ifndef _INTERCEPTION_H_
#define _INTERCEPTION_H_

#ifdef INTERCEPTION_STATIC
    #define INTERCEPTION_API
#else
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef INTERCEPTION_EXPORT
            #ifdef __GNUC__
                #define INTERCEPTION_API __attribute__((dllexport))
            #else
                #define INTERCEPTION_API __declspec(dllexport)
            #endif
        #else
            #ifdef __GNUC__
                #define INTERCEPTION_API __attribute__((dllimport))
            #else
                #define INTERCEPTION_API __declspec(dllimport)
            #endif
        #endif
    #else
        #if __GNUC__ >= 4
            #define INTERCEPTION_API __attribute__ ((visibility("default")))
        #else
            #define INTERCEPTION_API
        #endif
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define INTERCEPTION_MAX_KEYBOARD 10

#define INTERCEPTION_MAX_MOUSE 10

#define INTERCEPTION_MAX_DEVICE ((INTERCEPTION_MAX_KEYBOARD) + (INTERCEPTION_MAX_MOUSE))

#define INTERCEPTION_KEYBOARD(index) ((index) + 1)

#define INTERCEPTION_MOUSE(index) ((INTERCEPTION_MAX_KEYBOARD) + (index) + 1)

typedef void *InterceptionContext;

typedef int InterceptionDevice;

typedef int InterceptionPrecedence;

typedef unsigned short InterceptionFilter;

typedef int (*InterceptionPredicate)(InterceptionDevice device);

enum InterceptionKeyState
{
    INTERCEPTION_KEY_DOWN             = 0x00,
    INTERCEPTION_KEY_UP               = 0x01,
    INTERCEPTION_KEY_E0               = 0x02,
    INTERCEPTION_KEY_E1               = 0x04,
    INTERCEPTION_KEY_TERMSRV_SET_LED  = 0x08,
    INTERCEPTION_KEY_TERMSRV_SHADOW   = 0x10,
    INTERCEPTION_KEY_TERMSRV_VKPACKET = 0x20
};

enum InterceptionFilterKeyState
{
    INTERCEPTION_FILTER_KEY_NONE             = 0x0000,
    INTERCEPTION_FILTER_KEY_ALL              = 0xFFFF,
    INTERCEPTION_FILTER_KEY_DOWN             = INTERCEPTION_KEY_UP,
    INTERCEPTION_FILTER_KEY_UP               = INTERCEPTION_KEY_UP << 1,
    INTERCEPTION_FILTER_KEY_E0               = INTERCEPTION_KEY_E0 << 1,
    INTERCEPTION_FILTER_KEY_E1               = INTERCEPTION_KEY_E1 << 1,
    INTERCEPTION_FILTER_KEY_TERMSRV_SET_LED  = INTERCEPTION_KEY_TERMSRV_SET_LED << 1,
    INTERCEPTION_FILTER_KEY_TERMSRV_SHADOW   = INTERCEPTION_KEY_TERMSRV_SHADOW << 1,
    INTERCEPTION_FILTER_KEY_TERMSRV_VKPACKET = INTERCEPTION_KEY_TERMSRV_VKPACKET << 1
};

enum InterceptionMouseState
{
    INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN   = 0x001,
    INTERCEPTION_MOUSE_LEFT_BUTTON_UP     = 0x002,
    INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN  = 0x004,
    INTERCEPTION_MOUSE_RIGHT_BUTTON_UP    = 0x008,
    INTERCEPTION_MOUSE_MIDDLE_BUTTON_DOWN = 0x010,
    INTERCEPTION_MOUSE_MIDDLE_BUTTON_UP   = 0x020,

    INTERCEPTION_MOUSE_BUTTON_1_DOWN      = INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN,
    INTERCEPTION_MOUSE_BUTTON_1_UP        = INTERCEPTION_MOUSE_LEFT_BUTTON_UP,
    INTERCEPTION_MOUSE_BUTTON_2_DOWN      = INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN,
    INTERCEPTION_MOUSE_BUTTON_2_UP        = INTERCEPTION_MOUSE_RIGHT_BUTTON_UP,
    INTERCEPTION_MOUSE_BUTTON_3_DOWN      = INTERCEPTION_MOUSE_MIDDLE_BUTTON_DOWN,
    INTERCEPTION_MOUSE_BUTTON_3_UP        = INTERCEPTION_MOUSE_MIDDLE_BUTTON_UP,

    INTERCEPTION_MOUSE_BUTTON_4_DOWN      = 0x040,
    INTERCEPTION_MOUSE_BUTTON_4_UP        = 0x080,
    INTERCEPTION_MOUSE_BUTTON_5_DOWN      = 0x100,
    INTERCEPTION_MOUSE_BUTTON_5_UP        = 0x200,

    INTERCEPTION_MOUSE_WHEEL              = 0x400,
    INTERCEPTION_MOUSE_HWHEEL             = 0x800
};

enum InterceptionFilterMouseState
{
    INTERCEPTION_FILTER_MOUSE_NONE               = 0x0000,
    INTERCEPTION_FILTER_MOUSE_ALL                = 0xFFFF,

    INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_DOWN   = INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN,
    INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_UP     = INTERCEPTION_MOUSE_LEFT_BUTTON_UP,
    INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_DOWN  = INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN,
    INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_UP    = INTERCEPTION_MOUSE_RIGHT_BUTTON_UP,
    INTERCEPTION_FILTER_MOUSE_MIDDLE_BUTTON_DOWN = INTERCEPTION_MOUSE_MIDDLE_BUTTON_DOWN,
    INTERCEPTION_FILTER_MOUSE_MIDDLE_BUTTON_UP   = INTERCEPTION_MOUSE_MIDDLE_BUTTON_UP,

    INTERCEPTION_FILTER_MOUSE_BUTTON_1_DOWN      = INTERCEPTION_MOUSE_BUTTON_1_DOWN,
    INTERCEPTION_FILTER_MOUSE_BUTTON_1_UP        = INTERCEPTION_MOUSE_BUTTON_1_UP,
    INTERCEPTION_FILTER_MOUSE_BUTTON_2_DOWN      = INTERCEPTION_MOUSE_BUTTON_2_DOWN,
    INTERCEPTION_FILTER_MOUSE_BUTTON_2_UP        = INTERCEPTION_MOUSE_BUTTON_2_UP,
    INTERCEPTION_FILTER_MOUSE_BUTTON_3_DOWN      = INTERCEPTION_MOUSE_BUTTON_3_DOWN,
    INTERCEPTION_FILTER_MOUSE_BUTTON_3_UP        = INTERCEPTION_MOUSE_BUTTON_3_UP,

    INTERCEPTION_FILTER_MOUSE_BUTTON_4_DOWN      = INTERCEPTION_MOUSE_BUTTON_4_DOWN,
    INTERCEPTION_FILTER_MOUSE_BUTTON_4_UP        = INTERCEPTION_MOUSE_BUTTON_4_UP,
    INTERCEPTION_FILTER_MOUSE_BUTTON_5_DOWN      = INTERCEPTION_MOUSE_BUTTON_5_DOWN,
    INTERCEPTION_FILTER_MOUSE_BUTTON_5_UP        = INTERCEPTION_MOUSE_BUTTON_5_UP,

    INTERCEPTION_FILTER_MOUSE_WHEEL              = INTERCEPTION_MOUSE_WHEEL,
    INTERCEPTION_FILTER_MOUSE_HWHEEL             = INTERCEPTION_MOUSE_HWHEEL,

    INTERCEPTION_FILTER_MOUSE_MOVE               = 0x1000
};

enum InterceptionMouseFlag
{
    INTERCEPTION_MOUSE_MOVE_RELATIVE      = 0x000,
    INTERCEPTION_MOUSE_MOVE_ABSOLUTE      = 0x001,
    INTERCEPTION_MOUSE_VIRTUAL_DESKTOP    = 0x002,
    INTERCEPTION_MOUSE_ATTRIBUTES_CHANGED = 0x004,
    INTERCEPTION_MOUSE_MOVE_NOCOALESCE    = 0x008,
    INTERCEPTION_MOUSE_TERMSRV_SRC_SHADOW = 0x100
};

typedef struct
{
    unsigned short state;
    unsigned short flags;
    short rolling;
    int x;
    int y;
    unsigned int information;
} InterceptionMouseStroke;
    // typedef struct {
    //     unsigned short state;      // 鼠标按键/滚轮事件类型（按下/松开/滚轮等）
//     0x001	0000 0000 0000 0001	左键按下
// 0x002	0000 0000 0000 0010	左键释放
// 0x004	0000 0000 0000 0100	右键按下
// 0x008	0000 0000 0000 1000	右键释放
// 0x010	0000 0000 0001 0000	中键按下
// 0x020	0000 0000 0010 0000	中键释放
    //     unsigned short flags;      // 移动方式/附加标志（相对/绝对/虚拟桌面等）
    //     short rolling;             // 滚轮增量（wheel/hwheel 的 delta）
    //     int x;                     // 移动量或绝对坐标 X
    //     int y;                     // 移动量或绝对坐标 Y
    //     unsigned int information;  // 附加信息（通常填0，库/驱动可能写入）
    // } InterceptionMouseStroke;


typedef struct
{
    unsigned short code;
    unsigned short state;
    unsigned int information;
} InterceptionKeyStroke;
// // 一个“键盘事件”(keystroke)的数据结构：描述一次按下/松开某个键的输入
// typedef struct
// {
//     unsigned short code;        // 扫描码（Scan Code），不是 VK。比如 A 键、方向键等对应的硬件码
//     // 通常配合 MapVirtualKey(..., MAPVK_VK_TO_VSC_EX) 得到
//
//     unsigned short state;       // 事件状态/标志位（按下/抬起/扩展键等）
//     // 常见组合：
//     // - INTERCEPTION_KEY_DOWN  : 按下
//     // - INTERCEPTION_KEY_UP    : 松开
//     // - INTERCEPTION_KEY_E0    : 扩展键前缀 E0（方向键、Insert/Delete、Home/End 等）
//     // - INTERCEPTION_KEY_E1    : 扩展键前缀 E1（很少用，如 Pause/Break 等）
//
//     unsigned int information;   // 附加信息（库内部/驱动相关的元数据）
//     // 你做“发送输入”(interception_send)时通常填 0 即可
// } InterceptionKeyStroke;


typedef char InterceptionStroke[sizeof(InterceptionMouseStroke)];

InterceptionContext INTERCEPTION_API interception_create_context(void);

void INTERCEPTION_API interception_destroy_context(InterceptionContext context);

InterceptionPrecedence INTERCEPTION_API interception_get_precedence(InterceptionContext context, InterceptionDevice device);

void INTERCEPTION_API interception_set_precedence(InterceptionContext context, InterceptionDevice device, InterceptionPrecedence precedence);

InterceptionFilter INTERCEPTION_API interception_get_filter(InterceptionContext context, InterceptionDevice device);

void INTERCEPTION_API interception_set_filter(InterceptionContext context, InterceptionPredicate predicate, InterceptionFilter filter);

InterceptionDevice INTERCEPTION_API interception_wait(InterceptionContext context);

InterceptionDevice INTERCEPTION_API interception_wait_with_timeout(InterceptionContext context, unsigned long milliseconds);

int INTERCEPTION_API interception_send(InterceptionContext context, InterceptionDevice device, const InterceptionStroke *stroke, unsigned int nstroke);

int INTERCEPTION_API interception_receive(InterceptionContext context, InterceptionDevice device, InterceptionStroke *stroke, unsigned int nstroke);

unsigned int INTERCEPTION_API interception_get_hardware_id(InterceptionContext context, InterceptionDevice device, void *hardware_id_buffer, unsigned int buffer_size);

int INTERCEPTION_API interception_is_invalid(InterceptionDevice device);

int INTERCEPTION_API interception_is_keyboard(InterceptionDevice device);

int INTERCEPTION_API interception_is_mouse(InterceptionDevice device);

#ifdef __cplusplus
}
#endif

#endif
