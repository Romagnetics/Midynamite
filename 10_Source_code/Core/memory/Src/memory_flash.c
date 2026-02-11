/*
 * memory_flash.c
 *
 *  Created on: Sep 05, 2025
 *      Author: Romain Dereu
 */

#include <string.h>
#include "memory_main.h"
#include "_menu_controller.h" // for  save_mark_all_changed
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "utils.h"



/* Map which SAVE_FIELDs are u32 vs u8 (counts derived from your enum usage) */
enum {
    /* u32 fields */
    IDX_U32_TEMPO_CURRENT_TEMPO = 0,

	IDX_U32_TEMPO_CLICK_RATE,

	IDX_U32_MODIFY_VEL_PM,

	IDX_U32_TRANSPOSE_SHIFT,

	IDX_U32_ARPEGGIATOR_NOTES,

    IDX_U32_SETTINGS_FILTERED_CH,
    U32_COUNT   /* keep last */
};

enum {
    /* u8 fields */
    IDX_U8_TEMPO_CURRENTLY_SENDING = 0,
    IDX_U8_TEMPO_SEND_TO_OUT,

    IDX_U8_MODIFY_CHANGE_OR_SPLIT,
    IDX_U8_MODIFY_VELOCITY_TYPE,
    IDX_U8_MODIFY_SEND_TO_OUT,
    IDX_U8_MODIFY_SEND_CH1,
    IDX_U8_MODIFY_SEND_CH2,
    IDX_U8_MODIFY_SPLIT_NOTE,
    IDX_U8_MODIFY_SPLIT_CH1,
    IDX_U8_MODIFY_SPLIT_CH2,
    IDX_U8_MODIFY_VEL_ABS,
    IDX_U8_MODIFY_SENDING,

    IDX_U8_TRANSPOSE_TYPE,
    IDX_U8_TRANSPOSE_BASE_NOTE,
    IDX_U8_TRANSPOSE_INTERVAL,
    IDX_U8_TRANSPOSE_SCALE,
    IDX_U8_TRANSPOSE_SEND_ORIGINAL,
    IDX_U8_TRANSPOSE_SENDING,

	IDX_U8_ARPEGGIATOR_CURRENTLY_SENDING,
	IDX_U8_ARPEGGIATOR_INTERVAL,
	IDX_U8_ARPEGGIATOR_PATTERN,
	IDX_U8_ARPEGGIATOR_GATE,
	IDX_U8_ARPEGGIATOR_OCTAVES,
	IDX_U8_ARPEGGIATOR_LATCH,
	IDX_U8_ARPEGGIATOR_SWING,
	IDX_U8_ARPEGGIATOR_RETRIGGER,
	IDX_U8_ARPEGGIATOR_LENGTH,

    IDX_U8_SETTINGS_START_MENU,
    IDX_U8_SETTINGS_SEND_USB,
    IDX_U8_SETTINGS_BRIGHTNESS,
    IDX_U8_SETTINGS_CHANNEL_FILTER,
    IDX_U8_SETTINGS_MIDI_THRU,
    IDX_U8_SETTINGS_USB_THRU,

    U8_COUNT    /* keep last */
};

/* Private on-flash/in-RAM structure */
typedef struct {
    uint32_t check_data_validity;     /* first for easy validation */
    int32_t  u32_vals[U32_COUNT];
    uint8_t  u8_vals[U8_COUNT];
} save_struct;

/* --------------------------------------------------------------------------
   Module state
   -------------------------------------------------------------------------- */

static save_struct save_data;

/* Exported pointer tables (declared extern in memory_main.h) */
int32_t* u32_fields[SAVE_FIELD_COUNT] = {0};
uint8_t* u8_fields [SAVE_FIELD_COUNT] = {0};

/* --------------------------------------------------------------------------
   Internal helpers
   -------------------------------------------------------------------------- */

