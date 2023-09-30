/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

#include "quantum.h"

enum custom_keycodes {
  MC_ESC = SAFE_RANGE,
  MC_RYCST,
  KC_MY_BTN1, // Remap上では 0x5DAF
  KC_MY_BTN2, // Remap上では 0x5DB0
  KC_MY_BTN3  // Remap上では 0x5DB1
};

// コード表
// 【KBC_RST: 0x5DA5】Keyball 設定のリセット
// 【KBC_SAVE: 0x5DA6】現在の Keyball 設定を EEPROM に保存します
// 【CPI_I100: 0x5DA7】CPI を 100 増加させます(最大:12000)
// 【CPI_D100: 0x5DA8】CPI を 100 減少させます(最小:100)
// 【CPI_I1K: 0x5DA9】CPI を 1000 増加させます(最大:12000)
// 【CPI_D1K: 0x5DAA】CPI を 1000 減少させます(最小:100)
// 【SCRL_TO: 0x5DAB】タップごとにスクロールモードの ON/OFF を切り替えます
// 【SCRL_MO: 0x5DAC】キーを押している間、スクロールモードになります
// 【SCRL_DVI: 0x5DAD】スクロール除数を１つ上げます(max D7 = 1/128)←
// 最もスクロール遅い 【SCRL_DVD: 0x5DAE】スクロール除数を１つ下げます(min D0 =
// 1/1)← 最もスクロール速い

////////////////////////////////////
///
/// 自動マウスレイヤーの実装 ここから
/// 参考にさせていただいたページ
/// https://zenn.dev/takashicompany/articles/69b87160cda4b9
///
////////////////////////////////////

enum click_state {
  NONE = 0,
  WAITING, // マウスレイヤーが有効になるのを待つ。 Wait for mouse layer to
           // activate.
  CLICKABLE, // マウスレイヤー有効になりクリック入力が取れる。 Mouse layer is
             // enabled to take click input.
  CLICKING,  // クリック中。 Clicking.
};

enum click_state
    state; // 現在のクリック入力受付の状態 Current click input reception status
uint16_t click_timer; // タイマー。状態に応じて時間で判定する。 Timer. Time to
                      // determine the state of the system.

uint16_t to_reset_time = 800; // この秒数(千分の一秒)、CLICKABLE状態ならクリックレイヤーが無効になる。
                              // For this number of seconds (milliseconds), the
                              // click layer is disabled if in CLICKABLE state.

const int16_t to_clickable_movement = 0; // クリックレイヤーが有効になるしきい値
const uint16_t click_layer = 5; // マウス入力が可能になった際に有効になるレイヤー。Layers
                                // enabled when mouse input is enabled

int16_t mouse_record_threshold =
    30; // ポインターの動きを一時的に記録するフレーム数。 Number of frames in
        // which the pointer movement is temporarily recorded.
int16_t mouse_move_count_ratio =
    5; // ポインターの動きを再生する際の移動フレームの係数。 The coefficient of
       // the moving frame when replaying the pointer movement.

int16_t mouse_movement;

// クリック用のレイヤーを有効にする。　Enable layers for clicks
void enable_click_layer(void) {
  layer_on(click_layer);
  click_timer = timer_read();
  state = CLICKABLE;
}

// クリック用のレイヤーを無効にする。 Disable layers for clicks.
void disable_click_layer(void) {
  state = NONE;
  layer_off(click_layer);
}

// 自前の絶対数を返す関数。 Functions that return absolute numbers.
int16_t my_abs(int16_t num) {
  if (num < 0) {
    num = -num;
  }

  return num;
}

// 自前の符号を返す関数。 Function to return the sign.
int16_t mmouse_move_y_sign(int16_t num) {
  if (num < 0) {
    return -1;
  }

  return 1;
}

// 現在クリックが可能な状態か。 Is it currently clickable?
bool is_clickable_mode(void) { return state == CLICKABLE || state == CLICKING; }


