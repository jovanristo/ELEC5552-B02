#include "display.h"

#include "mx_spi3.h"
#include "mx_gpio_default.h"

#include "stm32_hal.h"


/* =========================================================
 * ILI9488 commands
 * ========================================================= */

#define ILI9488_SWRESET     0x01U
#define ILI9488_SLPOUT      0x11U
#define ILI9488_DISPON      0x29U

#define ILI9488_CASET       0x2AU
#define ILI9488_PASET       0x2BU
#define ILI9488_RAMWR       0x2CU

#define ILI9488_MADCTL      0x36U
#define ILI9488_COLMOD      0x3AU


/* =========================================================
 * SPI
 * ========================================================= */

static hal_spi_handle_t *display_spi = NULL;


/*
 * One complete display row.
 *
 * 480 pixels × 3 bytes per RGB666 pixel = 1440 bytes.
 */
static uint8_t line_buffer[DISPLAY_WIDTH * 3U];


/* =========================================================
 * Private function declarations
 * ========================================================= */

static bool Display_SendCommand(uint8_t command);

static bool Display_SendData(const uint8_t *data,
                             uint32_t length);

static void Display_HardwareReset(void);

static bool Display_SetAddressWindow(uint16_t x0,
                                     uint16_t y0,
                                     uint16_t x1,
                                     uint16_t y1);

static bool Display_FillRect(uint16_t x,
                             uint16_t y,
                             uint16_t width,
                             uint16_t height,
                             uint8_t red,
                             uint8_t green,
                             uint8_t blue);

static bool Display_DrawChar(char character,
                             uint16_t x,
                             uint16_t y,
                             uint8_t scale,
                             uint8_t red,
                             uint8_t green,
                             uint8_t blue);


/* =========================================================
 * Initialise display
 * ========================================================= */

