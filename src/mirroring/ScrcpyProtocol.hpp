#pragma once

#include <cstdint>

namespace Scrcpy {

// Control message types sent from desktop to scrcpy-server
enum ControlMessageType : uint8_t {
    SC_CONTROL_MSG_TYPE_INJECT_KEYCODE = 0,
    SC_CONTROL_MSG_TYPE_INJECT_TEXT = 1,
    SC_CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT = 2,
    SC_CONTROL_MSG_TYPE_INJECT_SCROLL_EVENT = 3,
    SC_CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON = 4,
    SC_CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL = 5,
    SC_CONTROL_MSG_TYPE_COLLAPSE_PANEL = 6,
    SC_CONTROL_MSG_TYPE_GET_CLIPBOARD = 7,
    SC_CONTROL_MSG_TYPE_SET_CLIPBOARD = 8,
    SC_CONTROL_MSG_TYPE_SET_SCREEN_POWER_MODE = 9,
    SC_CONTROL_MSG_TYPE_ROTATE_DEVICE = 10,
};

// Android MotionEvent actions
enum AndroidMotionEventAction : uint8_t {
    AMOTION_EVENT_ACTION_DOWN = 0,
    AMOTION_EVENT_ACTION_UP = 1,
    AMOTION_EVENT_ACTION_MOVE = 2,
    AMOTION_EVENT_ACTION_CANCEL = 3,
};

// Android KeyEvent actions
enum AndroidKeyEventAction : uint8_t {
    AKEY_EVENT_ACTION_DOWN = 0,
    AKEY_EVENT_ACTION_UP = 1,
};

// Android KeyCodes
enum AndroidKeyCode : uint32_t {
    AKEYCODE_HOME = 3,
    AKEYCODE_BACK = 4,
    AKEYCODE_APP_SWITCH = 187,
    AKEYCODE_POWER = 26,
    AKEYCODE_VOLUME_UP = 24,
    AKEYCODE_VOLUME_DOWN = 25,
};

// Video packet header (PTS + Size)
struct VideoPacketHeader {
    uint64_t pts;
    uint32_t size;
    bool isConfig;
    bool isKeyFrame;
};

} // namespace Scrcpy
