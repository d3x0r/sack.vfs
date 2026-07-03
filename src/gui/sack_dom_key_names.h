// DOM KeyboardEvent name tables — `event.key` and `event.code`.
//
// Both tables are indexed by sack's KEY_* macros from <keybrd.h>, which
// resolve to whatever scancode value sack's vidlib delivers on the current
// platform (Win32 VK on Windows, evdev KEY_* on Linux, etc.). That way one
// source of truth covers every platform sack supports — no per-platform
// table duplication.
//
// References:
//   event.key  — https://www.w3.org/TR/uievents-key/
//   event.code — https://www.w3.org/TR/uievents-code/
//   event.keyCode (legacy) — Win32 VK numbers regardless of platform
//
// Lookup contract:
//   const char *keyName  = (vk < 256) ? kDomKeyName [vk] : nullptr;
//   const char *codeName = (vk < 256) ? kDomCodeName[vk] : nullptr;
//
//   keyName == nullptr  → key is a printable character; fall back to
//                          GetKeyText(packed) for the current modifier
//                          state ("a" vs "A", layout-aware, etc.).
//   codeName == nullptr → physical position unknown for this scancode.
//                          Leave event.code as the empty string.
//
// Tables are populated via a static-init lambda because MSVC doesn't accept
// C99 array designators (`[N] = "..."`) in any C++ mode — and C++20 only
// added designated initializers for struct *members*, not arrays. GCC/Clang
// take it as an extension; MSVC errors with C2187/C2059. The lambda form
// works identically across compilers.

#pragma once

#include <keybrd.h>   // pulls keybrd.h (KEY_* macros)

// ---------- event.key — logical key name --------------------------------
//
// NULL slots are printable: ProcessEvent falls back to GetKeyText(packed)
// which returns the current shift/IME-state-aware character ("a"/"A",
// "1"/"!", " " for space, etc. — already correct for event.key).

static const char *const *const kDomKeyName = []{
	static const char *table[256] = {};

	// --- whitespace / line control
	table[ KEY_BACKSPACE ]   = "Backspace";
	table[ KEY_TAB ]         = "Tab";
	table[ KEY_ENTER ]       = "Enter";
	table[ KEY_ESCAPE ]      = "Escape";
	table[ KEY_SPACE ]       = " ";          // DOM uses literal space, not "Space"

	// --- modifiers (no L/R distinction without extra GetKeyState plumbing;
	//     Phase 2 of the event work can refine to "ShiftLeft" vs "ShiftRight"
	//     for code, but key just says "Shift" regardless).
	table[ KEY_SHIFT ]       = "Shift";
	table[ KEY_CTRL ]        = "Control";
	table[ KEY_ALT ]         = "Alt";
	table[ KEY_CAPS_LOCK ]   = "CapsLock";
	table[ KEY_NUM_LOCK ]    = "NumLock";
	table[ KEY_SCROLL_LOCK ] = "ScrollLock";

	// --- navigation
	table[ KEY_PGUP ]   = "PageUp";
	table[ KEY_PGDN ]   = "PageDown";
	table[ KEY_END ]    = "End";
	table[ KEY_HOME ]   = "Home";
	table[ KEY_LEFT ]   = "ArrowLeft";
	table[ KEY_UP ]     = "ArrowUp";
	table[ KEY_RIGHT ]  = "ArrowRight";
	table[ KEY_DOWN ]   = "ArrowDown";
	table[ KEY_INSERT ] = "Insert";
	table[ KEY_DELETE ] = "Delete";

	// --- function keys (DOM defines F1..F24; sack typically tops at F12)
	table[ KEY_F1  ] = "F1";  table[ KEY_F2  ] = "F2";  table[ KEY_F3  ] = "F3";
	table[ KEY_F4  ] = "F4";  table[ KEY_F5  ] = "F5";  table[ KEY_F6  ] = "F6";
	table[ KEY_F7  ] = "F7";  table[ KEY_F8  ] = "F8";  table[ KEY_F9  ] = "F9";
	table[ KEY_F10 ] = "F10"; table[ KEY_F11 ] = "F11"; table[ KEY_F12 ] = "F12";

	// --- numpad: DOM event.key for numpad-with-numlock gives the digit
	//     character ("0".."9"), not "Numpad0". Same for math keys: "+"
	//     not "NumpadAdd". (event.code is where physical names live.)
	table[ KEY_PAD_0 ] = "0"; table[ KEY_PAD_1 ] = "1"; table[ KEY_PAD_2 ] = "2";
	table[ KEY_PAD_3 ] = "3"; table[ KEY_PAD_4 ] = "4"; table[ KEY_PAD_5 ] = "5";
	table[ KEY_PAD_6 ] = "6"; table[ KEY_PAD_7 ] = "7"; table[ KEY_PAD_8 ] = "8";
	table[ KEY_PAD_9 ] = "9";
	table[ KEY_PAD_PLUS  ] = "+";
	table[ KEY_PAD_MINUS ] = "-";
	table[ KEY_PAD_MULT  ] = "*";
	table[ KEY_PAD_DIV   ] = "/";
	table[ KEY_PAD_DOT   ] = ".";

	// Windows-key style — DOM gives "Meta" (or "OS" in older specs).
	// KEY_WINDOW_1 / KEY_WINDOW_2 on Win32 are the left/right Win keys.
	table[ KEY_WINDOW_1 ] = "Meta";
	table[ KEY_WINDOW_2 ] = "Meta";

	// Letters and digits: NULL → printable, use GetKeyText.
	// Punctuation (KEY_SEMICOLON, KEY_QUOTE, etc.) ditto.

	return table;
}();

