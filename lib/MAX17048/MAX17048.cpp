#include "MAX17048.h"
#include <Arduino.h>

namespace EmbeddedDevices
{

    template <int CELL>
    void MAX17048<CELL>::attach(TwoWire& w) { wire = &w; }

    template <int CELL>
    uint16_t MAX17048<CELL>::adc() { return read(REG::VCELL); }

    template <int CELL>
    float MAX17048<CELL>::voltage() { return (float)read(REG::VCELL) * 78.125f * (float)CELL / 1000000.f; }

    template <int CELL>
    uint8_t MAX17048<CELL>::percent() { return (uint8_t)(read(REG::SOC) / 256); }

    template <int CELL>
    float MAX17048<CELL>::accuratePercent() { return (float)read(REG::SOC) / 256.f; }

    template <int CELL>
    bool MAX17048<CELL>::quickStart()
    {
        uint16_t v = read(REG::MODE);
        bitSet(v, 14);
        write(REG::MODE, v);
        return true;
    }

    template <int CELL>
    void MAX17048<CELL>::enSleep(bool b)
    {
        uint16_t v = read(REG::MODE);
        bitWrite(v, 13, b);
        write(REG::MODE, v);
    }

    template <int CELL>
    bool MAX17048<CELL>::isSleepable() { return bitRead(read(REG::MODE), 13); }

    template <int CELL>
    uint16_t MAX17048<CELL>::mode() { return read(REG::MODE); }

    template <int CELL>
    uint8_t MAX17048<CELL>::version() { return (uint8_t)read(REG::VERSION); }

    template <int CELL>
    bool MAX17048<CELL>::isHibernating() { return bitRead(read(REG::MODE), 12); }

    template <int CELL>
    float MAX17048<CELL>::hibernateActTh() { return (float)highByte(read(REG::HIBRT)) * 0.00125; }

    template <int CELL>
    float MAX17048<CELL>::hibernateHibTh() { return (float)lowByte(read(REG::HIBRT)) * 0.208; }

    template <int CELL>
    uint8_t MAX17048<CELL>::hibernateActTh(float th)
    {
        uint16_t v = read(REG::HIBRT) & 0xFF00;
        if (th > 0.0)
        {
            if (th < 0.31875) v |= (uint16_t)(th / 0.00125) & 0x00FF;
            else              v |= 0x00FF;
        }
        write(REG::HIBRT, v);
        return (uint8_t)(v & 0xFF);
    }

    template <int CELL>
    uint8_t MAX17048<CELL>::hibernateHibTh(float th)
    {
        uint16_t v = read(REG::HIBRT) & 0x00FF;
        if (th > 0.0)
        {
            if (th < 53.04) v |= (uint16_t)(th / 0.208) << 8;
            else            v |= 0xFF00;
        }
        write(REG::HIBRT, v);
        return (uint8_t)((v & 0xFF00) >> 8);
    }

    template <int CELL>
    void MAX17048<CELL>::tempCompensate(float temp)
    {
        uint8_t v = 0;
        if (temp > 20.0) v = 0x97 + (temp - 20.0) * -0.5;
        else             v = 0x97 + (temp - 20.0) * -5.0;
        rcomp(v);
    }

    template <int CELL>
    void MAX17048<CELL>::rcomp(uint8_t rcomp)
    {
        uint16_t v = (read(REG::CONFIG) & 0x00FF) | (rcomp << 8);
        write(REG::CONFIG, v);
    }

    template <int CELL>
    void MAX17048<CELL>::sleep(bool b)
    {
        uint16_t v = read(REG::CONFIG);
        bitWrite(v, 7, b);
        write(REG::CONFIG, v);
    }

    template <int CELL>
    bool MAX17048<CELL>::isAlerting()
    {
        return bitRead(read(REG::CONFIG), 5);
    }

    template <int CELL>
    void MAX17048<CELL>::clearAlert()
    {
        uint16_t v = read(REG::CONFIG);
        bitClear(v, 5);
        write(REG::CONFIG, v);
    }

    template <int CELL>
    uint8_t MAX17048<CELL>::emptyAlertThreshold() { return 32 - (read(REG::CONFIG) & 0x001F); }

    template <int CELL>
    void MAX17048<CELL>::emptyAlertThreshold(uint8_t th)
    {
        uint16_t v = read(REG::CONFIG);
        th = constrain(th, 1, 32);
        v &= 0xFFE0;
        v |= 32 - th;
        write(REG::CONFIG, v);
    }

    template <int CELL>
    float MAX17048<CELL>::vAlertMinThreshold() { return highByte(read(REG::VALRT)) * 0.02; }

