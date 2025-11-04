#pragma once
#ifndef MAX17048_H
#define MAX17048_H

#include <Wire.h>

namespace EmbeddedDevices
{
    template <int CELL>
    class MAX17048
    {
        public:
            void attach(TwoWire& w);
            uint16_t adc();
            float voltage();

            uint8_t percent();
            float accuratePercent();

            bool quickStart();

            void enSleep(bool b);
            bool isSleepable();

            uint16_t mode();

            uint8_t version();

            bool isHibernating();
            float hibernateActTh();
            float hibernateHibTh();
            uint8_t hibernateActTh(float th);
            uint8_t hibernateHibTh(float th);

            void tempCompensate(float temp);

            void rcomp(uint8_t rcomp);

            void sleep(bool b);

            bool isAlerting();
            void clearAlert();

            uint8_t emptyAlertThreshold();
            void emptyAlertThreshold(uint8_t th);

            float vAlertMinThreshold();
            void vAlertMinThreshold(float th);

            float vAlertMaxThreshold();
            void vAlertMaxThreshold(float th);

            float vResetThreshold();
            void vResetThreshold(float th);

            bool comparatorEnabled();
            void comparatorEnabled(bool b);

            uint8_t id();

            bool vResetAlertEnabled();
            void vResetAlertEnabled(bool b);

            uint8_t alertFlags();
            void clearAlert(uint8_t flags);

            float crate();

            uint8_t status();
            bool highVoltage();
            bool lowVoltage();
            bool resetVoltage();
            bool lowSOC();
            bool chnageSOC();

            bool reset();

        private:
            enum class REG
            {
                VCELL = 0x02,
                SOC = 0x04,
                MODE = 0x06,
                VERSION = 0x08,
                HIBRT = 0x0A,
                CONFIG = 0x0C,
                VALRT = 0x14,
                CRATE = 0x16,
                VRESET_ID = 0x18,
                STATUS = 0x1A,
                TABLE = 0x40,
                CMD = 0xFE
            };

            enum class ALERT
            {
                RI = (1 << 0),
                VH = (1 << 1),
                VL = (1 << 2),
                VR = (1 << 3),
                HD = (1 << 4),
                SC = (1 << 5)
            };

            void write(const REG reg, const bool stop = true);
            void write(const REG reg, const uint16_t data, const bool stop = true);
            uint16_t read(const REG reg);

            bool b_quick_start {false};
            bool b_sleep {false};
            TwoWire* wire;
            const uint8_t I2C_ADDR = 0x36;
    };
}

using MAX17048 = EmbeddedDevices::MAX17048<1>;
using MAX17049 = EmbeddedDevices::MAX17048<2>;

#endif // MAX17048_H
