#include "display.h"

#include "mx_spi3.h"
#include "mx_gpio_default.h"

#include "stm32_hal.h"
#include "lvgl.h"

#define DISPLAY_WIDTH       480U
#define DISPLAY_HEIGHT      320U
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


    /* Initialise LVGL */
    lv_init();

    /*
     * Let LVGL use the STM32 HAL millisecond tick.
     */
    lv_tick_set_cb(HAL_GetTick);


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


    /* ---------- Simple LVGL test screen ---------- */

    lv_obj_t *screen = lv_screen_active();

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x101010),
        0
    );


    lv_obj_t *label = lv_label_create(screen);

    lv_label_set_text(
        label,
        "LVGL WORKING"
    );

    lv_obj_set_style_text_color(
        label,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_obj_center(label);


    return true;
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