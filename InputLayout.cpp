#include "inputlayout.h"

KeyCode InputLayout::ToKeyCode(WPARAM vkCode)
{
    // 대소문자 통합 처리 (대문자로 변환해서 매핑)
    if (vkCode >= 'A' && vkCode <= 'Z')
    {
        return static_cast<KeyCode>(static_cast<uint8_t>(KeyCode::A) + (vkCode - 'A'));
    }

    if (vkCode >= 'a' && vkCode <= 'z')
    {
        return static_cast<KeyCode>(static_cast<uint8_t>(KeyCode::A) + (vkCode - 'a'));
    }

    // 2. 숫자 0~9 (메인 키보드 숫자열)
    if (vkCode >= '0' && vkCode <= '9')
    {
        return static_cast<KeyCode>(static_cast<uint8_t>(KeyCode::Key0) + (vkCode - '0'));
    }


    switch (vkCode)
    {
        // 공백 / 제어
    case VK_SPACE:      return KeyCode::Space;
    case VK_RETURN:     return KeyCode::Enter;
    case VK_ESCAPE:     return KeyCode::Escape;
    case VK_TAB:        return KeyCode::Tab;
    case VK_BACK:       return KeyCode::Backspace;
    case VK_DELETE:     return KeyCode::Delete;
    case VK_INSERT:     return KeyCode::Insert;
    case VK_HOME:       return KeyCode::Home;
    case VK_END:        return KeyCode::End;
    case VK_PRIOR:      return KeyCode::PageUp;     // Page Up
    case VK_NEXT:       return KeyCode::PageDown;   // Page Down

        // 방향키
    case VK_LEFT:       return KeyCode::LeftArrow;
    case VK_RIGHT:      return KeyCode::RightArrow;
    case VK_UP:         return KeyCode::UpArrow;
    case VK_DOWN:       return KeyCode::DownArrow;

        // 수정자 키
    case VK_LSHIFT:     return KeyCode::LeftShift;
    case VK_RSHIFT:     return KeyCode::RightShift;
    case VK_LCONTROL:   return KeyCode::LeftCtrl;
    case VK_RCONTROL:   return KeyCode::RightCtrl;
    case VK_LMENU:      return KeyCode::LeftAlt;    // Alt
    case VK_RMENU:      return KeyCode::RightAlt;

        // 숫자패드 (선택적으로 사용)
    case VK_NUMPAD0:    return KeyCode::Numpad0;
    case VK_NUMPAD1:    return KeyCode::Numpad1;
    case VK_NUMPAD2:    return KeyCode::Numpad2;
    case VK_NUMPAD3:    return KeyCode::Numpad3;
    case VK_NUMPAD4:    return KeyCode::Numpad4;
    case VK_NUMPAD5:    return KeyCode::Numpad5;
    case VK_NUMPAD6:    return KeyCode::Numpad6;
    case VK_NUMPAD7:    return KeyCode::Numpad7;
    case VK_NUMPAD8:    return KeyCode::Numpad8;
    case VK_NUMPAD9:    return KeyCode::Numpad9;
    case VK_MULTIPLY:   return KeyCode::NumpadMultiply;
    case VK_ADD:        return KeyCode::NumpadPlus;
    case VK_SUBTRACT:   return KeyCode::NumpadMinus;
    case VK_DECIMAL:    return KeyCode::NumpadDecimal;
    case VK_DIVIDE:     return KeyCode::NumpadDivide;

        // F1 ~ F12 (필요한 만큼만)
    case VK_F1:         return KeyCode::F1;
    case VK_F2:         return KeyCode::F2;
    case VK_F3:         return KeyCode::F3;
    case VK_F4:         return KeyCode::F4;
    case VK_F5:         return KeyCode::F5;
    case VK_F6:         return KeyCode::F6;
    case VK_F7:         return KeyCode::F7;
    case VK_F8:         return KeyCode::F8;
    case VK_F9:         return KeyCode::F9;
    case VK_F10:        return KeyCode::F10;
    case VK_F11:        return KeyCode::F11;
    case VK_F12:        return KeyCode::F12;

    default:
        return KeyCode::Unknown;
    }
}