    template <int CELL>
    void MAX17048<CELL>::vAlertMinThreshold(float th)
    {
        uint16_t v = read(REG::VALRT) & 0x00FF;
        if (th > 0.0)
        {
            if (th < 5.1) v |= (uint16_t)(th/ 0.02) << 8;
            else          v |= 0xFF00;
        }
        write(REG::VALRT, v);
    }

    template <int CELL>
    float MAX17048<CELL>::vAlertMaxThreshold() { return (read(REG::VALRT) & 0x00FF) * 0.02; }

    template <int CELL>
    void MAX17048<CELL>::vAlertMaxThreshold(float th)
    {
        uint16_t v = read(REG::VALRT) & 0xFF00;
        if (th > 0.0)
        {
            if (th < 5.1) v |= (uint8_t)(th / 0.02);
            else          v |= 0x00FF;
        }
        write(REG::VALRT, v);
    }

    template <int CELL>
    float MAX17048<CELL>::vResetThreshold() { return (read(REG::VRESET_ID) >> 9) * 0.04; }
    
    template <int CELL>
    void MAX17048<CELL>::vResetThreshold(float th)
    {
        uint16_t v = read(REG::VRESET_ID) & 0x01FF;
        if (th> 0.0)
        {
            if (th < 5.08) v |= (uint16_t)(th / 0.04) << 9;
            else           v |= 0xFE00;
        }
        write(REG::VRESET_ID, v);
    }

    template <int CELL>
    bool MAX17048<CELL>::comparatorEnabled() { return bitRead(read(REG::VRESET_ID), 8); }

    template <int CELL>
    void MAX17048<CELL>::comparatorEnabled(bool b)
    {
        uint16_t v = read(REG::VRESET_ID);
        bitWrite(v, 8,  b);
        write(REG::VRESET_ID, v);
    }

    template <int CELL>
    uint8_t MAX17048<CELL>::id() { return lowByte(read(REG::VRESET_ID)); }

    template <int CELL>
    bool MAX17048<CELL>::vResetAlertEnabled() { return bitRead(read(REG::STATUS), 14); }

    template <int CELL>
    void MAX17048<CELL>::vResetAlertEnabled(bool b)
    {
        uint16_t v = read(REG::STATUS);
        bitWrite(v, 14, b);
        write(REG::STATUS, v);
    }

    template <int CELL>
    uint8_t MAX17048<CELL>::alertFlags() { return highByte(read(REG::STATUS)) & 0x3F; }

    template <int CELL>
    void MAX17048<CELL>::clearAlert(uint8_t flags)
    {
        uint16_t v = read(REG::STATUS);
        v &= ~((flags & 0x3F) << 8);
        write(REG::STATUS, v);
    }

    template <int CELL>
    float MAX17048<CELL>::crate() { return (float)read(REG::CRATE) * 0.208f; }

    template <int CELL>
    uint8_t MAX17048<CELL>::status() { return read(REG::STATUS); }

    template <int CELL>
    bool MAX17048<CELL>::highVoltage() { return bitRead(alertFlags(), 1); }

    template <int CELL>
    bool MAX17048<CELL>::lowVoltage() { return bitRead(alertFlags(), 2); }

    template <int CELL>
    bool MAX17048<CELL>::resetVoltage() { return bitRead(alertFlags(), 3); }

    template <int CELL>
    bool MAX17048<CELL>::lowSOC() { return bitRead(alertFlags(), 4); }

    template <int CELL>
    bool MAX17048<CELL>::chnageSOC() { return bitRead(alertFlags(), 5); }

    template <int CELL>
    bool MAX17048<CELL>::reset() { write(REG::CMD, 0x5400, true); return true; }

    template <int CELL>
    void MAX17048<CELL>::write(const REG reg, const bool stop)
    {
        wire->beginTransmission(I2C_ADDR);
        wire->write((uint8_t)reg);
        wire->endTransmission(stop);
    }

    template <int CELL>
    void MAX17048<CELL>::write(const REG reg, const uint16_t data, const bool stop)
    {
        wire->beginTransmission(I2C_ADDR);
        wire->write((uint8_t)reg);
        wire->write((data & 0xFF00) >> 8);
        wire->write((data & 0x00FF) >> 0);
        wire->endTransmission(stop);
    }

    template <int CELL>
    uint16_t MAX17048<CELL>::read(const REG reg)
    {
        write(reg, false);
        wire->requestFrom((uint8_t)I2C_ADDR, (uint8_t)2);
        uint16_t data = (uint16_t)((wire->read() << 8) & 0xFF00);
        data |= (uint16_t)(wire->read() & 0x00FF);
        return data;
    }

    // Explicit template instantiation for the used types
    template class MAX17048<1>;
    template class MAX17048<2>;
}