/* Bind pointer tables to a given save_struct instance */
static void bind_field_pointers(save_struct* s, int32_t** u32tab, uint8_t** u8tab)
{
    /* Initialize all to NULL */
    for (int i = 0; i < SAVE_FIELD_COUNT; ++i) {
        u32tab[i] = NULL;
        u8tab[i]  = NULL;
    }

    /* ---- u32 mappings ---- */
    u32tab[TEMPO_CURRENT_TEMPO]        = &s->u32_vals[IDX_U32_TEMPO_CURRENT_TEMPO];
    u32tab[TEMPO_TEMPO_CLICK_RATE]     = &s->u32_vals[IDX_U32_TEMPO_CLICK_RATE];

    u32tab[MODIFY_VEL_PLUS_MINUS] = &s->u32_vals[IDX_U32_MODIFY_VEL_PM];

    u32tab[TRANSPOSE_MIDI_SHIFT_VALUE] = &s->u32_vals[IDX_U32_TRANSPOSE_SHIFT];

    u32tab[ARPEGGIATOR_NOTES] = &s->u32_vals[IDX_U32_ARPEGGIATOR_NOTES];

    u32tab[SETTINGS_FILTERED_CH]      = &s->u32_vals[IDX_U32_SETTINGS_FILTERED_CH];

    /* CHECKSUM exposed via u32 path for readback tools */
    u32tab[SAVE_DATA_VALIDITY]              = (int32_t*)&s->check_data_validity;

    /* ---- u8 mappings ---- */
    u8tab[TEMPO_CURRENTLY_SENDING]     = &s->u8_vals[IDX_U8_TEMPO_CURRENTLY_SENDING];
    u8tab[TEMPO_SEND_TO_MIDI_OUT]      = &s->u8_vals[IDX_U8_TEMPO_SEND_TO_OUT];

    u8tab[MODIFY_CHANGE_OR_SPLIT]      = &s->u8_vals[IDX_U8_MODIFY_CHANGE_OR_SPLIT];
    u8tab[MODIFY_VELOCITY_TYPE]        = &s->u8_vals[IDX_U8_MODIFY_VELOCITY_TYPE];
    u8tab[MODIFY_SEND_TO_MIDI_CH1]=&s->u8_vals[IDX_U8_MODIFY_SEND_CH1];
    u8tab[MODIFY_SEND_TO_MIDI_CH2]=&s->u8_vals[IDX_U8_MODIFY_SEND_CH2];
    u8tab[MODIFY_SPLIT_NOTE]           = &s->u8_vals[IDX_U8_MODIFY_SPLIT_NOTE];
    u8tab[MODIFY_SEND_TO_MIDI_OUT]     = &s->u8_vals[IDX_U8_MODIFY_SEND_TO_OUT];
    u8tab[MODIFY_SPLIT_MIDI_CH1] = &s->u8_vals[IDX_U8_MODIFY_SPLIT_CH1];
    u8tab[MODIFY_SPLIT_MIDI_CH2] = &s->u8_vals[IDX_U8_MODIFY_SPLIT_CH2];
    u8tab[MODIFY_VEL_ABSOLUTE]    = &s->u8_vals[IDX_U8_MODIFY_VEL_ABS];
    u8tab[MODIFY_CURRENTLY_SENDING]    = &s->u8_vals[IDX_U8_MODIFY_SENDING];

    u8tab[TRANSPOSE_TRANSPOSE_TYPE]    = &s->u8_vals[IDX_U8_TRANSPOSE_TYPE];
    u8tab[TRANSPOSE_BASE_NOTE]         = &s->u8_vals[IDX_U8_TRANSPOSE_BASE_NOTE];
    u8tab[TRANSPOSE_INTERVAL]          = &s->u8_vals[IDX_U8_TRANSPOSE_INTERVAL];
    u8tab[TRANSPOSE_TRANSPOSE_SCALE]   = &s->u8_vals[IDX_U8_TRANSPOSE_SCALE];
    u8tab[TRANSPOSE_SEND_ORIGINAL]     = &s->u8_vals[IDX_U8_TRANSPOSE_SEND_ORIGINAL];
    u8tab[TRANSPOSE_CURRENTLY_SENDING] = &s->u8_vals[IDX_U8_TRANSPOSE_SENDING];

    u8tab[ARPEGGIATOR_CURRENTLY_SENDING] = &s->u8_vals[IDX_U8_ARPEGGIATOR_CURRENTLY_SENDING];
    u8tab[ARPEGGIATOR_INTERVAL] = &s->u8_vals[IDX_U8_ARPEGGIATOR_INTERVAL];
    u8tab[ARPEGGIATOR_PATTERN] = &s->u8_vals[IDX_U8_ARPEGGIATOR_PATTERN];
    u8tab[ARPEGGIATOR_GATE] = &s->u8_vals[IDX_U8_ARPEGGIATOR_GATE];
    u8tab[ARPEGGIATOR_OCTAVES] = &s->u8_vals[IDX_U8_ARPEGGIATOR_OCTAVES];
    u8tab[ARPEGGIATOR_HOLD] = &s->u8_vals[IDX_U8_ARPEGGIATOR_LATCH];
    u8tab[ARPEGGIATOR_SWING] = &s->u8_vals[IDX_U8_ARPEGGIATOR_SWING];
    u8tab[ARPEGGIATOR_RETRIGGER] = &s->u8_vals[IDX_U8_ARPEGGIATOR_RETRIGGER];
    u8tab[ARPEGGIATOR_LENGTH] = &s->u8_vals[IDX_U8_ARPEGGIATOR_LENGTH];



    u8tab[SETTINGS_START_MENU]              = &s->u8_vals[IDX_U8_SETTINGS_START_MENU];
    u8tab[SETTINGS_SEND_USB]                = &s->u8_vals[IDX_U8_SETTINGS_SEND_USB];
    u8tab[SETTINGS_BRIGHTNESS]              = &s->u8_vals[IDX_U8_SETTINGS_BRIGHTNESS];
    u8tab[SETTINGS_CHANNEL_FILTER]          = &s->u8_vals[IDX_U8_SETTINGS_CHANNEL_FILTER];
    u8tab[SETTINGS_MIDI_THRU]               = &s->u8_vals[IDX_U8_SETTINGS_MIDI_THRU];
    u8tab[SETTINGS_USB_THRU]                = &s->u8_vals[IDX_U8_SETTINGS_USB_THRU];
}

