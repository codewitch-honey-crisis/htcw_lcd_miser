#pragma once
#include <Arduino.h>
namespace arduino {
template<uint8_t PinBL, bool BLHigh = true
#ifdef ESP32
, uint8_t PwmChannel = 15
#endif
>
class lcd_miser final {
public:
    constexpr static const uint8_t pin_bl = PinBL;
    constexpr static const bool bl_high = BLHigh;
#ifdef ESP32
    constexpr static const uint8_t bl_channel = PwmChannel & 0xF;
#endif
private:
    bool m_initialized;
    uint32_t m_timeout_ms;
    uint32_t m_timeout_ts;
    bool m_dimmed;
#if defined(ESP32) || defined(CORE_TEENSY)
    uint32_t m_fade_ts;
    uint32_t m_fade_step_ms;
    int m_dim_count;
    float m_max_level;
#endif
    void do_fade() {        
#ifdef ESP32
        uint32_t ms = millis();
        if(ms>=m_fade_ts) {
            m_fade_ts = ms+m_fade_step_ms;
            --m_dim_count;
            const uint8_t i = bl_high?m_dim_count:255*m_max_level-m_dim_count;
            
            ledcWrite(bl_channel,i);
        }   
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
#if defined(ESP32) || defined(CORE_TEENSY)
            ,m_fade_ts(0),m_fade_step_ms(10),m_max_level(1.0)
#endif
            {
        
    }
    bool initialize() {
        if(!m_initialized) {
#ifdef ESP32
            ledcSetup(bl_channel,5000,8);
            ledcAttachPin(pin_bl,bl_channel);
            ledcWrite(bl_channel,bl_high?(255*m_max_level):0);
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
#if defined(ESP32) || defined(CORE_TEENSY)
            m_dim_count = 255*m_max_level;
            m_fade_ts = ms+m_fade_step_ms;
#else
            do_fade();
#endif
        }
#if defined(ESP32) || defined(CORE_TEENSY)
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
#ifdef ESP32
        m_dim_count = 0;
        ledcWrite(bl_channel,bl_high?255*m_max_level:0);
#elif defined(CORE_TEENSY)
        m_dim_count = 0;
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
#if defined(ESP32) || defined(CORE_TEENSY)
        m_dim_count =255;
        m_fade_ts = millis()+m_fade_step_ms;
#endif
        do_fade();
        return true;
    }
#if defined(ESP32) || defined(CORE_TEENSY)
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
#if defined(ESP32)
            ledcWrite(bl_channel,255*value);
#elif defined(CORE_TEENSY)
            analogWrite(pin_bl,255*value);
#endif
        }
    }
#endif

};
}