#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <unistd.h>
#include "esp_log.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"


#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
const static char *TAG = "message";
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_4;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;


void delay(uint32_t delay)
{
    uint64_t start = (uint64_t)esp_timer_get_time();
    uint64_t end = delay + start ;
    while((uint64_t)esp_timer_get_time() < end);
}

void app_main(void)
{
    //Configure ADC
    if (unit == ADC_UNIT_1) {
        ESP_LOGI(TAG, "configuration ADC1");
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    } else {
         ESP_LOGI(TAG, "configuration ADC2");
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed=1;
    TIMERG0.wdt_wprotect=0;
    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    uint32_t delta = 10000 ;
   float alpha = 0.1;
   float average_voltage = 0;
    float next = esp_timer_get_time();
    float time;
    float edge_time ;
    uint32_t sample = 0;
    uint32_t last_sample = 0;
    int32_t delta_sample = 0;
    float sleep_duration = 0;
    int bit ;
    int last_bit = 0;
    float prev_time = 0;
    float T_min = 0.9;
    float T_max = 1.1;
    int P = 200000;
    //Continuously sample ADC1
    while (1) {
            if (unit == ADC_UNIT_1) {
                sample = adc1_get_raw((adc1_channel_t)channel);

            } else {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
                sample = raw;
            }

            //delta_sample = sample - last_sample ;
            time = esp_timer_get_time();
            average_voltage = alpha * sample + (1 - alpha) * average_voltage;
            bit = (sample > average_voltage) ? 1 : 0 ;
            if(bit != last_bit){

                edge_time = esp_timer_get_time();
                if(edge_time - prev_time < (T_min * P)){

                    ESP_LOGI(TAG, "discarded" );
                }

                else if(edge_time - prev_time > (T_max * P)){

                    ESP_LOGI(TAG, "Resync" );
                    prev_time = edge_time;
                }

                else{
                    ESP_LOGI(TAG, "correct edge" );
                    if(bit == 0){

                        ESP_LOGI(TAG, "falling edge detection at %f",time );
                    }

                    else{
                        ESP_LOGI(TAG, "rising edge detection at %f",time );

                    }
                    prev_time = edge_time;
                }
            } 
            last_bit = bit;
            next += delta;
            //last_sample = sample;
            sleep_duration = next - esp_timer_get_time() ;
            sleep_duration = ( sleep_duration > 0 ) ? sleep_duration : 0;
            delay(sleep_duration);
        
    }
       
}