report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
  int16_t current_x = mouse_report.x;
  int16_t current_y = mouse_report.y;

  if (current_x != 0 || current_y != 0) {

    switch (state) {
    case CLICKABLE:
      click_timer = timer_read();
      break;

    case CLICKING:
      break;

    case WAITING:
      mouse_movement += my_abs(current_x) + my_abs(current_y);

      if (mouse_movement >= to_clickable_movement) {
        mouse_movement = 0;
        enable_click_layer();
      }
      break;

    default:
      click_timer = timer_read();
      state = WAITING;
      mouse_movement = 0;
    }
  } else {
    switch (state) {
    case CLICKING:
      break;

    case CLICKABLE:
      if (timer_elapsed(click_timer) > to_reset_time) {
        disable_click_layer();
      }
      break;

    case WAITING:
      if (timer_elapsed(click_timer) > 50) {
        mouse_movement = 0;
        state = NONE;
      }
      break;

    default:
      mouse_movement = 0;
      state = NONE;
    }
  }

  mouse_report.x = current_x;
  mouse_report.y = current_y;

  return mouse_report;
}

////////////////////////////////////
///
/// 自動マウスレイヤーの実装 ここまで
///
////////////////////////////////////

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT_universal(
    MC_ESC   , KC_1    , KC_2     , KC_3     , KC_4     , KC_5     ,                                  KC_6     , KC_7     , KC_8     , KC_9     , KC_0     , KC_GRV  ,
    KC_TAB   , KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                                  KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     , KC_MINS  ,
    KC_LCTL   , KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                                  KC_H     , KC_J     , KC_K     , KC_L     , KC_SCLN  , KC_QUOT  ,
    KC_LSFT    , KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     , KC_RBRC  ,              KC_LBRC, KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  , MC_RYCST  ,
    MO(4)  , KC_LCTL  , KC_LALT  , KC_LGUI,LT(1,KC_LNG2),LT(2,KC_SPC),LT(3,KC_LNG1),          LT(2,KC_ENT),KC_BSPC  ,_______   ,_______   , _______ , KC_LNG1  , KC_PSCR
  ),

  [1] = LAYOUT_universal(
    KC_GRV, S(KC_1)  , KC_LBRC  , S(KC_3)  , S(KC_4)  , S(KC_5)  ,                                  KC_EQL   , S(KC_6)    ,S(KC_QUOT), S(KC_8)  , S(KC_9)  ,S(KC_INT1),
    S(KC_DEL), S(KC_Q)  , S(KC_W)  , S(KC_E)  , S(KC_R)  , S(KC_T)  ,                                  S(KC_Y)  , S(KC_U) ,S(KC_I)   , S(KC_O)  , S(KC_P)  , KC_BSLS,
    KC_GRV  , KC_EXLM  , KC_AT  ,   KC_HASH,   KC_DLR  ,   KC_PERC,                                    KC_CIRC, KC_AMPR   ,  KC_ASTR , KC_LPRN  , KC_RPRN  , KC_TILD,
    _______,  _______  , _______, _______  , _______   , _______ , KC_LBRC,                   KC_RBRC, KC_EQL , KC_UNDS   , KC_PLUS  , KC_LCBR  , KC_RCBR  , KC_PIPE,

    _______  ,S(KC_LCTL),S(KC_LALT),S(KC_LGUI), _______  , _______  , _______  ,            _______  , _______  , _______  ,S(KC_RGUI), _______  , S(KC_RALT), _______
  ),

  [2] = LAYOUT_universal(
    _______  , KC_F1    , KC_F2    , KC_F3    , KC_F4    , KC_F5    ,                                  KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   , KC_F11   ,
    _______  , _______  , KC_7     , KC_8     , KC_9     , _______  ,                                  KC_VOLD  , KC_VOLU  , KC_MUTE  ,KC_BRID  ,  KC_BRIU  , KC_F12   ,
    _______  , _______  , KC_4     , KC_5     , KC_6     ,S(KC_SCLN),                                  KC_LEFT  , KC_DOWN  ,  KC_UP   , KC_RGHT  , KC_BTN3  , KC_PGUP  ,
    _______  , _______ , KC_1     , KC_2     , KC_3     ,S(KC_MINS), S(KC_8)  ,            S(KC_9)  , KC_BTN4  , KC_BTN1  , KC_BTN3  , KC_BTN2  , KC_BTN5  , KC_PGDN  ,
    _______  , _______  , KC_0     , KC_DOT   , _______  , _______  , _______  ,             KC_DEL  , _______  , _______  , _______  , _______  , _______  , _______
  ),

  [3] = LAYOUT_universal(
    RGB_TOG  , _______  , _______  , _______  , _______  , _______  ,                                  RGB_M_P  , RGB_M_B  , RGB_M_R  , RGB_M_SW , RGB_M_SN , RGB_M_K  ,
    RGB_MOD  , RGB_HUI  , RGB_SAI  , RGB_VAI  , _______  , _______  ,                                  RGB_M_X  , RGB_M_G  , RGB_M_T  , RGB_M_TW , _______  , _______  ,
    RGB_RMOD , RGB_HUD  , RGB_SAD  , RGB_VAD  , _______  , _______  ,                                  CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , KBC_SAVE , KC_PGUP  ,
    _______  , _______  , SCRL_DVD , SCRL_DVI , SCRL_MO  , SCRL_TO  , EE_CLR  ,            EE_CLR  , KC_HOME  , KC_PGDN  , KC_PGUP  , KC_END   , KBC_RST  , KC_PGDN  ,
    QK_BOOT    , _______  , KC_LEFT  , KC_DOWN  , KC_UP    , KC_RGHT  , _______  ,            _______  , KC_BSPC  , _______  , _______  , _______  , _______  , QK_BOOT
  ),
  [4] = LAYOUT_universal(
    KC_GRV, S(KC_1)  , KC_LBRC  , S(KC_3)  , S(KC_4)  , S(KC_5)  ,                                  KC_EQL   , S(KC_6)  ,S(KC_QUOT), S(KC_8)  , S(KC_9)  ,S(KC_INT1),
    S(KC_DEL), S(KC_Q)  , S(KC_W)  , S(KC_E)  , S(KC_R)  , S(KC_T)  ,                                  S(KC_Y)  , S(KC_U)  , S(KC_I)  , S(KC_O)  , S(KC_P)  ,S(KC_INT3),
    S(KC_TAB), S(KC_A)  , S(KC_S)  , S(KC_D)  , S(KC_F)  , S(KC_G)  ,                                  S(KC_H)  , S(KC_J)  , S(KC_K)  , S(KC_L)  , KC_QUOT  , S(KC_2)  ,
    _______  , S(KC_Z)  , S(KC_X)  , S(KC_C)  , S(KC_V)  , S(KC_B)  ,S(KC_RBRC),           S(KC_NUHS), S(KC_N)  , S(KC_M)  ,S(KC_COMM), S(KC_DOT),S(KC_SLSH),S(KC_RSFT),
    _______  ,S(KC_LCTL),S(KC_LALT),S(KC_LGUI), _______  , _______  , _______  ,            _______  , _______  , _______  ,S(KC_RGUI), _______  , S(KC_RALT), _______
  ),
    [5] = LAYOUT_universal(
    _______  , _______  , _______  , _______  , _______  , _______  ,                                  _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                  _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                  _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  , _______  ,            _______  , _______  ,KC_MY_BTN1, KC_MY_BTN3  ,KC_MY_BTN2, _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  , _______  ,            _______  , _______  , _______  , _______  , _______  , _______  , _______
  )

};
// clang-format on


