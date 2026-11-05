# LVGL-ILI9341

This is an attempt to use the built in ILI9341 driver provided by LVGL.

At the time of writing the code compiles fine but the call to `lv_ili9341_create()` never returns. I cannot work out what the problem is.

I have included the `lv_conf.h` I am using. This needs to be copied to your Arduino IDE libraries directory. Logging is set to "trace"  in an attempt to spot the problem so the serial printout will be verbose.