# LCD Miser

A small utility for timing out an LCD/TFT backlight

Dim it off like a smartphone would with an ESP32 or Teensy, or simply shut it off (all other platforms)

```
[env:node32s]
platform = espressif32
board = node32s
framework = arduino
lib_deps = 
	codewitch-honey-crisis/htcw_lcd_miser
lib_ldf_mode = deep
```