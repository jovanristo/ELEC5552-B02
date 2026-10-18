#include "display.h"
#include "app.h"
#include "inputs.h"
#include "control.h"
#include "protection.h"
#include "timers.h"
#include "user_limits.h"

#include "mx_spi3.h"
#include "mx_gpio_default.h"

#include "stm32_hal.h"
#include "lvgl.h"

#define LVGL_BUFFER_LINES   20U

/* ILI9488 commands */
#define ILI9488_SWRESET     0x01U
#define ILI9488_SLPOUT      0x11U
#define ILI9488_DISPON      0x29U
#define ILI9488_CASET       0x2AU
#define ILI9488_PASET       0x2BU
#define ILI9488_RAMWR       0x2CU
#define ILI9488_MADCTL      0x36U
#define ILI9488_COLMOD      0x3AU


static hal_spi_handle_t *display_spi = NULL;
static lv_obj_t *dut_label = NULL;
static lv_obj_t *hint_label = NULL;
static uint32_t timer_entry = 0U;
static bool timer_editing = false;

/*
 * LVGL renders in RGB565.
 * 480 pixels × 20 rows × 2 bytes.
 */
static uint16_t lvgl_draw_buffer[
    DISPLAY_WIDTH * LVGL_BUFFER_LINES
];

/*
 * ILI9488 SPI pixel data is RGB666:
 * 3 bytes per pixel.
 *
 * One complete row:
 * 480 × 3 = 1440 bytes.
 */
static uint8_t spi_line_buffer[
    DISPLAY_WIDTH * 3U
];


static bool Display_SendCommand(uint8_t command);
static bool Display_SendData(const uint8_t *data, uint32_t length);

static void Display_HardwareReset(void);

static bool Display_SetAddressWindow(uint16_t x0,
                                     uint16_t y0,
                                     uint16_t x1,
                                     uint16_t y1);

static void Display_Flush(lv_display_t *display,
                          const lv_area_t *area,
                          uint8_t *px_map);


/* =========================================================
 * Display initialisation
 * ========================================================= */

bool Display_Init(void)
{
    display_spi = mx_spi3_gethandle();

    if (display_spi == NULL)
    {
        return false;
    }


    /* LVGL is initialised once by main.c before Display_Init(). */

    /* ---------- Initialise ILI9488 ---------- */

    Display_HardwareReset();


    if (!Display_SendCommand(ILI9488_SWRESET))
    {
        return false;
    }

    HAL_Delay(120U);


    if (!Display_SendCommand(ILI9488_SLPOUT))
    {
        return false;
    }

    HAL_Delay(120U);


    /*
     * Landscape orientation.
     */
    uint8_t madctl = 0x28U;

    if (!Display_SendCommand(ILI9488_MADCTL))
    {
        return false;
    }

    if (!Display_SendData(&madctl, 1U))
    {
        return false;
    }


    /*
     * ILI9488 SPI mode:
     * RGB666 / 18-bit colour.
     */
    uint8_t pixel_format = 0x66U;

    if (!Display_SendCommand(ILI9488_COLMOD))
    {
        return false;
    }

    if (!Display_SendData(&pixel_format, 1U))
    {
        return false;
    }


    if (!Display_SendCommand(ILI9488_DISPON))
    {
        return false;
    }

    HAL_Delay(20U);


    /* ---------- Create LVGL display ---------- */

    lv_display_t *display =
        lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    if (display == NULL)
    {
        return false;
    }


    /*
     * LVGL internally renders pixels as RGB565.
     *
     * Our flush callback converts those pixels to RGB666
     * before sending them to the ILI9488.
     */
    lv_display_set_color_format(
        display,
        LV_COLOR_FORMAT_RGB565
    );


    lv_display_set_buffers(
        display,
        lvgl_draw_buffer,
        NULL,
        sizeof(lvgl_draw_buffer),
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );


    lv_display_set_flush_cb(
        display,
        Display_Flush
    );

    lv_display_set_default(display);

    /* ---------- Simple LVGL test screen ---------- */

    lv_obj_t *screen = lv_screen_active();

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x101010),
        0
    );
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);


    /* Screen title */
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "DC ELECTRONIC LOAD");
    lv_obj_set_style_text_color(
        title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 35);

    /* DUT connection status */
    dut_label = lv_label_create(screen);
    lv_label_set_text(dut_label, "CONNECT DUT");
    lv_obj_set_style_text_color(
        dut_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(dut_label);

    /* User instructions */
    hint_label = lv_label_create(screen);
    lv_label_set_text(hint_label, "Waiting for DUT voltage...");
    lv_obj_set_style_text_color(
        hint_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -40);


    return true;
}

