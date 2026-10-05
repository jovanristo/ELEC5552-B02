#include "display.h"

#include "mx_spi3.h"
#include "mx_gpio_default.h"

#include "stm32_hal.h"


/* ---------------------------------------------------------
 * ILI9488 commands
 * --------------------------------------------------------- */

#define ILI9488_SWRESET     0x01U
#define ILI9488_SLPOUT      0x11U
#define ILI9488_DISPON      0x29U
#define ILI9488_MADCTL      0x36U
#define ILI9488_COLMOD      0x3AU


/* ---------------------------------------------------------
 * SPI handle
 * --------------------------------------------------------- */

static hal_spi_handle_t *display_spi = NULL;


/* ---------------------------------------------------------
 * Private functions
 * --------------------------------------------------------- */

static bool Display_SendCommand(uint8_t command);
static bool Display_SendData(const uint8_t *data, uint32_t length);
static void Display_HardwareReset(void);


/* ---------------------------------------------------------
 * Initialise display
 * --------------------------------------------------------- */

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
     * Memory access control.
     *
     * 0x28 gives us the initial landscape orientation:
     * 480 x 320.
     *
     * We can change this later if the image is mirrored
     * or upside down.
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
     * ILI9488 SPI pixel format.
     *
     * 0x66 = 18-bit RGB666.
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


/* ---------------------------------------------------------
 * Hardware reset
 * --------------------------------------------------------- */

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


/* ---------------------------------------------------------
 * Send one ILI9488 command
 * --------------------------------------------------------- */

static bool Display_SendCommand(uint8_t command)
{
    /*
     * DC LOW = command
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


/* ---------------------------------------------------------
 * Send parameter / pixel data
 * --------------------------------------------------------- */

static bool Display_SendData(const uint8_t *data, uint32_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return false;
    }


    /*
     * DC HIGH = parameter / display data
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
            100U) != HAL_OK)
    {
        return false;
    }


    return true;
}
