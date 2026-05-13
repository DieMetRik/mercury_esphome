#pragma once
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace mercury {

static const char *const TAG = "mercury";

class Mercury : public PollingComponent, public uart::UARTDevice {
public:
    void set_voltage_sensor(sensor::Sensor *s) { Volts = s; }
    void set_current_sensor(sensor::Sensor *s) { Amps = s; }
    void set_power_sensor(sensor::Sensor *s) { Watts = s; }
    void set_t1_sensor(sensor::Sensor *s) { Tariff1 = s; }
    void set_t2_sensor(sensor::Sensor *s) { Tariff2 = s; }
    void set_t3_sensor(sensor::Sensor *s) { Tariff3 = s; }
    void set_total_sensor(sensor::Sensor *s) { Sum_Tariff = s; }
    void set_frequency_sensor(sensor::Sensor *s) { Freq = s; }
    void set_datetime_sensor(text_sensor::TextSensor *s) { dt_string = s; }

    void set_serial_number(uint32_t serial) { this->serial_number_ = serial; }

    unsigned char electrical_parameters[7];
    unsigned char electrical_parameters_F[7];
    unsigned char tarif[7];
    unsigned char dtime[7];

    uint8_t Re_buf[100];
    int counter = 0;
    int step = 0;
    double V_f, A_f, W_f, F_f;
    double sum;
    double T1_f, T2_f, T3_f;
    bool crc_good[8];

    int cnt_dt_tariff = 1100;
    int interval = 500;
    int cnt_dt_tariff_sp = 1200;

    std::string hh_s, mm_s, ss_s, dd_s, mon_s, yy_s;
    std::string dt_str;

//    int seriall = 319; // РЎРµСЂРёР№РЅС‹Р№ РЅРѕРјРµСЂ

    long mercury_pow(long a, int s) {
        long out = 1;
        for (int i = 0; i < s; i++) out *= a;
        return out;
    }

    template <size_t N = 2>
    long readLong(unsigned char *inp) {
        long out = 0;
        for (int i = 0; i < (int)N; i++) {
            unsigned char v = inp[i];
            int p = mercury_pow(10, ((int)(N - 1) - i) * 2);
            out += (((v >> 4) & 15) * 10 + (v & 15)) * p;
        }
        return out;
    }

    template <size_t N = 2>
    double readDouble(unsigned char *inp, int del) {
        return (double)readLong<N>(inp) / del;
    }

    uint16_t crc16_full(const uint16_t *data, uint16_t len) {
        uint16_t crc = 0xFFFF;
        while (len--) {
            crc ^= *data++;
            for (uint8_t i = 0; i < 8; i++) {
                if ((crc & 0x01) != 0) {
                    crc >>= 1;
                    crc ^= 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }

    bool crc_check(const uint8_t *data, uint16_t len) {
        uint16_t fr[100];
        for (int i = 0; i <= len; i++) fr[i] = data[i];
        uint16_t crc_f = crc16_full(fr, len + 1);
        uint8_t crc_1 = crc_f >> 0;
        uint8_t crc_2 = crc_f >> 8;
        return (crc_1 == data[len + 1] && crc_2 == data[len + 2]);
    }

    std::string readInt(uint8_t value) {
        int val = (value / 16 * 10) + (value % 16);
        char buf[3];
        sprintf(buf, "%02d", val);
        return std::string(buf);
    }

    void calculateParams(unsigned char *frame, uint32_t serial_, unsigned char comm) {
        frame[0] = serial_ >> 24;
        frame[1] = serial_ >> 16;
        frame[2] = serial_ >> 8;
        frame[3] = serial_;
        frame[4] = comm;
        uint16_t fr[6];
        for (int i = 0; i <= 5; i++) fr[i] = frame[i];
        auto crc = crc16_full(fr, 5);
        frame[5] = crc >> 0;
        frame[6] = crc >> 8;
    }

    void setup() override {
        calculateParams(electrical_parameters, this->serial_number_, 0x63);
        calculateParams(electrical_parameters_F, this->serial_number_, 0x81);
        calculateParams(tarif, this->serial_number_, 0x27);
        calculateParams(dtime, this->serial_number_, 0x21);
    }

	void dump_config() override {
	        ESP_LOGCONFIG("mercury", "Mercury 206 Meter:");
			ESP_LOGCONFIG("mercury", "  Serial Number: %u", this->serial_number_); // Выводим в логи при старте
	        ESP_LOGCONFIG("mercury", "  Update Interval: %.1fs", this->get_update_interval() / 1000.0f);
	        LOG_SENSOR("  ", "Voltage", this->Volts);
	        LOG_SENSOR("  ", "Current", this->Amps);
	        LOG_SENSOR("  ", "Power", this->Watts);
	    }

    void main_uart_read(uint8_t *command) {
        this->write_array(command, 7);
        uint32_t start_time = millis();
        while (millis() - start_time < 100) {
            while (available()) {
                Re_buf[counter] = read();
                counter++;
            }
            yield();
        }
    }

    void update() override {
        cnt_dt_tariff++;
        if (step == 0 && cnt_dt_tariff > cnt_dt_tariff_sp) main_uart_read(tarif);
        if (step == 1) main_uart_read(electrical_parameters);
        if (step == 2) main_uart_read(electrical_parameters_F);
        if (step == 3 && cnt_dt_tariff > cnt_dt_tariff_sp) main_uart_read(dtime);

        if (counter > 0) {
            if (Re_buf[4] == 0x27 && crc_check(Re_buf, 20)) {
                crc_good[0] = true;
                double T1 = readDouble<4>(&Re_buf[5], 100);
                if (T1 > 0 && T1 < 50000) T1_f = T1;
                double T2 = readDouble<4>(&Re_buf[9], 100);
                if (T2 > 0 && T2 < 50000) T2_f = T2;
                double T3 = readDouble<4>(&Re_buf[13], 100);
                if (T3 > 0 && T3 < 50000) T3_f = T3;
                sum = T1_f + T2_f + T3_f;
            }
            if (Re_buf[4] == 0x63 && crc_check(Re_buf, 11)) {
                crc_good[1] = true;
                double V = readDouble(&Re_buf[5], 10);
                if (V > 190 && V < 260) V_f = V;
                double A = readDouble(&Re_buf[7], 100);
                if (A >= 0 && A < 100) A_f = A;
                double W = readDouble<3>(&Re_buf[9], 1);
                if (W >= 0 && W < 100000) W_f = W;
            }
            if (Re_buf[4] == 0x81 && crc_check(Re_buf, 14)) {
                crc_good[2] = true;
                double F = readDouble(&Re_buf[5], 100);
                if (F >= 20 && F < 80) F_f = F;
            }
            if (Re_buf[4] == 0x21 && crc_check(Re_buf, 11)) {
                crc_good[3] = true;
                hh_s = readInt(Re_buf[6]);
                mm_s = readInt(Re_buf[7]);
                ss_s = readInt(Re_buf[8]);
                dd_s = readInt(Re_buf[9]);
                mon_s = readInt(Re_buf[10]);
                yy_s = "20" + readInt(Re_buf[11]);
                dt_str = yy_s + "-" + mon_s + "-" + dd_s + " " + hh_s + ":" + mm_s + ":" + ss_s;
            }
        }
        
        counter = 0;
        step = (step + 1) % 4;

        if (crc_good[1]) {
            if (Volts) Volts->publish_state(V_f);
            if (Amps) Amps->publish_state(A_f);
            if (Watts) Watts->publish_state(W_f);
            crc_good[1] = false;
        }
        if (crc_good[0] && crc_good[3]) {
            if (Tariff1) Tariff1->publish_state(T1_f);
            if (Tariff2) Tariff2->publish_state(T2_f);
            if (Tariff3) Tariff3->publish_state(T3_f);
            if (Sum_Tariff) Sum_Tariff->publish_state(sum);
            if (dt_string) dt_string->publish_state(dt_str);
            crc_good[0] = false;
            crc_good[3] = false;
            cnt_dt_tariff = 0;
        }
        if (crc_good[2]) {
            if (Freq) Freq->publish_state(F_f);
            crc_good[2] = false;
        }
    }

protected:
    sensor::Sensor *Volts{nullptr};
    sensor::Sensor *Amps{nullptr};
    sensor::Sensor *Watts{nullptr};
    sensor::Sensor *Tariff1{nullptr};
    sensor::Sensor *Tariff2{nullptr};
    sensor::Sensor *Tariff3{nullptr};
    sensor::Sensor *Sum_Tariff{nullptr};
    sensor::Sensor *Freq{nullptr};
    text_sensor::TextSensor *dt_string{nullptr};
    uint32_t serial_number_{0};
};

} // namespace mercury
} // namespace esphome