// ---------- event.code — physical key position --------------------------
//
// DOM event.code is layout-independent (the physical key in its US-QWERTY
// position). NULL slots leave event.code empty.

static const char *const *const kDomCodeName = []{
	static const char *table[256] = {};

	// --- whitespace / line control
	table[ KEY_BACKSPACE ]   = "Backspace";
	table[ KEY_TAB ]         = "Tab";
	table[ KEY_ENTER ]       = "Enter";
	table[ KEY_ESCAPE ]      = "Escape";
	table[ KEY_SPACE ]       = "Space";

	// --- modifiers — DOM defines ShiftLeft / ShiftRight separately,
	//     but sack delivers a single KEY_SHIFT scancode. Best-effort:
	//     report the "Left" form. Same for Control / Alt.
	table[ KEY_SHIFT ]       = "ShiftLeft";
	table[ KEY_CTRL ]        = "ControlLeft";
	table[ KEY_ALT ]         = "AltLeft";
	table[ KEY_CAPS_LOCK ]   = "CapsLock";
	table[ KEY_NUM_LOCK ]    = "NumLock";
	table[ KEY_SCROLL_LOCK ] = "ScrollLock";
	table[ KEY_WINDOW_1 ]    = "MetaLeft";
	table[ KEY_WINDOW_2 ]    = "MetaRight";

	// --- navigation
	table[ KEY_PGUP ]   = "PageUp";
	table[ KEY_PGDN ]   = "PageDown";
	table[ KEY_END ]    = "End";
	table[ KEY_HOME ]   = "Home";
	table[ KEY_LEFT ]   = "ArrowLeft";
	table[ KEY_UP ]     = "ArrowUp";
	table[ KEY_RIGHT ]  = "ArrowRight";
	table[ KEY_DOWN ]   = "ArrowDown";
	table[ KEY_INSERT ] = "Insert";
	table[ KEY_DELETE ] = "Delete";

	// --- function keys
	table[ KEY_F1  ] = "F1";  table[ KEY_F2  ] = "F2";  table[ KEY_F3  ] = "F3";
	table[ KEY_F4  ] = "F4";  table[ KEY_F5  ] = "F5";  table[ KEY_F6  ] = "F6";
	table[ KEY_F7  ] = "F7";  table[ KEY_F8  ] = "F8";  table[ KEY_F9  ] = "F9";
	table[ KEY_F10 ] = "F10"; table[ KEY_F11 ] = "F11"; table[ KEY_F12 ] = "F12";

	// --- letters (physical position, US-QWERTY naming)
	table[ KEY_A ] = "KeyA"; table[ KEY_B ] = "KeyB"; table[ KEY_C ] = "KeyC";
	table[ KEY_D ] = "KeyD"; table[ KEY_E ] = "KeyE"; table[ KEY_F ] = "KeyF";
	table[ KEY_G ] = "KeyG"; table[ KEY_H ] = "KeyH"; table[ KEY_I ] = "KeyI";
	table[ KEY_J ] = "KeyJ"; table[ KEY_K ] = "KeyK"; table[ KEY_L ] = "KeyL";
	table[ KEY_M ] = "KeyM"; table[ KEY_N ] = "KeyN"; table[ KEY_O ] = "KeyO";
	table[ KEY_P ] = "KeyP"; table[ KEY_Q ] = "KeyQ"; table[ KEY_R ] = "KeyR";
	table[ KEY_S ] = "KeyS"; table[ KEY_T ] = "KeyT"; table[ KEY_U ] = "KeyU";
	table[ KEY_V ] = "KeyV"; table[ KEY_W ] = "KeyW"; table[ KEY_X ] = "KeyX";
	table[ KEY_Y ] = "KeyY"; table[ KEY_Z ] = "KeyZ";

	// --- digit row
	table[ KEY_0 ] = "Digit0"; table[ KEY_1 ] = "Digit1"; table[ KEY_2 ] = "Digit2";
	table[ KEY_3 ] = "Digit3"; table[ KEY_4 ] = "Digit4"; table[ KEY_5 ] = "Digit5";
	table[ KEY_6 ] = "Digit6"; table[ KEY_7 ] = "Digit7"; table[ KEY_8 ] = "Digit8";
	table[ KEY_9 ] = "Digit9";

	// --- punctuation (US-QWERTY positions)
	table[ KEY_SEMICOLON ]     = "Semicolon";
	table[ KEY_EQUAL ]         = "Equal";
	table[ KEY_COMMA ]         = "Comma";
	table[ KEY_DASH ]          = "Minus";
	table[ KEY_PERIOD ]        = "Period";
	table[ KEY_SLASH ]         = "Slash";
	table[ KEY_ACCENT ]        = "Backquote";
	table[ KEY_LEFT_BRACKET ]  = "BracketLeft";
	table[ KEY_RIGHT_BRACKET ] = "BracketRight";
	table[ KEY_BACKSLASH ]     = "Backslash";
	table[ KEY_QUOTE ]         = "Quote";

	// --- numpad (physical names — distinct from main row)
	table[ KEY_PAD_0 ] = "Numpad0"; table[ KEY_PAD_1 ] = "Numpad1"; table[ KEY_PAD_2 ] = "Numpad2";
	table[ KEY_PAD_3 ] = "Numpad3"; table[ KEY_PAD_4 ] = "Numpad4"; table[ KEY_PAD_5 ] = "Numpad5";
	table[ KEY_PAD_6 ] = "Numpad6"; table[ KEY_PAD_7 ] = "Numpad7"; table[ KEY_PAD_8 ] = "Numpad8";
	table[ KEY_PAD_9 ]      = "Numpad9";
	table[ KEY_PAD_PLUS  ]  = "NumpadAdd";
	table[ KEY_PAD_MINUS ]  = "NumpadSubtract";
	table[ KEY_PAD_MULT  ]  = "NumpadMultiply";
	table[ KEY_PAD_DIV   ]  = "NumpadDivide";
	table[ KEY_PAD_DOT   ]  = "NumpadDecimal";
	table[ KEY_PAD_ENTER ]  = "NumpadEnter";

	return table;
}();


// ---------- event.keyCode — legacy numeric (Win32 VK numbers) ----------
//
// Browsers historically standardize event.keyCode on Win32 VK numbers,
// regardless of OS. On Windows sack already delivers VK numbers in the
// packed key value, so we pass through. On Linux sack delivers evdev
// KEY_* scancodes (linux/input-event-codes.h values) which differ — a
// translation table is needed there.

#if defined( _WIN32 )

static inline uint32_t sack_vk_to_dom_keyCode( uint32_t vk ) {
	// Win32 KEY_* already are Win32 VK values; passthrough.
	return vk;
}

#elif defined( __linux__ )

// TODO: build the Linux evdev → Win32-VK table when needed.
// Chromium's ui/events/keycodes/dom/keycode_converter_data.inc has a
// canonical mapping (under BSD-style license — easy to lift) — about
// 200 entries covering everything DOM cares about. For now, passthrough
// (event.keyCode will be Linux scancode, not browser-compatible).
static inline uint32_t sack_vk_to_dom_keyCode( uint32_t vk ) {
	return vk;
}

#else

static inline uint32_t sack_vk_to_dom_keyCode( uint32_t vk ) {
	return vk;
}

#endif
