#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

// Motion sensor output (HLK-LD1020 in pulse mode, or an HC-SR501 on the
// fallback front plate). Both drive a push-pull 3.3V high on detection.
//
// GPIO 13 specifically:
//   - not a strapping pin, unlike GPIO 12 (MTDI), where a sensor idling HIGH at
//     power-on selects a 1.8V flash voltage and the board fails to boot
//   - RTC-capable, so it can serve as an ext0 deep-sleep wake source
//   - unused by SD_MMC while it runs in 1-bit mode
//
// CAUTION: GPIO 13 is SD_MMC D3, the card's chip-select. 1-bit mode never
// drives it, but a card samples D3 during initialisation: held LOW it comes up
// in SPI mode and SD_MMC.begin() then fails with a 0x107 timeout. So this pin
// must NOT be pulled down while the card is mounting. It is configured as a
// plain input (the sensor drives it push-pull), the deep-sleep pulldown is
// released at boot before the card is touched, and pin setup happens only
// after the card has mounted. See setup() in main.cpp.
constexpr int PIN_MOTION =                    13;

// On-board flash LED. Very bright and current-hungry; shares the SD D1 line.
constexpr int LED_PIN =                        4;

// OV2640 interface, fixed by the AI-Thinker module's routing.
constexpr int PWDN_GPIO_NUM =                 32;
constexpr int RESET_GPIO_NUM =                -1;
constexpr int XCLK_GPIO_NUM =                  0;
constexpr int SIOD_GPIO_NUM =                 26;
constexpr int SIOC_GPIO_NUM =                 27;
constexpr int Y9_GPIO_NUM =                   35;
constexpr int Y8_GPIO_NUM =                   34;
constexpr int Y7_GPIO_NUM =                   39;
constexpr int Y6_GPIO_NUM =                   36;
constexpr int Y5_GPIO_NUM =                   21;
constexpr int Y4_GPIO_NUM =                   19;
constexpr int Y3_GPIO_NUM =                   18;
constexpr int Y2_GPIO_NUM =                    5;
constexpr int VSYNC_GPIO_NUM =                25;
constexpr int HREF_GPIO_NUM =                 23;
constexpr int PCLK_GPIO_NUM =                 22;

// The SD card is driven over SD_MMC (GPIO 2/14/15 in 1-bit mode), which has no
// chip-select line. There is deliberately no SD_CS constant here: the previous
// value of 5 collided with the camera's Y2 data pin.

#endif
