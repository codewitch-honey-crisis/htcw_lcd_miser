#pragma once
#include <Arduino.h>
#ifdef ESP32
#define HTCW_ESP32_PWM
#endif
namespace arduino {
template<uint8_t PinBL, bool BLHigh = true
#ifdef HTCW_ESP32_PWM
, uint8_t PwmChannel = 15
#endif
>
class lcd_miser final {
public:
    constexpr static const uint8_t pin_bl = PinBL;
    constexpr static const bool bl_high = BLHigh;
#ifdef HTCW_ESP32_PWM
    constexpr static const uint8_t bl_channel = PwmChannel & 0xF;
#endif
private:
    bool m_initialized;
    uint32_t m_timeout_ms;
    uint32_t m_timeout_ts;
    bool m_dimmed;
#ifdef HTCW_ESP32_PWM
    uint32_t m_fade_ts;
    uint32_t m_fade_step_ms;
    int m_dim_count;
#endif
    void do_fade() {        
#ifdef HTCW_ESP32_PWM
        uint32_t ms = millis();
        if(ms>=m_fade_ts) {
            m_fade_ts = ms+m_fade_step_ms;
            --m_dim_count;
            const uint8_t i = bl_high?m_dim_count:255-m_dim_count;
            S
            ledcWrite(bl_channel,i);
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
#ifdef HTCW_ESP32_PWM
            ,m_fade_ts(0),m_fade_step_ms(10)
#endif
            {
        
    }
    bool initialize() {
        if(!m_initialized) {
#ifdef HTCW_ESP32_PWM
            ledcAttachPin(pin_bl,bl_channel);
            ledcSetup(bl_channel,5000,8);
            ledcWrite(bl_channel,bl_high?255:0);
            m_dim_count = 0;
            
#else
            pinMode(pin_bl,OUTPUT);
            digitalWrite(pin_bl,bl_high?HIGH:LOW);
#endif
            m_timeout_ts = millis()+m_timeout_ms;
            m_initialized = true;
            m_dimmed = false;
            
        }
        return true;
    }
    inline bool initialized() const { return m_initialized; }
    inline uint32_t timeout() const { return m_timeout_ms; }
    void timeout(uint32_t milliseconds) {
        m_timeout_ms = milliseconds;
        m_timeout_ts = millis()+m_timeout_ms;
    }
    inline bool dimmed() const { return m_dimmed; }
    bool update() {
        if(!initialize()) {return false;}
        uint32_t ms = millis();
        if(!m_dimmed && ms>=m_timeout_ts) {

            m_dimmed = true;
#ifdef HTCW_ESP32_PWM
            m_dim_count = 255;
            m_fade_ts = ms+m_fade_step_ms;
#else
            do_fade();
#endif
        }
#ifdef HTCW_ESP32_PWM
        if(m_dimmed && m_dim_count) {
            do_fade();
        } 
#endif
        return true;
    }
    bool wake() {
        if(!initialize()) {return false;}
        if(!m_dimmed) {return true;}
        m_timeout_ts = millis()+m_timeout_ms;
        m_dimmed = false;
#ifdef HTCW_ESP32_PWM
        m_dim_count = 0;
        ledcWrite(bl_channel,bl_high?255:0);
#else
        digitalWrite(pin_bl,bl_high?HIGH:LOW);
#endif
        return true;
    }
    bool dim() {
        if(m_dimmed) { return true;}
        if(!initialize()) {return false;}
        m_dimmed = true;
#ifdef HTCW_ESP32_PWM
        m_dim_count =255;
        m_fade_ts = millis()+m_fade_step_ms;
#endif
        do_fade();
        return true;
    }
#ifdef HTCW_ESP32_PWM
    inline uint32_t fade_step() const { return m_fade_step_ms; }
    void fade_step(uint32_t milliseconds) {
        m_fade_step_ms = milliseconds;
    }
#endif

};
}