bool Display_Init(void)
{
    display_spi = mx_spi3_gethandle();

    if (display_spi == NULL)
    {
        return false;
    }


    /* Hardware reset */
    Display_HardwareReset();


    /* Software reset */
    if (!Display_SendCommand(ILI9488_SWRESET))
    {
        return false;
    }

    HAL_Delay(120U);


    /* Exit sleep mode */
    if (!Display_SendCommand(ILI9488_SLPOUT))
    {
        return false;
    }

    HAL_Delay(120U);


    /*
     * Landscape orientation.
     *
     * We can change this later if the screen appears
     * rotated, mirrored or upside down.
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
     * RGB666 / 18-bit pixel format.
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


    /* Turn display on */
    if (!Display_SendCommand(ILI9488_DISPON))
    {
        return false;
    }

    HAL_Delay(20U);

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


/* =========================================================
 * Send command
 * ========================================================= */

static bool Display_SendCommand(uint8_t command)
{
    /* DC LOW = command */
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
 * Send parameter / pixel data
 * ========================================================= */

static bool Display_SendData(const uint8_t *data,
                             uint32_t length)
{
    if ((data == NULL) ||
        (length == 0U))
    {
        return false;
    }


    /* DC HIGH = data */
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
 * Set area of display to draw into
 * ========================================================= */

static bool Display_SetAddressWindow(uint16_t x0,
                                     uint16_t y0,
                                     uint16_t x1,
                                     uint16_t y1)
{
    uint8_t data[4];


    /* Column start / end */
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


    /* Row start / end */
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


    /* Start writing pixels */
    if (!Display_SendCommand(ILI9488_RAMWR))
    {
        return false;
    }


    return true;
}


/* =========================================================
 * Fill rectangle with one colour
 * ========================================================= */

static bool Display_FillRect(uint16_t x,
                             uint16_t y,
                             uint16_t width,
                             uint16_t height,
                             uint8_t red,
                             uint8_t green,
                             uint8_t blue)
{
    if ((width == 0U) ||
        (height == 0U))
    {
        return false;
    }


    if ((x >= DISPLAY_WIDTH) ||
        (y >= DISPLAY_HEIGHT))
    {
        return false;
    }


    /* Clip rectangle to display */
    if ((x + width) > DISPLAY_WIDTH)
    {
        width = DISPLAY_WIDTH - x;
    }

    if ((y + height) > DISPLAY_HEIGHT)
    {
        height = DISPLAY_HEIGHT - y;
    }


    if (!Display_SetAddressWindow(
            x,
            y,
            (uint16_t)(x + width - 1U),
            (uint16_t)(y + height - 1U)))
    {
        return false;
    }


    /*
     * RGB666:
     * only upper 6 bits of each colour byte are significant.
     */
    red   &= 0xFCU;
    green &= 0xFCU;
    blue  &= 0xFCU;


    /* Build one complete coloured row */
    for (uint16_t i = 0U; i < width; i++)
    {
        line_buffer[(i * 3U) + 0U] = red;
        line_buffer[(i * 3U) + 1U] = green;
        line_buffer[(i * 3U) + 2U] = blue;
    }


    /* Send that same row repeatedly */
    for (uint16_t row = 0U; row < height; row++)
    {
        if (!Display_SendData(
                line_buffer,
                (uint32_t)width * 3U))
        {
            return false;
        }
    }


    return true;
}


/* =========================================================
 * Fill whole display
 * ========================================================= */

bool Display_FillScreen(uint8_t red,
                        uint8_t green,
                        uint8_t blue)
{
    return Display_FillRect(
        0U,
        0U,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        red,
        green,
        blue
    );
}


/* =========================================================
 * Basic 5 × 7 character drawing
 *
 * Only T, E and S are required for initial test.
 * ========================================================= */

static bool Display_DrawChar(char character,
                             uint16_t x,
                             uint16_t y,
                             uint8_t scale,
                             uint8_t red,
                             uint8_t green,
                             uint8_t blue)
{
    uint8_t rows[7];


    switch (character)
    {
        case 'T':
            rows[0] = 0x1FU;   /* 11111 */
            rows[1] = 0x04U;   /* 00100 */
            rows[2] = 0x04U;
            rows[3] = 0x04U;
            rows[4] = 0x04U;
            rows[5] = 0x04U;
            rows[6] = 0x04U;
            break;


        case 'E':
            rows[0] = 0x1FU;   /* 11111 */
            rows[1] = 0x10U;   /* 10000 */
            rows[2] = 0x10U;
            rows[3] = 0x1FU;   /* 11111 */
            rows[4] = 0x10U;
            rows[5] = 0x10U;
            rows[6] = 0x1FU;
            break;


        case 'S':
            rows[0] = 0x0FU;   /* 01111 */
            rows[1] = 0x10U;   /* 10000 */
            rows[2] = 0x10U;
            rows[3] = 0x0EU;   /* 01110 */
            rows[4] = 0x01U;   /* 00001 */
            rows[5] = 0x01U;
            rows[6] = 0x1EU;   /* 11110 */
            break;


        default:
            return false;
    }


    /*
     * Draw each active 5x7 font pixel as a filled rectangle.
     */
    for (uint8_t row = 0U; row < 7U; row++)
    {
        for (uint8_t column = 0U; column < 5U; column++)
        {
            uint8_t mask =
                (uint8_t)(1U << (4U - column));

            if ((rows[row] & mask) != 0U)
            {
                if (!Display_FillRect(
                        (uint16_t)(x + (column * scale)),
                        (uint16_t)(y + (row * scale)),
                        scale,
                        scale,
                        red,
                        green,
                        blue))
                {
                    return false;
                }
            }
        }
    }


    return true;
}


/* =========================================================
 * Initial display hardware test
 * ========================================================= */

bool Display_DrawTest(void)
{
    /*
     * Scale 8:
     *
     * Character:
     * 5 × 8 = 40 pixels wide
     * 7 × 8 = 56 pixels tall
     *
     * Letter spacing = 8 pixels
     *
     * TEST width:
     * 40 + 8 + 40 + 8 + 40 + 8 + 40
     * = 184 pixels
     */

    const uint8_t scale = 8U;

    const uint16_t text_width = 184U;
    const uint16_t text_height = 56U;

    const uint16_t start_x =
        (DISPLAY_WIDTH - text_width) / 2U;

    const uint16_t start_y =
        (DISPLAY_HEIGHT - text_height) / 2U;


    /* Black background */
    if (!Display_FillScreen(
            0U,
            0U,
            0U))
    {
        return false;
    }


    /* White T */
    if (!Display_DrawChar(
            'T',
            start_x,
            start_y,
            scale,
            255U,
            255U,
            255U))
    {
        return false;
    }


    /* White E */
    if (!Display_DrawChar(
            'E',
            (uint16_t)(start_x + 48U),
            start_y,
            scale,
            255U,
            255U,
            255U))
    {
        return false;
    }


    /* White S */
    if (!Display_DrawChar(
            'S',
            (uint16_t)(start_x + 96U),
            start_y,
            scale,
            255U,
            255U,
            255U))
    {
        return false;
    }


    /* White T */
    if (!Display_DrawChar(
            'T',
            (uint16_t)(start_x + 144U),
            start_y,
            scale,
            255U,
            255U,
            255U))
    {
        return false;
    }


    return true;
}