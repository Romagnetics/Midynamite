#include "menus.h"
#include "_menu_ui.h" //menu change check
#include "main.h" //GPIO
#include "midi_transform.h" //calculate_incoming_midi
#include "midi_arp.h" //arp_process_pending_tempo_ticks
#include "midi_tempo.h" //mt_process_pending_tempo_out
#include "threads.h"
#include "usb_device.h" //MX_USB_DEVICE_Init
#include "utils.h" //debounce

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

// -------------------------
// Display flag helpers (local to threads.c)
// -------------------------
static inline uint32_t display_flag_for_menu(menu_list_t m)
{
    return (m < AMOUNT_OF_MENUS) ? (1 << (uint32_t)m) : 0;
}

// Wait on any menu flag bit [0..AMOUNT_OF_MENUS-1]
#define DISPLAY_FLAG_MASK ((AMOUNT_OF_MENUS >= 32) ? 0xFFFFFFFF : ((1 << (uint32_t)AMOUNT_OF_MENUS) - 1))

// -------------------------
// Internal state
// -------------------------
static osThreadId_t s_display_handle      = NULL;
static osThreadId_t s_midi_core_handle    = NULL;
static osThreadId_t s_medium_tasks_handle = NULL;

static osEventFlagsId_t s_display_flags;
static const osEventFlagsAttr_t s_flags_attrs = { .name = "display_flags" };

// -------------------------
// Thread attributes
// -------------------------
static const osThreadAttr_t s_display_attrs = {
  .name       = "display_update",
  .stack_size = 4096,
  .priority   = (osPriority_t)osPriorityBelowNormal,
};
static const osThreadAttr_t s_midi_core_attrs = {
  .name       = "midi_core",
  .stack_size = 128 * 4,
  .priority   = (osPriority_t)osPriorityRealtime,
};
static const osThreadAttr_t s_medium_tasks_attrs = {
  .name       = "medium_tasks",
  .stack_size = 128 * 4,
  .priority   = (osPriority_t)osPriorityAboveNormal,
};


// -------------------------
// Display thread
// -------------------------
static void DisplayUpdateThread(void *arg)
{
    (void)arg;
    for (;;)
    {
        uint32_t f = osEventFlagsWait(
            s_display_flags,
            DISPLAY_FLAG_MASK,
            osFlagsWaitAny,
            osWaitForever
        );

        screen_update_menu(f);
        osDelay(30);
    }
}

// -------------------------
// MIDI core thread
// -------------------------
static void MidiCoreThread(void *argument)
{
    (void)argument;
    MX_USB_DEVICE_Init();
    for (;;)
    {
        calculate_incoming_midi();


        uint32_t flags = osThreadFlagsWait(MIDI_CORE_FLAG_MASK, osFlagsWaitAny, 1);
        if ((flags & osFlagsError) != 0) {
            continue;
        }

        if ((flags & MIDI_CORE_FLAG_ARP_TICK) != 0) {
            arp_process_pending_tempo_ticks();
        }

        if ((flags & MIDI_CORE_FLAG_TEMPO_OUT) != 0) {
            mt_process_pending_tempo_out();
        }
        osDelay(1);

    }
}

// -------------------------
// Medium tasks thread
// -------------------------
static void MediumTasksThread(void *argument)
{
    (void)argument;

    // Initial menu draw trigger
    set_current_menu(CURRENT_MENU, UI_MODIFY_SET, save_get(SETTINGS_START_MENU));
    set_current_menu(OLD_MENU, UI_MODIFY_SET, 99);
    refresh_screen();

    for (;;)
    {
        menu_change_check();

        uint8_t old_menu     = get_current_menu(OLD_MENU);
        uint8_t current_menu = get_current_menu(CURRENT_MENU);

        if (old_menu != current_menu) {
            refresh_screen();
        }

        // IMPORTANT: your latest _menu_controller.c defines update_menu() with NO args.
        update_menu();

        current_menu = get_current_menu(CURRENT_MENU);
        set_current_menu(OLD_MENU, UI_MODIFY_SET, current_menu);

        // Btn3 toggles "currently sending" depending on menu
        static uint8_t OldBtn3State = 1;
        if (debounce_button(GPIOB, Btn3_Pin, &OldBtn3State, 50)) {
            start_stop_pressed();
        }

        // Panic (both buttons)
        extern UART_HandleTypeDef huart1, huart2;
        panic_midi(&huart1, &huart2, GPIOB, Btn1_Pin, Btn2_Pin);

        osDelay(10);
    }
}

// -------------------------
// Public API
// -------------------------
void threads_start(void)
{
    s_display_flags       = osEventFlagsNew(&s_flags_attrs);
    s_display_handle      = osThreadNew(DisplayUpdateThread, NULL, &s_display_attrs);
    s_midi_core_handle    = osThreadNew(MidiCoreThread,       NULL, &s_midi_core_attrs);
    s_medium_tasks_handle = osThreadNew(MediumTasksThread,    NULL, &s_medium_tasks_attrs);
}

void threads_display_notify(uint32_t flags)
{
    if (s_display_flags) osEventFlagsSet(s_display_flags, flags);
}

void threads_midi_core_set_flags(uint32_t flags)
{
    if (s_midi_core_handle == NULL) {
        return;
    }

    osThreadFlagsSet(s_midi_core_handle, flags);
}



osThreadId_t threads_display_handle(void)
{
    return s_display_handle;
}