void Display_Update(void)
{
    static uint32_t last_update_ms = 0U;
    static uint32_t last_press_ms = 0U;

    /* Pages: 0-5 setup, 6 running, 7 finished, 8 fault */
    static uint8_t page = 0U;
    static AppMode_t pending_mode = APP_MODE_NONE;
    /* Preserve the original fault cause even after sensors recover. */
    static ProtectionStatus_t fault_snapshot;
    static bool fault_clear_timing = false;
    static uint32_t fault_clear_since_ms = 0U;

    static bool last_ok = false;
    static bool last_1 = false;
    static bool last_2 = false;
    static bool last_left = false;

    static uint32_t current_entry = 0U;
    static uint32_t voltage_entry = 0U;
    static bool last_digits[10] = {false};
    static bool last_right = false;
    static bool last_up = false;
    static bool last_down = false;

    uint32_t now_ms = HAL_GetTick();

    AppState_t *app = App_GetState();
    const Inputs_t *in = Inputs_Get();

    if (app == NULL || in == NULL ||
        dut_label == NULL || hint_label == NULL)
    {
        return;
    }

    const ProtectionStatus_t *protection = Protection_GetStatus();

    /* Faults take priority over every menu page, including normal STOP. */
    if (protection != NULL && protection->fault_latched && page != 8U)
    {
        fault_snapshot = *protection;
        Control_Stop();                  /* Clamp MOSFETs and stop timer. */
        page = 8U;
        fault_clear_timing = false;
        last_press_ms = now_ms;
    }

    /* Require stable, fault-free readings before allowing acknowledgement. */
    if (page == 8U)
    {
        if (protection != NULL &&
            protection->fault_latched &&
            protection->fault == PROTECTION_OK)
        {
            if (!fault_clear_timing)
            {
                fault_clear_timing = true;
                fault_clear_since_ms = now_ms;
            }
        }
        else
        {
            fault_clear_timing = false;
        }
    }

    /* Detect new button presses */
    bool ok = in->button_ok && !last_ok;
    bool key1 = in->button_1 && !last_1;
    bool key2 = in->button_2 && !last_2;
    bool left = in->button_left && !last_left;

    last_ok = in->button_ok;
    last_1 = in->button_1;
    last_2 = in->button_2;
    last_left = in->button_left;

    /* Detect newly pressed numerical key */
    const bool keys[10] = {
        in->button_0, in->button_1,
        in->button_2, in->button_3,
        in->button_4, in->button_5,
        in->button_6, in->button_7,
        in->button_8, in->button_9
    };

    int digit = -1;

    for (uint32_t i = 0U; i < 10U; i++)
    {
        if (keys[i] && !last_digits[i])
        {
            digit = (int)i;
        }

        last_digits[i] = keys[i];
    }

    bool right = in->button_right && !last_right;
    last_right = in->button_right;

    bool up = in->button_up && !last_up;
    bool down = in->button_down && !last_down;

    last_up = in->button_up;
    last_down = in->button_down;

    /* STOP must not be blocked by keypad debounce */
    if (page == 6U && ok)
    {
        Control_Stop();
        page = 7U;
        last_press_ms = now_ms;
    }

    /* Basic debounce */
    if ((uint32_t)(now_ms - last_press_ms) >= 120U)
    {
        if (page == 0U)
        {
            if (app->dut_connected && ok)
            {
                page = 1U;
                last_press_ms = now_ms;
            }
        }
        else if (page == 1U)
        {
            if (key1)
            {
                pending_mode = APP_MODE_CC;
                last_press_ms = now_ms;
            }
            else if (key2)
            {
                pending_mode = APP_MODE_CV;
                last_press_ms = now_ms;
            }
            else if (left)
            {
                page = 0U;
                last_press_ms = now_ms;
            }
            else if (ok && pending_mode != APP_MODE_NONE)
            {
                if (App_SelectMode(pending_mode))
                {
                    current_entry = 0U;
                    voltage_entry = 0U;

                    /* CC needs current entry, CV goes directly to voltage */
                    page = (pending_mode == APP_MODE_CC) ? 2U : 3U;
                }
                last_press_ms = now_ms;
            }
        }
        else if (page == 2U)
        {
            if (left)
            {
                page = 1U;
                last_press_ms = now_ms;
            }
            else if (app->selected_mode == APP_MODE_CC)
            {
                if (right)
                {
                    /* Clear entered current */
                    current_entry = 0U;
                    app->current_setpoint_a = 0.0f;
                    last_press_ms = now_ms;
                }
                else if (digit >= 0)
                {
                    /* Append digit in units of 0.01 A */
                    uint32_t next = current_entry * 10U +
                                    (uint32_t)digit;

                    uint32_t max_entry =
                        (uint32_t)(app->max_current_a * 100.0f + 0.5f);

                    if (next <= max_entry)
                    {
                        current_entry = next;
                        app->current_setpoint_a =
                            (float)current_entry / 100.0f;
                    }

                    last_press_ms = now_ms;
                }
                else if (ok && current_entry > 0U)
                {
                    page = 3U; /* Voltage settings next */
                    last_press_ms = now_ms;
                }
            }

    }

    else if (page == 3U)
    {
        if (left)
        {
            page = (app->selected_mode == APP_MODE_CC) ? 2U : 1U;
            last_press_ms = now_ms;
        }
        else if (right)
        {
            voltage_entry = 0U;
            app->voltage_setpoint_v = 0.0f;
            last_press_ms = now_ms;
        }
        else if (digit >= 0)
        {
            /* Voltage entered in units of 0.1 V */
            uint32_t next = voltage_entry * 10U +
                            (uint32_t)digit;

            uint32_t max_entry =
                (uint32_t)(app->max_voltage_v * 10.0f + 0.05f);

            if (next <= max_entry)
            {
                voltage_entry = next;
                app->voltage_setpoint_v =
                    (float)voltage_entry / 10.0f;
            }

            last_press_ms = now_ms;
        }
        else if (ok &&
                (app->selected_mode == APP_MODE_CC ||
                voltage_entry > 0U))
        {
            page = 4U;  /* Timer settings next */
            last_press_ms = now_ms;
        }
    }
    else if (page == 4U)
    {
        if (left)
        {
            page = 3U;
            last_press_ms = now_ms;
        }
        else if (right)
        {
            timer_entry = 0U;
            timer_editing = true;
            last_press_ms = now_ms;
        }
        else if (digit >= 0)
        {
            /* Enter time as HHMMSS */
            uint32_t next = timer_editing ?
                timer_entry * 10U + (uint32_t)digit :
                (uint32_t)digit;

            if (next <= 999999U)
            {
                timer_entry = next;
                timer_editing = true;
            }

            last_press_ms = now_ms;
        }
        else if (ok)
        {
            /* Keep existing duration if unchanged */
            bool valid = !timer_editing;

            if (timer_editing)
            {
                uint32_t hours = timer_entry / 10000U;
                uint32_t minutes = (timer_entry / 100U) % 100U;
                uint32_t seconds = timer_entry % 100U;

                valid = App_SetRunTime(hours, minutes, seconds);
            }

            if (valid)
            {
                page = 5U;  /* Review and START next */
                timer_editing = false;
            }

            last_press_ms = now_ms;
        }
    }

        else if (page == 5U)  /* Review / START */
    {
        if (left)
        {
            page = 4U;
            last_press_ms = now_ms;
        }
        else if (ok)
        {
            if (Control_Start())
            {
                page = 6U;
            }

            last_press_ms = now_ms;
        }
    }
    else if (page == 6U)  /* Running */
    {
        if (Control_GetState() == CONTROL_STATE_CC)
        {
            uint32_t max_entry =
                (uint32_t)(app->max_current_a * 100.0f + 0.5f);

            bool changed = false;

            if (up && current_entry < max_entry)
            {
                current_entry++;
                changed = true;
            }
            else if (down && current_entry > 1U)
            {
                current_entry--;
                changed = true;
            }

            if (changed)
            {
                app->current_setpoint_a =
                    (float)current_entry / 100.0f;

                last_press_ms = now_ms;
            }
        }
    }

    else if (page == 7U)  /* Finished: reset user settings */
    {
        if (ok)
        {
            page = 0U;
            pending_mode = APP_MODE_NONE;

            current_entry = 0U;
            voltage_entry = 0U;
            timer_entry = 0U;
            timer_editing = false;

            app->selected_mode = APP_MODE_NONE;
            app->current_setpoint_a = 0.0f;
            app->voltage_setpoint_v = 0.0f;
            app->run_time_ms = 60000U;

            last_press_ms = now_ms;
        }
    }
    else if (page == 8U)  /* Fault acknowledgement */
    {
        if (ok && protection != NULL &&
            protection->fault_latched &&
            fault_clear_timing &&
            (uint32_t)(now_ms - fault_clear_since_ms) >= 500U)
        {
            /* Does not restart the load; checked sensors are now normal. */
            Protection_Reset();
            page = 0U;
            pending_mode = APP_MODE_NONE;
            current_entry = 0U;
            voltage_entry = 0U;
            timer_entry = 0U;
            timer_editing = false;
            app->selected_mode = APP_MODE_NONE;
            app->current_setpoint_a = 0.0f;
            app->voltage_setpoint_v = 0.0f;
            app->run_time_ms = 60000U;
            fault_clear_timing = false;
            last_press_ms = now_ms;
        }
    }
    } /* End button debounce */

    /* Detect automatic completion or protective shutdown */
    if (page == 6U && !app->running)
    {
        page = 7U;
    }

    /* Refresh display every 200 ms */
    if ((uint32_t)(now_ms - last_update_ms) < 200U)
    {
        return;
    }

    last_update_ms = now_ms;

    /* Restore normal label colour after leaving a red fault screen. */
    if (page != 0U && page != 8U)
    {
        lv_obj_set_style_text_color(
            dut_label, lv_color_hex(0xFFFFFF), 0);
    }

    if (page == 0U)
    {
        if (app->dut_connected)
        {
            uint32_t v10 =
                (uint32_t)(app->dut_voltage_v * 10.0f + 0.5f);

            lv_label_set_text_fmt(
                dut_label, "DUT: %lu.%lu V",
                (unsigned long)(v10 / 10U),
                (unsigned long)(v10 % 10U));

            lv_obj_set_style_text_color(
                dut_label, lv_color_hex(0x00CC66), 0);

            lv_label_set_text(
                hint_label, "Press OK to continue");
        }
        else
        {
            lv_label_set_text(dut_label, "CONNECT DUT");
            lv_obj_set_style_text_color(
                dut_label, lv_color_hex(0xFFFFFF), 0);
            lv_label_set_text(
                hint_label, "Waiting for DUT voltage...");
        }
    }
    else if (page == 1U)
    {
        if (pending_mode == APP_MODE_CC)
        {
            lv_label_set_text(dut_label, "MODE: CC");
        }
        else if (pending_mode == APP_MODE_CV)
        {
            lv_label_set_text(dut_label, "MODE: CV");
        }
        else
        {
            lv_label_set_text(dut_label, "SELECT MODE");
        }

        lv_obj_set_style_text_color(
            dut_label, lv_color_hex(0xFFFFFF), 0);

        lv_label_set_text(
            hint_label, "1: CC  2: CV  OK: Confirm  LEFT: Back");
    }
    else if (page == 2U && app->selected_mode == APP_MODE_CC)
    {
        lv_label_set_text_fmt(
            dut_label,
            "SET CURRENT: %lu.%02lu A",
            (unsigned long)(current_entry / 100U),
            (unsigned long)(current_entry % 100U)
        );

        uint32_t max_entry =
            (uint32_t)(app->max_current_a * 100.0f + 0.5f);

        lv_label_set_text_fmt(
            hint_label,
            "MAX %lu.%02lu A  |  0500 = 5.00 A\n"
            "RIGHT: Clear  LEFT: Back  OK: Next",
            (unsigned long)(max_entry / 100U),
            (unsigned long)(max_entry % 100U)
        );
    }
    else if (page == 2U)
    {
        lv_label_set_text(dut_label, "CV VOLTAGE SETTINGS");
        lv_label_set_text(hint_label, "Next step");
    }
    
    else if (page == 3U)
    {
        uint32_t max_entry =
            (uint32_t)(app->max_voltage_v * 10.0f + 0.05f);

        if (app->selected_mode == APP_MODE_CC)
        {
            lv_label_set_text_fmt(
                dut_label,
                "CC CUTOFF: %lu.%lu V",
                (unsigned long)(voltage_entry / 10U),
                (unsigned long)(voltage_entry % 10U)
            );
        }
        else
        {
            lv_label_set_text_fmt(
                dut_label,
                "CV TARGET: %lu.%lu V",
                (unsigned long)(voltage_entry / 10U),
                (unsigned long)(voltage_entry % 10U)
            );
        }

        lv_label_set_text_fmt(
            hint_label,
            "MAX %lu.%lu V  |  200 = 20.0 V\n"
            "RIGHT: Clear  LEFT: Back  OK: Next",
            (unsigned long)(max_entry / 10U),
            (unsigned long)(max_entry % 10U)
        );
    }
    else if (page == 4U)
    {
        uint32_t hours, minutes, seconds;

        if (timer_editing)
        {
            hours = timer_entry / 10000U;
            minutes = (timer_entry / 100U) % 100U;
            seconds = timer_entry % 100U;
        }
        else
        {
            uint32_t total = app->run_time_ms / 1000U;

            hours = total / 3600U;
            minutes = (total % 3600U) / 60U;
            seconds = total % 60U;
        }

        lv_label_set_text_fmt(
            dut_label,
            "RUN TIME: %02lu:%02lu:%02lu",
            (unsigned long)hours,
            (unsigned long)minutes,
            (unsigned long)seconds
        );

        lv_label_set_text(
            hint_label,
            "Enter HHMMSS (max 24h)\n"
            "RIGHT: Clear  LEFT: Back  OK: Next"
        );
    }

    else if (page == 5U)  /* Review */
    {
        uint32_t voltage10 =
            (uint32_t)(app->dut_voltage_v * 10.0f + 0.5f);

        uint32_t time_s = app->run_time_ms / 1000U;

        if (app->selected_mode == APP_MODE_CC)
        {
            uint32_t current100 =
                (uint32_t)(app->current_setpoint_a * 100.0f + 0.5f);

            uint32_t cutoff10 =
                (uint32_t)(app->voltage_setpoint_v * 10.0f + 0.5f);

            uint32_t power =
                (uint32_t)(app->dut_voltage_v *
                        app->current_setpoint_a + 0.5f);

            lv_label_set_text_fmt(
                dut_label,
                "MODE: CC\n"
                "DUT: %lu.%lu V\n"
                "CURRENT: %lu.%02lu A\n"
                "CUTOFF: %lu.%lu V\n"
                "EST. POWER: %lu W\n"
                "TIME: %02lu:%02lu:%02lu",
                (unsigned long)(voltage10 / 10U),
                (unsigned long)(voltage10 % 10U),
                (unsigned long)(current100 / 100U),
                (unsigned long)(current100 % 100U),
                (unsigned long)(cutoff10 / 10U),
                (unsigned long)(cutoff10 % 10U),
                (unsigned long)power,
                (unsigned long)(time_s / 3600U),
                (unsigned long)((time_s % 3600U) / 60U),
                (unsigned long)(time_s % 60U)
            );
        }
        else
        {
            uint32_t target10 =
                (uint32_t)(app->voltage_setpoint_v * 10.0f + 0.5f);

            lv_label_set_text_fmt(
                dut_label,
                "MODE: CV\n"
                "DUT: %lu.%lu V\n"
                "TARGET: %lu.%lu V\n"
                "CURRENT: -\n"
                "POWER LIMIT: 500 W\n"
                "TIME: %02lu:%02lu:%02lu",
                (unsigned long)(voltage10 / 10U),
                (unsigned long)(voltage10 % 10U),
                (unsigned long)(target10 / 10U),
                (unsigned long)(target10 % 10U),
                (unsigned long)(time_s / 3600U),
                (unsigned long)((time_s % 3600U) / 60U),
                (unsigned long)(time_s % 60U)
            );
        }

        lv_label_set_text(
            hint_label,
            "OK: START    LEFT: BACK"
        );
    }
    else if (page == 6U)  /* Running */
    {
        uint32_t v10 =
            (uint32_t)(app->dut_voltage_v * 10.0f + 0.5f);

        uint32_t i100 =
            (uint32_t)(app->total_current_a * 100.0f + 0.5f);

        uint32_t power =
            (uint32_t)(app->power_w + 0.5f);

        TimerHMS_t remaining = Timers_RunTimerGetRemainingHMS();

        float max_internal = 0.0f;
        float max_external = 0.0f;
        bool internal_valid = false;
        bool external_valid = false;

        for (uint32_t i = 0U; i < NUM_THERMISTORS; i++)
        {
            if (!in->thermistor_valid[i])
                continue;

            float temp = app->temperature_c[i];

            /* External thermistors: 4-7 (array indexes 3-6) */
            if (i >= 3U && i <= 6U)
            {
                if (!external_valid || temp > max_external)
                    max_external = temp;

                external_valid = true;
            }
            else
            {
                if (!internal_valid || temp > max_internal)
                    max_internal = temp;

                internal_valid = true;
            }
        }

        /* Show -- if no valid temperature measurement exists */
        char internal_text[12] = "--";
        char external_text[12] = "--";

        if (internal_valid)
        {
            lv_snprintf(internal_text, sizeof(internal_text),
                        "%ld", (long)(max_internal +
                        (max_internal >= 0.0f ? 0.5f : -0.5f)));
        }

        if (external_valid)
        {
            lv_snprintf(external_text, sizeof(external_text),
                        "%ld", (long)(max_external +
                        (max_external >= 0.0f ? 0.5f : -0.5f)));
        }

        lv_label_set_text_fmt(
            dut_label,
            "%s RUNNING\n"
            "VOLTAGE: %lu.%lu V\n"
            "CURRENT: %lu.%02lu A\n"
            "POWER: %lu W\n"
            "TIME LEFT: %02lu:%02lu:%02lu\n"
            "FAN: %lu RPM\n"
            "TEMP INT: %s C   EXT: %s C",
            app->selected_mode == APP_MODE_CC ? "CC" : "CV",
            (unsigned long)(v10 / 10U),
            (unsigned long)(v10 % 10U),
            (unsigned long)(i100 / 100U),
            (unsigned long)(i100 % 100U),
            (unsigned long)power,
            (unsigned long)remaining.hours,
            (unsigned long)remaining.minutes,
            (unsigned long)remaining.seconds,
            (unsigned long)app->fan_rpm,
            internal_text,
            external_text
        );

        if (Control_GetState() == CONTROL_STATE_CC)
        {
            lv_label_set_text_fmt(
                hint_label,
                "SET: %lu.%02lu A   UP/DOWN: 0.01 A   OK: STOP",
                (unsigned long)(current_entry / 100U),
                (unsigned long)(current_entry % 100U)
            );
        }
        else
        {
            lv_label_set_text(hint_label, "OK: STOP");
        }

    }
    else if (page == 7U)  /* Finished */
    {
        /* Show the reason if the user band (REQ-10) stopped the run. */
        UserLimitResult_t band = UserLimits_GetLastResult();

        if (band == USER_LIMIT_OK)
        {
            lv_label_set_text(
                dut_label,
                "RUN FINISHED\nLOAD DISABLED"
            );
        }
        else
        {
            const char *reason =
                (band == USER_LIMIT_BELOW_BAND) ? "BELOW SET RANGE" :
                (band == USER_LIMIT_ABOVE_BAND) ? "ABOVE SET RANGE" :
                                                  "DID NOT REACH SETPOINT";

            lv_label_set_text_fmt(
                dut_label,
                "RUN STOPPED\n%s\nLOAD DISABLED",
                reason
            );
        }

        lv_label_set_text(
            hint_label,
            "OK: RESET"
        );
    }
    else if (page == 8U)  /* Latched protection fault */
    {
        lv_obj_set_style_text_color(
            dut_label, lv_color_hex(0xFF4444), 0);

        if (fault_snapshot.fault == PROTECTION_INTERNAL_TEMP_FAULT)
        {
            uint32_t probe = fault_snapshot.internal_temp.probe_index;
            probe = (probe < 3U) ? (probe + 1U) : (probe + 5U);
            const char *reason =
                (fault_snapshot.internal_temp.fault == INTERNAL_TEMP_OVER_LIMIT)
                ? "OVER TEMPERATURE" : "INVALID SENSOR";
            lv_label_set_text_fmt(dut_label,
                "PROTECTION FAULT\nHEATSINK TEMP\nPROBE %lu: %s\nLOAD DISABLED",
                (unsigned long)probe, reason);
        }
        else if (fault_snapshot.fault == PROTECTION_EXTERNAL_TEMP_FAULT)
        {
            uint32_t probe =
                (uint32_t)fault_snapshot.external_temp.probe_index + 4U;
            const char *reason =
                (fault_snapshot.external_temp.fault == BATTERY_TEMP_OVER_LIMIT)
                ? "OVER TEMPERATURE" : "INVALID SENSOR";
            lv_label_set_text_fmt(dut_label,
                "PROTECTION FAULT\nDUT TEMPERATURE\nPROBE %lu: %s\nLOAD DISABLED",
                (unsigned long)probe, reason);
        }
        else if (fault_snapshot.fault == PROTECTION_CURRENT_FAULT)
        {
            if (fault_snapshot.current.fault == CURRENT_OVER_LIMIT)
            {
                lv_label_set_text(dut_label,
                    "PROTECTION FAULT\nOVERCURRENT\nLOAD DISABLED");
            }
            else
            {
                lv_label_set_text_fmt(dut_label,
                    "PROTECTION FAULT\nCURRENT SENSOR %lu\nINVALID READING\nLOAD DISABLED",
                    (unsigned long)fault_snapshot.current.channel_index + 1UL);
            }
        }
        else if (fault_snapshot.fault == PROTECTION_VOLTAGE_FAULT)
        {
            const char *reason =
                (fault_snapshot.voltage == VOLTAGE_OVER_LIMIT)
                ? "OVERVOLTAGE" : "INVALID VOLTAGE";
            lv_label_set_text_fmt(dut_label,
                "PROTECTION FAULT\n%s\nLOAD DISABLED", reason);
        }
        else
        {
            lv_label_set_text(dut_label,
                "PROTECTION FAULT\nLOAD DISABLED");
        }

        if (fault_clear_timing &&
            (uint32_t)(now_ms - fault_clear_since_ms) >= 500U)
        {
            lv_label_set_text(hint_label, "OK: RESET AFTER CHECKS");
        }
        else
        {
            lv_label_set_text(hint_label,
                "FAULT ACTIVE - CHECK DUT / SENSORS");
        }
    }
}