#ifdef OLED_ENABLE

#include "lib/oledkit/oledkit.h"
static const char PROGMEM qmk_logo[] = {
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,
    0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0xA0,
    0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB,
    0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xC0, 0xC1,
    0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC,
    0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0x00};
static const char PROGMEM qmk_logo_connect[] = {
    0x80, 0x10, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,
    0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x11, 0xA0,
    0x10, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB,
    0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0x11, 0xC0, 0x10,
    0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC,
    0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0x11, 0x00};
static const char PROGMEM qmk_1[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0x00};
static const char PROGMEM qmk_2[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0x20, 0x20, 0xDF, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xDF, 0x20, 0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0x00};
static const char PROGMEM qmk_3[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0x00};
static const char PROGMEM qmk_4[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0xDF, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0x00};
static const char PROGMEM qmk_5[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0x20, 0x20, 0xDF, 0x00};
static const char PROGMEM qmk_6[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20, 0xDF, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0xDF, 0xDF, 0xDF, 0xDF, 0x20, 0x20, 0xDF, 0x00};

void oledkit_render_info_user(void) {
  // デフォルトで押下したキー、動かしたボールの動きが表示される設定は無効化する
  // keyball_oled_render_keyinfo();
  // keyball_oled_render_ballinfo();

  switch (get_highest_layer(layer_state)) {
  case 0: // デフォルトレイヤーのときはPCとの疎通チェックのために特別な画像を表示
    oled_write_P(qmk_logo_connect, false);
    break;
  case 1:
    oled_write_P(qmk_1, false);
    break;
  case 2:
    oled_write_P(qmk_2, false);
    break;
  case 3:
    oled_write_P(qmk_3, false);
    break;
  case 4:
    oled_write_P(qmk_4, false);
    break;
  case 5:
    oled_write_P(qmk_5, false);
    break;
  case 6:
    oled_write_P(qmk_6, false);
    break;
  default:
    oled_write_P(qmk_logo, false);
    break;
  }
}

