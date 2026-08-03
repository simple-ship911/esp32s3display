#include "st7789.h"

static const char *TAG="st7789";

static uint16_t COLUMN = 320;
static uint16_t ROW = 240;
        
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;


static uint8_t *dma_buffer = NULL;
static uint32_t dma_buffer_size = 0;
static SemaphoreHandle_t dma_mutex = NULL;

static void lcd_write_cmd(uint8_t cmd)
{
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, NULL, 0));
}
static void lcd_write_cmd_param(uint8_t cmd, const uint8_t *data, size_t data_len){
        if (data_len > 0 && data == NULL) {
        ESP_LOGE(TAG, "Invalid param: data is NULL while len is %d", data_len);
        return;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, data, data_len));
}
// static void lcd_write_cmd_param16(uint8_t cmd, const uint16_t *data, size_t data_len){
//     uint8_t data_buf[2];
    
//     if (data_len > 0 && data == NULL) {
//         ESP_LOGE(TAG, "Invalid param: data is NULL while len is %d", data_len);
//         return;
//     }
//     data_buf[0] = (data[0] >> 8) & 0xFF; // 高字节
//     data_buf[1] = data[0] & 0xFF;        //
//     ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, (const void *)data_buf,  2));
// }
// static void lcd_write_cmd_param32(uint8_t cmd, const uint32_t *data, size_t data_len){
//     uint8_t data_buf[4];
//     if (data_len > 0 && data == NULL) {
//         ESP_LOGE(TAG, "Invalid param: data is NULL while len is %d", data_len);
//         return;
//     }
//     data_buf[0] = (data[0] >> 24) & 0xFF; // 高字节
//     data_buf[1] = (data[0] >> 16) & 0xFF; // 中字节
//     data_buf[2] = (data[0] >> 8) & 0xFF;    // 低字节
//     data_buf[3] = data[0] & 0xFF;        //
//     ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, (const void *)data_buf,  4));
// }
static void lcd_write_data(const uint16_t *color_data, size_t pixel_num){
    if (color_data == NULL) {
        ESP_LOGI(TAG, "Invalid param: color_data is NULL");
        return;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_color(io_handle, 0x2C, (const void *)color_data, pixel_num * 2));
}
void lcd_st7789_init(void)
{   ESP_LOGI(TAG, "Initialize LCD SPI bus...");
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 320 * 240 * 2 +8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "Initialize LCD bus done.");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 5 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    ESP_LOGI(TAG, "Initializing LCD panel IO...");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = COLOR_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    // 为 ST7789 创建 LCD 面板句柄，并指定 SPI IO 设备句柄
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_LOGI(TAG, "Initialize LCD panel done.");
    // 应用 ST7789V 厂商寄存器初始化序列（电源/伽马/帧率等调校，末尾退出睡眠并开显示）
    lcd_write_cmd_param(0x3a,(const uint8_t[]){0x05}, 1); // 设置像素格式为 16 位
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xc5,(const uint8_t[]){0x1a}, 1); //vcom offset
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0x36,(const uint8_t[]){0x00}, 1); // 设置扫描方向为正向扫描
    vTaskDelay(pdMS_TO_TICKS(1));
    //-------------ST7789V Frame rate setting-----------//
    lcd_write_cmd_param(0xb2, (const uint8_t[]){0x05, 0x05, 0x00, 0x33, 0x33}, 5); // Porch Setting
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xb7, (const uint8_t[]){0x05}, 1);                          // Gate Control (12.2v / -10.43v)
    vTaskDelay(pdMS_TO_TICKS(1));
    //--------------ST7789V Power setting---------------//
    lcd_write_cmd_param(0xBB, (const uint8_t[]){0x3F}, 1);  // VCOM
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xC0, (const uint8_t[]){0x2c}, 1);  // Power control
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xC2, (const uint8_t[]){0x01}, 1);  // VDV and VRH Command Enable
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xC3, (const uint8_t[]){0x0F}, 1);  // VRH Set (4.3+(vcom+vcom offset+vdv))
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xC4, (const uint8_t[]){0x20}, 1);  // VDV Set (0v)
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xC6, (const uint8_t[]){0x01}, 1);  // Frame Rate Control in Normal Mode (111Hz)
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xd0, (const uint8_t[]){0xa4, 0xa1}, 2); // Power Control 1
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xE8, (const uint8_t[]){0x03}, 1);  // Power Control 1
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xE9, (const uint8_t[]){0x09, 0x09, 0x08}, 3); // Equalize time control
    vTaskDelay(pdMS_TO_TICKS(1));

    //---------------ST7789V gamma setting-------------//
    lcd_write_cmd_param(0xE0, (const uint8_t[]){0xD0,0x05,0x09,0x09,0x08,0x14,0x28,0x33,0x3F,0x07,0x13,0x14,0x28,0x30}, 14); // Set Gamma
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd_param(0xE1, (const uint8_t[]){0xD0,0x05,0x09,0x09,0x08,0x03,0x24,0x32,0x32,0x3B,0x14,0x13,0x28,0x2F}, 14); // Set Gamma
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_cmd(0x20);                 // 反显关 (INVOFF)
    lcd_write_cmd(0x11);                 // Exit Sleep 退出睡眠
    vTaskDelay(pdMS_TO_TICKS(120));      // 退出睡眠需要至少 120ms
    lcd_write_cmd(0x29);                 // Display on 开显示
    ESP_LOGI(TAG, "Initialize LCD done.");
}
void lcd_display_degree(uint16_t degree)
{
    uint16_t madctl_param = 0;
    switch (degree) {
        case 0:
            COLUMN = 240;
            ROW = 320;
            madctl_param = 0x00; // 正向
            lcd_write_cmd_param(0x36, (const uint8_t[]){madctl_param}, 1);
            break;
        case 90:
            COLUMN = 320;
            ROW = 240;
            madctl_param = 0xA0; // 顺时针旋转90度
            lcd_write_cmd_param(0x36, (const uint8_t[]){madctl_param}, 1);
            
            break;
        case 180:
            COLUMN = 240;
            ROW = 320;
            madctl_param = 0xC0; // 顺时针旋转180度
            lcd_write_cmd_param(0x36, (const uint8_t[]){madctl_param}, 1);
            break;
        case 270:
            COLUMN = 320;
            ROW = 240;
            madctl_param = 0x60; // 顺时针旋转270度
            lcd_write_cmd_param(0x36, (const uint8_t[]){madctl_param}, 1);
           
            break;
        default:
            ESP_LOGW(TAG, "Invalid degree: %d. Valid values are 0, 90, 180, or 270.", degree);
            return;
    }
}
void lcd_dma_init(void)
{
    // 分配最大可能的一行内存（例如 480 像素）
    uint32_t max_width = 480;  // 根据你的屏幕修改
    dma_buffer_size = max_width * 2;
    dma_buffer = heap_caps_malloc(dma_buffer_size, MALLOC_CAP_DMA);
    dma_mutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "DMA buffer allocated: %d bytes", dma_buffer_size);
}
/**
 * @brief 矩形区域DMA填充颜色
 * @param x1,y1 左上角
 * @param x2,y2 右下角（包含在内）
 * @param color RGB565颜色
 */
