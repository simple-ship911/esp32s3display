#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "st7789.h"
#include "esp_err.h"
#include "esp_log.h"

void app_main(void)
{
    lcd_st7789_init();
    lcd_dma_init();
    vTaskDelay(pdMS_TO_TICKS(2000));
    while(1){
    lcd_display_degree(0);
    lcd_fill_clear(BLUE);
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    lcd_fill_clear(RED);
    lcd_display_degree(90);
    lcd_fill_clear(GREEN);
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    lcd_fill_clear(BLACK);
    lcd_display_degree(180);
    lcd_fill_clear(WHITE);
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    lcd_fill_clear(YELLOW);
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    lcd_display_degree(270);
    lcd_fill_clear(ORANGE);
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    lcd_fill_clear(WHITE);
    vTaskDelay(pdMS_TO_TICKS(1000));}
    
       while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
