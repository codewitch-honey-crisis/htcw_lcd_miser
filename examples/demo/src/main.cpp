#include <Arduino.h>
#include <Wire.h> // shouldn't need this, but do
#include <tft_io.hpp>
#ifdef ESP_WROVER_KIT
#include <ili9341.hpp>
#else
#include <st7789.hpp>
#include <htcw_button.hpp>
#endif
#include <lcd_miser.hpp>
#include <gfx.hpp>

using namespace arduino;
using namespace gfx;
#ifdef ESP_WROVER_KIT
#define LCD_HOST    VSPI
#define LCD_WIDTH 240
#define LCD_HEIGHT 320
#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5
#define BCKL_HIGH false
#else // Lilygo TTGO
#define LCD_WIDTH 135
#define LCD_HEIGHT 240
#define LCD_HOST    VSPI
#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 19
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#define PIN_NUM_DC   16
#define PIN_NUM_RST  23
#define PIN_NUM_BCKL 4
#define BCKL_HIGH true
#endif
using bus_t = tft_spi_ex<LCD_HOST,PIN_NUM_CS,PIN_NUM_MOSI,PIN_NUM_MISO,PIN_NUM_CLK,SPI_MODE0,
true
#ifdef OPTIMIZE_DMA
,LCD_WIDTH*LCD_HEIGHT*2+8
#endif
>;
#ifdef ESP_WROVER_KIT
using lcd_t = ili9341<PIN_NUM_DC,PIN_NUM_RST,-1,bus_t,1,BCKL_HIGH,400,200>;
#else
using lcd_t = st7789<LCD_WIDTH,LCD_HEIGHT,PIN_NUM_DC,PIN_NUM_RST,-1,bus_t,1,true,400,200>;
button<0,10,true> button0;
#endif
using color_t = color<typename lcd_t::pixel_type>;

lcd_t lcd;
lcd_miser<PIN_NUM_BCKL,BCKL_HIGH> lcd_power;
void setup() {
  Serial.begin(115200);
  lcd_power.initialize();
  //lcd_power.timeout(10000);
  button0.callback([](bool pressed,void* state) {lcd_power.wake();});
  lcd.fill(lcd.bounds(),color_t::white);

}

void loop() {
  lcd_power.update();
#ifndef ESP_WROVER_KIT
  button0.update();
#endif
}