void lcd_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    if (io_handle == NULL || panel_handle == NULL) return;
    if (x1 > x2 || y1 > y2) return;
    
    
    if (dma_buffer == NULL){
        return;
    }

    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;
    uint32_t line_bytes = width * 2;
    
    // 加锁保护 DMA 缓冲区
    if (dma_mutex) xSemaphoreTake(dma_mutex, portMAX_DELAY);
    
    // 填充一行数据（仅需填充一次）
    uint8_t high_byte = (color >> 8) & 0xFF;
    uint8_t low_byte = color & 0xFF;
    for (int i = 0; i < width; i++) {
        dma_buffer[i * 2]     = high_byte;
        dma_buffer[i * 2 + 1] = low_byte;
    }
    
    // 设置窗口（使用正确的 API）
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, dma_buffer);
    
    // 发送数据（DMA 硬件自动传输）
    for (uint16_t y = 0; y < height; y++) {
        esp_lcd_panel_io_tx_color(io_handle, -1, dma_buffer, line_bytes);
    }
    
    if (dma_mutex) xSemaphoreGive(dma_mutex);
}
void lcd_fill_clear(uint16_t color)
{
    uint8_t col_param[]={
        0x00,0x00,(COLUMN-1)>>8,(COLUMN-1) & 0xFF};
    uint8_t row_param[]={
        0x00,0x00,(ROW-1)>>8,(ROW-1) & 0xFF};

    esp_lcd_panel_io_tx_param(io_handle, 0x2a, col_param, sizeof(col_param));
    esp_lcd_panel_io_tx_param(io_handle, 0x2b, row_param, sizeof(row_param));
    esp_lcd_panel_io_tx_param(io_handle, 0x2c,NULL,0);
    uint32_t line_bytes = COLUMN * 2;
    if (dma_buffer == NULL) {
        dma_buffer = heap_caps_malloc(line_bytes, MALLOC_CAP_DMA);
        if (dma_buffer == NULL) {
            ESP_LOGE(TAG, "DMA buffer allocation failed");
            return;
        }
    }
    uint8_t high = (color >> 8) & 0xFF;
    uint8_t low = color & 0xFF;
    for (int i = 0; i < COLUMN; i++) {
        dma_buffer[i * 2] = high;
        dma_buffer[i * 2 + 1] = low;
    }
    for (uint16_t y = 0; y < ROW; y++) {
        esp_lcd_panel_io_tx_color(io_handle, -1, dma_buffer, line_bytes);
    }

    ESP_LOGI(TAG, "Clear screen done.%d",color);
}
// void lcd_draw_char(uint16_t x, uint16_t y, char ch, uint16_t fg_color)
// {
//     if(io_handle == NULL || panel_handle == NULL) return;
//     if(ch<0x20 || ch>0x7F){
//         ch = ' ';
//     }
//     uint8_t high_fg = (fg_color >> 8) & 0xFF;
//     uint8_t low_fg = fg_color & 0xFF;
//         uint8_t idx = ch - 0x20;
//     const uint8_t *font_data = font8x16[idx];
    
