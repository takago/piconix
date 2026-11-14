/* SPDX-License-Identifier: GPL-3.0-or-later */
/* PicoNIX仮想スクリーン：端末の文字表示・キー名解釈をTTY/PTYから分離する． */
#ifndef PICONIX_KERNEL_SCREEN_H
#define PICONIX_KERNEL_SCREEN_H

#include <stddef.h>
#include <stdint.h>

#define VT_COLS 80u
#define VT_ROWS 24u
#define SCREEN_PTY_COUNT 2
#define SCREEN_CDC_COUNT 2

/* CDCの画面選択状態．TTY切替側が参照する最小限の共有状態． */
extern uint8_t cdc_active;
extern uint8_t cdc_dual_mode;

/* USB送信リングへ積めたバイト数を返すコールバック． */
typedef size_t (*screen_cdc_emit_t)(const uint8_t *bytes, size_t count);
void screen_cdc_init(void);
void screen_cdc_begin_repaint(void);
void screen_cdc_poll(screen_cdc_emit_t emit);
int screen_cdc_repainting(void);

/* キー解析はTTY入力やシェル生成を行わず，呼出し元へ操作を返す． */
typedef enum {
    SCREEN_CDC_NONE=0,
    SCREEN_CDC_SWITCH,
    SCREEN_CDC_DUAL_TOGGLE,
    SCREEN_CDC_FOCUS_UP,
    SCREEN_CDC_FOCUS_DOWN,
    SCREEN_CDC_CYCLE_LEFT,
    SCREEN_CDC_CYCLE_RIGHT
} screen_cdc_action_t;
typedef struct {
    uint8_t flushed[6];   /* 判定できなかったESC列の元データ． */
    uint8_t flushed_count;
    uint8_t flushed_tty;  /* ESC列の受信開始時の入力先． */
    uint8_t pass;         /* 通常入力を配送する必要がある場合は1． */
    uint8_t pass_byte;
    uint8_t pass_tty;
    screen_cdc_action_t action;
} screen_cdc_key_result_t;
void screen_cdc_key_feed(uint8_t ch,uint32_t now,uint8_t tty,
                         screen_cdc_key_result_t *result);
int screen_cdc_key_timeout(uint32_t now,screen_cdc_key_result_t *result);
void screen_cdc_key_reset(void);

typedef struct {
    char cells[VT_ROWS][VT_COLS];              /* 画面内の文字セル． */
    uint8_t inverse[VT_ROWS][(VT_COLS+7u)/8u]; /* 反転表示セルのビット列． */
    uint8_t row;            /* カーソル行． */
    uint8_t col;            /* カーソル列． */
    uint8_t esc;            /* エスケープシーケンスの解析状態． */
    uint8_t params;         /* 解析中のパラメータ数・位置の管理． */
    uint8_t private_csi;    /* 私用CSIシーケンスの解析状態． */
    uint8_t reverse;        /* 現在の反転表示属性． */
    uint8_t cursor_hidden;  /* カーソル非表示の状態． */
    uint8_t clipped;        /* 画面外への描画を抑制する状態． */
    uint16_t arg[3];        /* CSIシーケンスの数値引数． */
    uint32_t revision;      /* 画面更新を追跡する世代番号． */
} vt_screen_t;

/* 固定画面の保存先．画面の所有権はscreen.cにあり，CDC描画は参照のみ行う． */
extern vt_screen_t pty_screen[SCREEN_PTY_COUNT];
extern vt_screen_t cdc_screen[SCREEN_CDC_COUNT];

void screen_clear(vt_screen_t *screen);
void screen_reset(vt_screen_t *screen);
/* ESC[6nを検出したとき1を返す．TTYへの応答は呼出し側の責務． */
int screen_byte(vt_screen_t *screen, int is_pty, uint8_t c);
/* FDストリーム位置は呼出し側が保持し，スクリーン側で更新する． */
int screen_read(const vt_screen_t *screen, uint32_t *pos, void *p, size_t n);
/* .keyのkey:/text:を入力バイト列へ変換．出力上限127B，不正指定は-1． */
int screen_key_decode(const void *p, size_t n, char out[128], size_t *length);

#endif
