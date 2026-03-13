/*
 * menu_select.c
 *
 *  Created on: Mar 12, 2026
 *      Author: Codex
 */

#include "menus.h"
#include "_menu_ui.h"
#include "_menu_controller.h"
#include "screen_driver.h"
#include "stm32f4xx_hal.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;

#define MENU_SELECT_HOLD_MS 300

static volatile uint8_t s_menu_select_blocks_render = 0;

static int8_t menu_select_encoder_step(void)
{
    int32_t delta = __HAL_TIM_GET_COUNTER(&htim3) - ENCODER_CENTER;
    if (delta <= -ENCODER_THRESHOLD) {
        __HAL_TIM_SET_COUNTER(&htim3, ENCODER_CENTER);
        return -1;
    }
    if (delta >= ENCODER_THRESHOLD) {
        __HAL_TIM_SET_COUNTER(&htim3, ENCODER_CENTER);
        return 1;
    }
    return 0;
}

static uint8_t menu_wrap_index(int16_t menu)
{
    while (menu < 0) {
        menu += AMOUNT_OF_MENUS;
    }
    while (menu >= AMOUNT_OF_MENUS) {
        menu -= AMOUNT_OF_MENUS;
    }
    return (uint8_t)menu;
}

static void menu_select_draw_mode(void)
{
    screen_driver_Fill(Black);

    for (uint8_t i = 0; i < 5; ++i) {
        const int16_t offset = (int16_t)i - 1;
        const uint8_t current = get_current_menu(CURRENT_MENU);
        const uint8_t menu_idx = menu_wrap_index((int16_t)current + offset);
        const int16_t y = (int16_t)(5 + (i * 10));
        const save_field_t sending_field = sending_field_for_menu((menu_list_t)menu_idx);

        write_underline_68(message->menu_list_long[menu_idx], TXT_LEFT, y, (i == 1) ? 1 : 0);

        if (sending_field != SAVE_FIELD_INVALID) {
            const uint8_t sending = save_get(sending_field);
            write_68(message->off_on[sending], 100, y);
        }
    }

    screen_driver_UpdateScreen();
}



uint8_t menu_select_refresh(void)
{
    static uint8_t was_pressed = 0;
    static uint8_t menu_mode_on = 0;
    static uint32_t pressed_since = 0;
    static uint8_t prev_btn3 = 1;

    const uint8_t is_pressed = (HAL_GPIO_ReadPin(GPIOB, Btn4_Pin) == 0) ? 1 : 0;
    s_menu_select_blocks_render = is_pressed;

    if (is_pressed && !was_pressed) {
        pressed_since = HAL_GetTick();
        menu_mode_on = 0;
    }

    if (is_pressed) {
        if (!menu_mode_on) {
            const uint32_t now = HAL_GetTick();
            if ((now - pressed_since) >= MENU_SELECT_HOLD_MS) {
                menu_mode_on = 1;
            }
        }

        if (menu_mode_on) {
            const int8_t step = menu_select_encoder_step();
            if (step != 0) {
                const uint8_t current = get_current_menu(CURRENT_MENU);
                const uint8_t next = menu_wrap_index((int16_t)current + (int16_t)step);
                set_current_menu(CURRENT_MENU, UI_MODIFY_SET, next);
            }

            const uint8_t btn3 = HAL_GPIO_ReadPin(GPIOB, Btn3_Pin);
            if (btn3 == 0 && prev_btn3 == 1) {
                const menu_list_t menu = (menu_list_t)get_current_menu(CURRENT_MENU);
                const save_field_t field = sending_field_for_menu(menu);
                if (field != SAVE_FIELD_INVALID) {
                    save_modify_u8(field, SAVE_MODIFY_INCREMENT, 0);
                    threads_display_notify(flag_for_menu(menu));
                }
            }
            prev_btn3 = btn3;


            menu_select_draw_mode();
        }

        was_pressed = 1;
        return menu_mode_on;
    }

    if (was_pressed) {
        if (menu_mode_on) {
            threads_display_notify(flag_for_menu((menu_list_t)get_current_menu(CURRENT_MENU)));
        } else {
            set_current_menu(CURRENT_MENU, UI_MODIFY_INCREMENT, 0);
            threads_display_notify(flag_for_menu((menu_list_t)get_current_menu(CURRENT_MENU)));
        }
    }

    was_pressed = 0;
    menu_mode_on = 0;
    prev_btn3 = 1;
    return 0;
}

uint8_t menu_select_blocks_render(void)
{
    return s_menu_select_blocks_render;
}