/* =========================================================
 * LVGL flush callback
 * ========================================================= */

static void Display_Flush(lv_display_t *display,
                          const lv_area_t *area,
                          uint8_t *px_map)
{
    uint16_t width =
        (uint16_t)(area->x2 - area->x1 + 1);

    uint16_t height =
        (uint16_t)(area->y2 - area->y1 + 1);


    /*
     * Tell ILI9488 which area LVGL is updating.
     */
    if (!Display_SetAddressWindow(
            (uint16_t)area->x1,
            (uint16_t)area->y1,
            (uint16_t)area->x2,
            (uint16_t)area->y2))
    {
        lv_display_flush_ready(display);
        return;
    }


    const uint16_t *pixels =
        (const uint16_t *)px_map;


    for (uint16_t y = 0U; y < height; y++)
    {
        for (uint16_t x = 0U; x < width; x++)
        {
            uint16_t rgb565 =
                pixels[(y * width) + x];


            /* Extract RGB565 */
            uint8_t red5 =
                (uint8_t)((rgb565 >> 11) & 0x1FU);

            uint8_t green6 =
                (uint8_t)((rgb565 >> 5) & 0x3FU);

            uint8_t blue5 =
                (uint8_t)(rgb565 & 0x1FU);


            /*
             * Convert to 8-bit values.
             * ILI9488 uses upper 6 bits in RGB666 mode.
             */
            uint8_t red =
                (uint8_t)((red5 << 3) |
                          (red5 >> 2));

            uint8_t green =
                (uint8_t)((green6 << 2) |
                          (green6 >> 4));

            uint8_t blue =
                (uint8_t)((blue5 << 3) |
                          (blue5 >> 2));


            red   &= 0xFCU;
            green &= 0xFCU;
            blue  &= 0xFCU;


            uint32_t index = x * 3U;

            spi_line_buffer[index + 0U] = red;
            spi_line_buffer[index + 1U] = green;
            spi_line_buffer[index + 2U] = blue;
        }


        if (!Display_SendData(
                spi_line_buffer,
                (uint32_t)width * 3U))
        {
            lv_display_flush_ready(display);
            return;
        }
    }


    /*
     * Tell LVGL the pixels have finished transmitting.
     */
    lv_display_flush_ready(display);
}


