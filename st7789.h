#pragma once


#include "stdint.h"
#include "fonts.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_lcd_panel_io.h>
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_err.h"
#include "esp_log.h"


#define LCD_HOST       SPI2_HOST

#define PIN_NUM_MISO   -1
#define PIN_NUM_MOSI   GPIO_NUM_11
#define PIN_NUM_CLK    GPIO_NUM_12

#define PIN_NUM_CS     GPIO_NUM_5
#define PIN_NUM_DC     GPIO_NUM_7

#define PIN_NUM_RST    GPIO_NUM_6


#define RED 0xf800
#define GREEN 0x07e0
#define BLUE 0x001f
#define WHITE 0xffff
#define BLACK 0x0000
#define YELLOW 0xFFE0
#define ORANGE 0xFC00



#define degree0 0x00
#define degree90 0xA0
#define degree180 0xc0
#define degree270 0x60





void lcd_st7789_init(void);
void lcd_display_degree(uint16_t degree);
void lcd_dma_init(void);
void lcd_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_fill_clear(uint16_t color);
// void lcd_draw_char(uint16_t x, uint16_t y, char c);
// void lcd_draw_string(uint16_t x, uint16_t y, char *str);
// void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
