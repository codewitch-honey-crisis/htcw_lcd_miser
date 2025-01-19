#pragma once
#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <esp_idf_version.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif
#ifdef ARDUINO
namespace arduino {
#else
namespace esp_idf {
#endif
template<uint8_t PinBL, bool BLHigh = true
#ifdef ESP_PLATFORM
, uint8_t PwmChannel = 0
#endif
>
class lcd_miser final {
public:
    constexpr static const uint8_t pin_bl = PinBL;
    constexpr static const bool bl_high = BLHigh;
#ifdef ESP_PLATFORM
    constexpr static const uint8_t bl_channel = PwmChannel & 0xF;
#endif
private:
    bool m_initialized;
    uint32_t m_timeout_ms;
    uint32_t m_timeout_ts;
    bool m_dimmed;
#if defined(ESP_PLATFORM) || defined(CORE_TEENSY)
    uint32_t m_fade_ts;
    uint32_t m_fade_step_ms;
    int m_dim_count;
    float m_max_level;
#endif
    void do_fade() {        
#if defined(ESP_PLATFORM)
#ifdef ARDUINO
        uint32_t ms = millis();
        if(ms>=m_fade_ts) {
            m_fade_ts = ms+m_fade_step_ms;
            --m_dim_count;
            const uint8_t i = bl_high?m_dim_count:255*m_max_level-m_dim_count;
            
            ledcWriteChannel(bl_channel,i);
        }   
#else
        uint32_t ms = pdTICKS_TO_MS(xTaskGetTickCount());
        if(ms>=m_fade_ts) {
            m_fade_ts = ms+m_fade_step_ms;
            --m_dim_count;
            
            const uint8_t i = bl_high?m_dim_count:255*m_max_level-m_dim_count;
            ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)bl_channel, i);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)bl_channel);   
        }
#endif
#elif defined(CORE_TEENSY)
    uint32_t ms = millis();
    if(ms>=m_fade_ts) {
        m_fade_ts = ms+m_fade_step_ms;
        --m_dim_count;
        const uint8_t i = bl_high?m_dim_count:255*m_max_level-m_dim_count;
        
        analogWrite(pin_bl,i);
        delay(10);
    }
#else
        digitalWrite(pin_bl,bl_high?LOW:HIGH);
#endif
    }

public:
    
    lcd_miser() : 
            m_initialized(false), 
            m_timeout_ms(10*1000),
            m_timeout_ts(0),
            m_dimmed(false)
#if defined(ESP_PLATFORM) || defined(CORE_TEENSY)
            ,m_fade_ts(0),m_fade_step_ms(10),m_max_level(1.0)
#endif
            {
        
    }
    bool initialize() {
        if(!m_initialized) {
#ifdef ESP_PLATFORM
#ifdef ARDUINO
            // BUG: Output starts dark! This isn't outputing
            ledcAttachChannel(pin_bl,5000,8,bl_channel);
            ledcWriteChannel(bl_channel,bl_high?(255*m_max_level):0);
#else
            ledc_timer_config_t ledc_timer;
            memset(&ledc_timer,0,sizeof(ledc_timer)); 
            ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
            ledc_timer.duty_resolution  = LEDC_TIMER_8_BIT;
            ledc_timer.timer_num        = LEDC_TIMER_0;
            ledc_timer.freq_hz          = 5000;
            ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
        
            ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

            ledc_channel_config_t ledc_cfg;
            memset(&ledc_cfg,0,sizeof(ledc_cfg));
            ledc_cfg.duty = 0;
            ledc_cfg.channel = (ledc_channel_t)bl_channel;
            ledc_cfg.hpoint = 0;
            ledc_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
            ledc_cfg.gpio_num = pin_bl;
            ledc_cfg.timer_sel = LEDC_TIMER_0;
            ledc_cfg.flags.output_invert= 0;
            ledc_cfg.intr_type = LEDC_INTR_DISABLE;
            ESP_ERROR_CHECK(ledc_channel_config(&ledc_cfg));
            ledc_set_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)bl_channel,bl_high?(255*m_max_level):0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE,(ledc_channel_t)bl_channel);
#endif
            m_dim_count = 0;
