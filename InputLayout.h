#pragma once
#include <Windows.h>
#include <stdint.h>
#include <WinUser.h>

enum class KeyCode : uint8_t
{
    Unknown = 0,

    // 알파벳 (대소문자 구분 없이 매핑)
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // 숫자 키 (메인 키보드)
    Key0, Key1, Key2, Key3, Key4,
    Key5, Key6, Key7, Key8, Key9,

    // 기능/제어 키
    Space,
    Enter,
    Escape,
    Tab,
    Backspace,
    Delete,
    Insert,
    Home,
    End,
    PageUp,
    PageDown,

    // 방향키
    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,

    // 수정자 키 (Shift, Ctrl, Alt)
    LeftShift,
    RightShift,
    LeftCtrl,
    RightCtrl,
    LeftAlt,
    RightAlt,

    // 숫자패드 (선택적)
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    NumpadMultiply,   // *
    NumpadPlus,       // +
    NumpadMinus,      // -
    NumpadDecimal,    // .
    NumpadDivide,     // /

    // F키 (필요하면 더 추가 가능)
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    COUNT  // 배열 크기 / 열거 끝 표시용
};

class Input
{
public:
	KeyCode ToKeyCode(WPARAM vkCode);
};