#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  switch (keycode) {
  case KC_MY_BTN1:
  case KC_MY_BTN2:
  case KC_MY_BTN3: {
    report_mouse_t currentReport = pointing_device_get_report();

    // どこのビットを対象にするか。 Which bits are to be targeted?
    uint8_t btn = 1 << (keycode - KC_MY_BTN1);

    if (record->event.pressed) {
      // ビットORは演算子の左辺と右辺の同じ位置にあるビットを比較して、両方のビットのどちらかが「1」の場合に「1」にします。
      // Bit OR compares bits in the same position on the left and right sides
      // of the operator and sets them to "1" if either of both bits is "1".
      currentReport.buttons |= btn;
      state = CLICKING;
    } else {
      // ビットANDは演算子の左辺と右辺の同じ位置にあるビットを比較して、両方のビットが共に「1」の場合だけ「1」にします。
      // Bit AND compares the bits in the same position on the left and right
      // sides of the operator and sets them to "1" only if both bits are "1"
      // together.
      currentReport.buttons &= ~btn;
      enable_click_layer();
    }

    pointing_device_set_report(currentReport);
    pointing_device_send();
    return false;
    break;
  case MC_ESC:
    if (record->event.pressed) {
      register_code(KC_LNG2);
      register_code(KC_ESC);
      return false;
    } else {
      unregister_code(KC_LNG2);
      unregister_code(KC_ESC);
      return false;
      // キーコード QMKBEST が放された時
    }
    break;
  case MC_RYCST:
    if (record->event.pressed) {
      register_code(KC_LALT);
      register_code(KC_SPC);
      register_code(KC_LNG2);
      return false;
    } else {
      unregister_code(KC_LALT);
      unregister_code(KC_SPC);
      unregister_code(KC_LNG2);
      return false;
      // キーコード QMKBEST が放された時
    }
    break;
  default:
    if (record->event.pressed) {
      disable_click_layer();
    }
  }
  }

  return true;
}
layer_state_t layer_state_set_user(layer_state_t state)
{
  // レイヤーが1または3の場合、スクロールモードが有効になる
  keyball_set_scroll_mode(get_highest_layer(state) == 3);
  // keyball_set_scroll_mode(get_highest_layer(state) == 1 || get_highest_layer(state) == 3);

  // レイヤーとLEDを連動させる
  uint8_t layer = biton32(state);
  switch (layer)
  {
  case 4:
    rgblight_sethsv(HSV_WHITE);
    break;

  default:
    rgblight_sethsv(HSV_OFF);
  }

  return state;
}