/* Bind the module-global pointer tables to save_data */
static void save_init_field_pointers(void)
{
    bind_field_pointers(&save_data, u32_fields, u8_fields);
}

/* Build a default-initialized save_struct using your parameter table */
static save_struct make_default_settings(void)
{
    save_struct s;
    memset(&s, 0, sizeof(s));

    /* temporary local pointer tables bound to 's' */
    int32_t* t_u32[SAVE_FIELD_COUNT];
    uint8_t* t_u8 [SAVE_FIELD_COUNT];
    bind_field_pointers(&s, t_u32, t_u8);

    for (int f = 0; f < SAVE_FIELD_COUNT; ++f) {
        int32_t def = save_limits[f].def;   /* relies on menu.h */
        if (t_u32[f]) {
            *t_u32[f] = def;
        } else if (t_u8[f]) {
            *t_u8[f]  = (uint8_t)def;
        } else {
            /* no storage backing for e.g. SETTINGS_ABOUT etc. -> ignore */
        }
    }

    s.check_data_validity = DATA_VALIDITY_CHECKSUM;
    return s;
}

/* Flash reader as const (do not mutate flash via this pointer) */
static inline const save_struct* read_setting_memory(void)
{
    return (const save_struct*)FLASH_SECTOR7_ADDR;
}

/* --------------------------------------------------------------------------
   Public API
   -------------------------------------------------------------------------- */

HAL_StatusTypeDef store_settings(void)
{
    if (!save_lock_with_retries()) return HAL_ERROR;

    save_struct local = save_data;   /* atomic snapshot under lock */
    save_unlock();

    local.check_data_validity = DATA_VALIDITY_CHECKSUM;

    HAL_StatusTypeDef status;
    uint32_t error_status = 0;

    HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector       = FLASH_SECTOR_7;
    erase.NbSectors    = 1;

    status = HAL_FLASHEx_Erase(&erase, &error_status);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }

    const uint32_t* p    = (const uint32_t*)&local;
    const uint32_t  words = (sizeof(save_struct) + 3u) / 4u;

    for (uint32_t i = 0; i < words; ++i) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                   FLASH_SECTOR7_ADDR + i * 4u,
                                   p[i]);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

void save_load_from_flash(void)
{
    const save_struct* flash_ptr = read_setting_memory();

    if (flash_ptr->check_data_validity == DATA_VALIDITY_CHECKSUM) {
        save_data = *flash_ptr;    /* copy from flash */
    } else {
        save_data = make_default_settings();
    }

    save_init_field_pointers();
    save_mark_all_changed();
}

/* --------------------------------------------------------------------------
   Unit test helpers
   -------------------------------------------------------------------------- */
#ifdef UNIT_TEST
void memory_init_defaults(void)
{
    save_data = make_default_settings();
    save_init_field_pointers();
    save_mark_all_changed();
}

void memory_set_midi_thru(uint8_t v)
{
    (void)save_modify_u8(SETTINGS_MIDI_THRU, SAVE_MODIFY_SET, v ? 1u : 0u);
}

#endif