//     // 设置窗口（8x16）
//     uint8_t col_param[] = {
//         (x >> 8) & 0xFF, x & 0xFF,
//         ((x + 7) >> 8) & 0xFF, (x + 7) & 0xFF
//     };
//     uint8_t row_param[] = {
//         (y >> 8) & 0xFF, y & 0xFF,
//         ((y + 15) >> 8) & 0xFF, (y + 15) & 0xFF
//     };
    
//     esp_lcd_panel_io_tx_param(io_handle, 0x2A, col_param, 4);
//     esp_lcd_panel_io_tx_param(io_handle, 0x2B, row_param, 4);
//     esp_lcd_panel_io_tx_param(io_handle, 0x2C, NULL, 0);
    
//     // 发送像素数据
//     uint8_t pixel_data[16 * 8 * 2];  // 16行 x 8列 x 2字节
//     int pixel_idx = 0;
    
//     for (int row = 0; row < 16; row++) {
//         uint8_t byte = font_data[row];
//         for (int col = 0; col < 8; col++) {
//             if (byte & (0x80 >> col)) {
//                 pixel_data[pixel_idx++] = high_fg;
//                 pixel_data[pixel_idx++] = low_fg;
        
//         }
//     }
    
//     // 发送所有像素数据
//     esp_lcd_panel_io_tx_color(io_handle, -1, pixel_data, sizeof(pixel_data));


// }
// }