/* =========================================================
 * ILI9488 address window
 * ========================================================= */

static bool Display_SetAddressWindow(uint16_t x0,
                                     uint16_t y0,
                                     uint16_t x1,
                                     uint16_t y1)
{
    uint8_t data[4];


    /* X coordinates */

    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)(x0 & 0xFFU);
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)(x1 & 0xFFU);

    if (!Display_SendCommand(ILI9488_CASET))
    {
        return false;
    }

    if (!Display_SendData(data, 4U))
    {
        return false;
    }


    /* Y coordinates */

    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)(y0 & 0xFFU);
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)(y1 & 0xFFU);

    if (!Display_SendCommand(ILI9488_PASET))
    {
        return false;
    }

    if (!Display_SendData(data, 4U))
    {
        return false;
    }


    /* Begin pixel transmission */

    if (!Display_SendCommand(ILI9488_RAMWR))
    {
        return false;
    }


    return true;
}


/* =========================================================
 * Send ILI9488 command
 * ========================================================= */

static bool Display_SendCommand(uint8_t command)
{
    /*
     * DC LOW = command.
     */
    HAL_GPIO_WritePin(
        LCD_DC_PORT,
        LCD_DC_PIN,
        HAL_GPIO_PIN_RESET
    );


    if (HAL_SPI_Transmit(
            display_spi,
            &command,
            1U,
            100U) != HAL_OK)
    {
        return false;
    }


    return true;
}


/* =========================================================
 * Send ILI9488 data
 * ========================================================= */

static bool Display_SendData(const uint8_t *data,
                             uint32_t length)
{
    if ((data == NULL) ||
        (length == 0U))
    {
        return false;
    }


    /*
     * DC HIGH = data.
     */
    HAL_GPIO_WritePin(
        LCD_DC_PORT,
        LCD_DC_PIN,
        HAL_GPIO_PIN_SET
    );


    if (HAL_SPI_Transmit(
            display_spi,
            data,
            length,
            1000U) != HAL_OK)
    {
        return false;
    }


    return true;
}


/* =========================================================
 * Hardware reset
 * ========================================================= */

static void Display_HardwareReset(void)
{
    HAL_GPIO_WritePin(
        LCD_RST_PORT,
        LCD_RST_PIN,
        HAL_GPIO_PIN_RESET
    );

    HAL_Delay(20U);


    HAL_GPIO_WritePin(
        LCD_RST_PORT,
        LCD_RST_PIN,
        HAL_GPIO_PIN_SET
    );

    HAL_Delay(120U);
}