//----------------------------------------------------------------------
// LVGL-ILI9341
// ============
// This is an attempt to use the built in ILI9341 driver provided by LVGL.
//----------------------------------------------------------------------
#include <lvgl.h>
#include <driver/spi_master.h>

// Screen size
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Buffer used by LVGL
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint8_t* drawBuf = NULL;

// If we are not using the TFT_eSPI or Lovyan drivers we need to
// configure the SPI bus
#define PIN_MISO      12
#define PIN_MOSI      13
#define PIN_SCK       14
#define PIN_DC         2
#define PIN_CS        15
#define PIN_RST       -1
#define PIN_BACKLIGHT 21

// SPI handle for the display
spi_device_handle_t spiHandle;

// Text area
static lv_obj_t* txt;

//----------------------------------------------------------------------
// If we are using the built in ILI9341 driver we need to provide these
// two functions to write data to the display.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// my_lcd_send_cmd
// ---------------
// Send short command to the LCD. This function shall wait until the
// transaction finishes.
//----------------------------------------------------------------------
void my_lcd_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size) {
  // Prints added for debugging
  Serial.println("Entering my_lcd_send_cmd");
  Serial.printf("Sending %d bytes\n", cmd_size);
  for (int i = 0; i < cmd_size; i++)
    Serial.printf("%02x ", cmd[i]);
  Serial.println("");

  spi_transaction_t t = {0};
  t.length    = cmd_size;
  t.tx_buffer = cmd;
  esp_err_t e = spi_device_transmit(spiHandle, &t);
  if (e != ESP_OK) {
    Serial.printf("spi_device_transmit() returned %d\n", e);
  }
  Serial.println("Leaving my_lcd_send_cmd");
}

//----------------------------------------------------------------------
// my_lcd_send_color
// -----------------
// Send large array of pixel data to the LCD. If necessary, this
// function has to do the byte-swapping. This function can do the
// transfer in the background.
//----------------------------------------------------------------------
void my_lcd_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size) {
  Serial.println("Entering my_lcd_send_color");
  spi_transaction_t t = {0};
  t.length    = cmd_size;
  t.tx_buffer = cmd;
  spi_device_transmit(spiHandle, &t);
  // spi_device_polling_transmit(spiHandle, &t);
  Serial.println("Leaving my_lcd_send_color");
}

//----------------------------------------------------------------------
// InitSPI
// -------
// Initialise the SPI bus
//----------------------------------------------------------------------
bool InitSPI() {
  // Initialise the SPI bus
  spi_bus_config_t bus = {};
  bus.mosi_io_num     = PIN_MOSI;
  bus.miso_io_num     = PIN_MISO;
  bus.sclk_io_num     = PIN_SCK;
  bus.quadwp_io_num   = -1;
  bus.quadhd_io_num   = -1;
  bus.max_transfer_sz = DRAW_BUF_SIZE;
  esp_err_t e = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
  if (e != ESP_OK) {
    Serial.printf("spi_bus_initialize() returned %d\n", e);
  }

  // Add the ILI9341 device
  spi_device_interface_config_t dev = {};
  dev.clock_speed_hz = 55000000; // Safe speed for ILI9341
  dev.mode           = 0;
  dev.spics_io_num   = PIN_CS;
  dev.queue_size     = 1;
  e = spi_bus_add_device(SPI2_HOST, &dev, &spiHandle);
  if (e != ESP_OK) {
    Serial.printf("spi_bus_add_device() returned %d\n", e);
  }

  // Allocate the buffer used by LVGL. Make it DMA capable.
  drawBuf = (uint8_t*) heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA);

  // Return indicating success
  return true;
}

//----------------------------------------------------------------------
// log_print
// ---------
// If logging is enabled, it will inform the user about what is happening in the library
//----------------------------------------------------------------------
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

//----------------------------------------------------------------------
// CreateMainGUI
// -------------
// Create the controls on the display
//----------------------------------------------------------------------
void CreateMainGUI(void) {
  // Create the text area
  txt = lv_textarea_create(lv_screen_active());
  // Resize the text area to fill the screen with a 5 pixel margin
  lv_obj_set_pos(txt, 5, 5);
  lv_obj_set_size(txt, 310, 230);
  // Set the textarea event handler (currently unused)
  // lv_obj_add_event_cb(txt, textarea_event_handler, LV_EVENT_READY, txt);
  // Set the focus to the text area
  lv_obj_add_state(txt, LV_STATE_FOCUSED);
}

//----------------------------------------------------------------------
// ProcessInput
// ------------
// Add the string read from the serial monitor input to the edit control
//----------------------------------------------------------------------
void ProcessInput(const char* Input) {
  lv_textarea_add_text(txt, Input);
}

//----------------------------------------------------------------------
// setup
// -----
//----------------------------------------------------------------------
void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Turn the backlight on
  pinMode(PIN_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_BACKLIGHT, HIGH);

  // Initialise the SPI bus
  InitSPI();
  Serial.println("SPI bus initialised");

  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);
  Serial.println("LVGL initialised");

  // Create a display object
  lv_display_t * disp;
  // Common Flags
  // LV_LCD_FLAG_NONE: Default settings with no special orientation or color modifications.
  // LV_LCD_FLAG_MIRROR_X: Mirrors the display horizontally (column address order flip).
  // LV_LCD_FLAG_MIRROR_Y: Mirrors the display vertically (page address order flip).
  // LV_LCD_FLAG_BGR: Sets the color order to Blue-Green-Red instead of Red-Green-Blue (RGB).
  lv_lcd_flag_t flags = LV_LCD_FLAG_NONE;
  Serial.println("Creating ILI9341 device");
  disp = lv_ili9341_create(SCREEN_WIDTH, SCREEN_HEIGHT, flags, my_lcd_send_cmd, my_lcd_send_color);
  Serial.println("ILI9341 device created");
  
  // Function to draw the GUI
  CreateMainGUI();
  Serial.println("GUI created");
}

//----------------------------------------------------------------------
// loop
// ----
//----------------------------------------------------------------------
void loop() {
  if (Serial.available()) {
    // Read a string from the serial monitor
    String s = Serial.readString();
    // Send the string to the display
    ProcessInput(s.c_str());
  }

  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}