#else
            pinMode(pin_bl,OUTPUT);
            digitalWrite(pin_bl,bl_high?HIGH:LOW);
#endif
#ifdef ESP_PLATFORM
#ifdef ARDUINO
            m_timeout_ts = millis()+m_timeout_ms;
#else
            m_timeout_ts = pdTICKS_TO_MS(xTaskGetTickCount())+m_timeout_ms;
#endif
#else
            m_timeout_ts = millis()+m_timeout_ms;
#endif
            m_initialized = true;
            m_dimmed = false;
            
        }
        return true;
    }
    inline bool initialized() const { return m_initialized; }
    inline uint32_t timeout() const { return m_timeout_ms; }
    void timeout(uint32_t milliseconds) {
        m_timeout_ms = milliseconds;
#ifdef ARDUINO
        m_timeout_ts = millis()+m_timeout_ms;
#else
#ifdef ESP_PLATFORM
        m_timeout_ts = pdTICKS_TO_MS(xTaskGetTickCount())+m_timeout_ms;
#endif
#endif
    }
    inline bool dimmed() const { return m_dimmed; }
    bool update() {
        if(!initialize()) {return false;}
#ifdef ARDUINO
        uint32_t ms = millis();
#else
#ifdef ESP_PLATFORM
        uint32_t ms = pdTICKS_TO_MS(xTaskGetTickCount());
#endif
#endif
        if(!m_dimmed && ms>=m_timeout_ts) {

            m_dimmed = true;
#if defined(ESP_PLATFORM) || defined(CORE_TEENSY)
            m_dim_count = 255*m_max_level;
            m_fade_ts = ms+m_fade_step_ms;
#else
            do_fade();
#endif
        }
#if defined(ESP_PLATFORM) || defined(CORE_TEENSY)
        if(m_dimmed && m_dim_count) {
            do_fade();
        } 
#endif
        return true;
    }
    bool wake() {
        if(!initialize()) {return false;}
        
#ifdef ARDUINO
        m_timeout_ts = millis()+m_timeout_ms;
#else
#ifdef ESP_PLATFORM
        m_timeout_ts = pdTICKS_TO_MS(xTaskGetTickCount())+m_timeout_ms;
#endif
#endif
        m_dim_count = 0;
        if(!m_dimmed) {return true;}
        m_dimmed = false;
        
#ifdef ESP_PLATFORM
#ifdef ARDUINO
        ledcWriteChannel(bl_channel,bl_high?255*m_max_level:0);
#else
        ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)bl_channel, bl_high?255*m_max_level:0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)bl_channel);   
#endif
#elif defined(CORE_TEENSY)
        analogWrite(pin_bl,bl_high?255*m_max_level:0);
#else
        digitalWrite(pin_bl,bl_high?HIGH:LOW);
#endif
        return true;
    }
    bool dim() {
        if(m_dimmed) { return true;}
        if(!initialize()) {return false;}
        m_dimmed = true;
#if defined(ESP_PLATFORM) || defined(CORE_TEENSY)
        m_dim_count =255;
#ifdef ARDUINO
        m_fade_ts = millis()+m_fade_step_ms;
#else
        m_fade_ts = pdTICKS_TO_MS(xTaskGetTickCount()) +m_fade_step_ms;
#endif
#endif
        do_fade();
        return true;
    }
#if defined(ESP_PLATFORM) || defined(CORE_TEENSY)
    uint32_t fade_step() const { return m_fade_step_ms; }
    void fade_step(uint32_t milliseconds) {
        m_fade_step_ms = milliseconds;
    }
    bool faded() const {
        return m_dim_count == 0 && m_dimmed;
    }
    float max_level() const { return m_max_level; }
    void max_level(float value) {
        m_max_level = value;
        if(!dimmed()) {
#if defined(ESP_PLATFORM)
#ifdef ARDUINO
            ledcWriteChannel(bl_channel,255*value);
#else
            ledc_set_duty(LEDC_LOW_SPEED_MODE, bl_channel, 255*value);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, bl_channel);   
#endif
#elif defined(CORE_TEENSY)
            analogWrite(pin_bl,255*value);
#endif
        }
    }
#endif

};
}