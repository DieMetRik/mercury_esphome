//Автор кода https://github.com/Brokly/ESPHome-Mercury230
//My telegram https://t.me/DieMetRik

#pragma once

#include <Arduino.h>
#include "esphome.h"
#include <stdarg.h>
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace energy_meter_mercury230 {
    
using namespace esphome;
using sensor::Sensor;    
using text_sensor::TextSensor;    
using uart::UARTDevice;  
using uart::UARTComponent;

class Mercury : public Sensor, public PollingComponent  {
    
    //коды ошибок
    enum _replyReason:uint8_t { REP_OK=0, //Все нормально
                                ERROR_COMMAND=1, //Недопустимая команда или параметр
                                ERROR_HARDWARE=2,//Внутренняя ошибка счетчика
                                ERROR_ACCESS_LEVEL=3,//Недостаточен уровень для удовлетворения запроса
                                ERROR_CORE_TIME=4, //Внутренние часы счетчика уже корректировались в течение текущих суток
                                ERROR_CONNECTION=5, //Не открыт канал связи
                                ERROR_TIMEOUT=6, //Ошибка ответа, ошибка КС
                                BUFFER_OVERFLOW=7 // переполнениебуфера
    };

    //типы функций обратного вызова
    typedef void (*callBack1_t)(float);
    typedef void (*callBackStr_t)(char*);
    typedef void (*callBack2_t)(float,float);
    typedef void (*callBack3_t)(float,float,float);
    typedef void (*callBack4_t)(float,float,float,float);
    typedef void (*debug_t)(uint8_t, uint8_t*);

    // типы пакетов по теме
    enum _packetType:uint8_t { _OK=0,       // пакет проверки связи, используется только при тесте или пинге
                           CONNECT=1,    // установка конекта
                           CLOSE=2,      // закрытие конекта
                           WRITE=3,      // запись
                           READ=4,       // чтение параметров 
                           LIST=5,       // чтение журналов
                           READ_PARAMS=8 // чтение доп параметров 
    };

    // тип пакета в буфере отправки
    enum _currentSend:uint8_t { NONE=0, //в буфере нет пакета
                            GET_TEST,
                            GET_ACCESS,
                            WRITE_TIME,
                            CORE_TIME,
                            GET_TIME,
                            GET_POWER,
                            GET_VOLTAGE,
                            GET_CURRENT,
                            GET_KOEF_POWER,
                            GET_FREQ,
                            GET_ANGLE_PH,
                            GET_DISTORTION,
                            GET_TEMP,
                            GET_LINEAR_VOLTAGE,
                            GET_VERS,
                            GET_SER_NUM,
                            GET_TIME_CODE,
                            GET_CRC,
                            GET_VALUE_A,
                            GET_VALUE_B,
                            GET_VALUE_C,
                            GET_ADDR
    };

    // тип запроса чтения параметров
    enum _reqType:uint8_t { PARAM_SER_NUM  = 0 ,    // серийный номер и дату
                        PARAM_VERS     = 3,     // версия
                        PARAM_UNO      = 0x11,  // читаем один конкретный параметр, НЕ БУДУ ИСПОЛЬЗОВАТЬ
                        PARAM_ALL_FULL = 0x14,  // ответ по всем фазам, списком, без сокращения незначащих битов
                        PARAM_ALL      = 0x16,  // ответ по всем фазам, списком, в сокращенном формате, при запросе указывать  фазу 1(!!!)
                        PARAM_CRC      = 0x26   // читаем CRC прибора
    };

    //================== ИСХОДЯЩИЕ ПАКЕТЫ ========================

    // общий буфер отправки
    struct _sBuff{
        uint8_t addr; // адрес счетчика
        _packetType packType; // тип пакета
        uint8_t data[30]; // тело буфера
    };
    
    private:
        // буфера работы с пакетами
        uint8_t inPacket[32];
        uint8_t sizeInPacket=0;
        uint8_t outPacket[32];
        uint8_t sizeOutPacket=0;
        std::string pass=""; // буфер пароля для подключения к счетчику
        bool act_pass=false;       // пароль указан пользователем
        bool pas_in_hex=false;     // ПАРОЛЬ в виде HEX
        bool admin=false;          // тип доступа
        uint8_t addr=0;           // адрес счетчика
    
        #include "mercury230_proto.h"

        // указатель на UART, по которому общаемся с кондиционером
        UARTComponent *my_serial{nullptr};
        Sensor *VoltA {nullptr};
        Sensor *VoltB {nullptr};
        Sensor *VoltC {nullptr};
        Sensor *Amps {nullptr};
        Sensor *AmpA {nullptr};
        Sensor *AmpB {nullptr};
        Sensor *AmpC {nullptr};
        Sensor *Watts {nullptr};
        Sensor *WattA {nullptr};
        Sensor *WattB {nullptr};
        Sensor *WattC {nullptr};
        Sensor *RatioA {nullptr};
        Sensor *RatioB {nullptr};
        Sensor *RatioC {nullptr};
        Sensor *AngleA {nullptr};
        Sensor *AngleB {nullptr};
        Sensor *AngleC {nullptr};
        Sensor *Freq {nullptr};
        Sensor *ValueAA {nullptr};
        Sensor *ValueRA {nullptr};
        Sensor *ValueAB {nullptr};
        Sensor *ValueRB {nullptr};
        Sensor *ValueAC {nullptr};
        Sensor *ValueRC {nullptr};
        TextSensor *vers_string {nullptr};
        TextSensor *error_string {nullptr};
        TextSensor *sn_string {nullptr};
        TextSensor *fab_date_string {nullptr};
        GPIOPin* led_active_pin{nullptr};
        //флаги обработок
        bool cbPower=false;
        bool cbVolt=false;
        bool cbCurrent=false;
        bool cbKoef=false;
        bool cbAngles=false;
        bool cbFreq=false;
        bool cbValuesA=false;
        bool cbValuesB=false;
        bool cbValuesC=false;

        // калбэки для отладки
        bool debugIn=false;
        bool debugOut=false;

        const uint32_t minUpdatePeriod = 5000;
        const char *const TAG = "Mercury";

        // вывод отладочной информации в лог
        // 
        // dbgLevel - уровень сообщения, определен в ESPHome. За счет его использования можно из ESPHome управлять полнотой сведений в логе.
        // msg - сообщение, выводимое в лог
        // line - строка, на которой произошел вызов (удобно при отладке)
        //
        // Своровал, спасибо GrKoR :)
        void _debugMsg(const String &msg, uint8_t dbgLevel = ESPHOME_LOG_LEVEL_DEBUG, unsigned int line = 0, ... ){
            if (dbgLevel < ESPHOME_LOG_LEVEL_NONE) dbgLevel = ESPHOME_LOG_LEVEL_NONE;
            if (dbgLevel > ESPHOME_LOG_LEVEL_VERY_VERBOSE) dbgLevel = ESPHOME_LOG_LEVEL_VERY_VERBOSE;
            if (line == 0) line = __LINE__; // если строка не передана, берем текущую строку
            va_list vl;
            va_start(vl, line);
            esp_log_vprintf_(dbgLevel, TAG, line, msg.c_str(), vl);
            va_end(vl);
        }

        // выводим данные пакета в лог для отладки
        // 
        // dbgLevel - уровень сообщения, определен в ESPHome. За счет его использования можно из ESPHome управлять полнотой сведений в логе.
        // packet - указатель на пакет для вывода;
        //          если указатель на crc равен nullptr или первый байт в буфере не AC_PACKET_START_BYTE, то считаем, что передан битый пакет
        //          или не пакет вовсе. Для такого выводим только массив байт.
        //          Для нормального пакета данные выводятся с форматированием. 
        // line - строка, на которой произошел вызов (удобно при отладке)
        //
        void _debugPrintPacket(uint8_t* data, uint8_t size, bool in, uint8_t dbgLevel = ESPHOME_LOG_LEVEL_DEBUG, unsigned int line = 0){
            String st = "";
            char textBuf[11];
            // заполняем время получения пакета
            memset(textBuf, 0, 11);
            sprintf(textBuf, "%010u", millis());
            st = st + textBuf + ": ";
            // формируем преамбулы
            if (in) {
                st += "[<=] ";      // признак входящего пакета
            } else {
                st += "[=>] ";      // признак исходящего пакета
            } 
            for (uint8_t i=0; i<size; i++){
                memset(textBuf, 0, 11);
                sprintf(textBuf, "%02X", data[i]);
                st += textBuf;
                if(i<size-3){
                   st+=' ';
                } else if(i==size-3){
                   st+=F(" ("); 
                } else if(i==size-1){
                   st+=')'; 
                }                    
            }

            if (line == 0) line = __LINE__;
            _debugMsg(st, dbgLevel, line);
        }
        

	public:
  
        // вывод в дебаг текущей конфигурации компонента
        void dump_config() {
            ESP_LOGCONFIG(TAG, "Mercury:");
            ESP_LOGCONFIG(TAG, "UART Bus:");
            ESP_LOGCONFIG(TAG, "  RX Buffer Size: %u", my_serial->get_rx_buffer_size());
            ESP_LOGCONFIG(TAG, "  Baud Rate: %u baud", my_serial->get_baud_rate());
            ESP_LOGCONFIG(TAG, "  Data Bits: %u", my_serial->get_data_bits());
            ESP_LOGCONFIG(TAG, "  Parity: %s", LOG_STR_ARG(parity_to_str(my_serial->get_parity())));
            ESP_LOGCONFIG(TAG, "  Stop bits: %u", my_serial->get_stop_bits());
            ESP_LOGCONFIG(TAG, "  Update interval: %u sec", (this->update_interval_/1000));
            LOG_SENSOR("", "Voltage phase A ", this->VoltA);
            LOG_SENSOR("", "Voltage phase B ", this->VoltB);
            LOG_SENSOR("", "Voltage phase C ", this->VoltC);
            LOG_SENSOR("", "Amperage Summ ", this->Amps);
            LOG_SENSOR("", "Amperage phase A ", this->AmpA);
            LOG_SENSOR("", "Amperage phase B ", this->AmpB);
            LOG_SENSOR("", "Amperage phase C ", this->AmpC);
            LOG_SENSOR("", "Watts All ", this->Watts);
            LOG_SENSOR("", "Watts phase A ", this->WattA);
            LOG_SENSOR("", "Watts phase B ", this->WattB);
            LOG_SENSOR("", "Watts phase C ", this->WattC);
            LOG_SENSOR("", "Ratio phase A ", this->RatioA);
            LOG_SENSOR("", "Ratio phase B ", this->RatioB);
            LOG_SENSOR("", "Ratio phase C ", this->RatioC);
            LOG_SENSOR("", "Phase shift AB ", this->AngleA);
            LOG_SENSOR("", "Phase shift BC ", this->AngleB);
            LOG_SENSOR("", "Phase shift CA ", this->AngleC);
            LOG_SENSOR("", "Frequency", this->Freq);
            LOG_SENSOR("", "Values ActiveA+ ", this->ValueAA);
            LOG_SENSOR("", "Values ReactiveA+ ", this->ValueRA);
            LOG_SENSOR("", "Values ActiveB+ ", this->ValueAB);
            LOG_SENSOR("", "Values ReactiveB+ ", this->ValueRB);
            LOG_SENSOR("", "Values ActivC+ ", this->ValueAC);
            LOG_SENSOR("", "Values ReactiveC+ ", this->ValueRC);
            LOG_TEXT_SENSOR("", "Date of Мanufacture ", this->fab_date_string);
            LOG_TEXT_SENSOR("", "Serial Number ", this->sn_string);
            LOG_TEXT_SENSOR("", "Version ", this->vers_string);
            LOG_TEXT_SENSOR("", "Last Error ", this->error_string);
            LOG_PIN("Active pin ", this->led_active_pin);
            // параметры предустановленные пользователем
            if(addr){
                ESP_LOGCONFIG(TAG,"Device address: %02u", addr);
            }
            uint8_t buff[6]={0};
            if(act_pass){
                if(pas_in_hex){
                    if(!getPass(buff)){
                       ESP_LOGE(TAG, "Password wrong");
                       act_pass=false;
                    }
                }
            }
            if(act_pass){
                if(pas_in_hex){
                    ESP_LOGCONFIG(TAG, "Password in HEX");
                } else {
                    ESP_LOGCONFIG(TAG, "Password in ASCII");
                }
                ESP_LOGCONFIG("", "Password for send: %X,%X,%X,%X,%X,%X",buff[0],buff[1],buff[2],buff[3],buff[4],buff[5]);   
            }   
            if(admin){
                ESP_LOGCONFIG("", "Access level: ADMIN (highly not recommended)");
            }
        }
        
        // подключение последовательного интерфейса
        void initUart(UARTComponent *parent = nullptr){ my_serial=parent;}  
        // подключение сенсоров и прочего
        void set_VoltA(sensor::Sensor *sens) {this->VoltA=sens;}
        void set_VoltB(sensor::Sensor *sens) {this->VoltB=sens;}
        void set_VoltC(sensor::Sensor *sens) {this->VoltC=sens;}
        void set_Amps(sensor::Sensor *sens) {this->Amps=sens;}
        void set_AmpA(sensor::Sensor *sens) {this->AmpA=sens;}
        void set_AmpB(sensor::Sensor *sens) {this->AmpB=sens;}
        void set_AmpC(sensor::Sensor *sens) {this->AmpC=sens;}
        void set_Watts(sensor::Sensor *sens) {this->Watts=sens;}
        void set_WattA(sensor::Sensor *sens) {this->WattA=sens;}
        void set_WattB(sensor::Sensor *sens) {this->WattB=sens;}
        void set_WattC(sensor::Sensor *sens) {this->WattC=sens;}
        void set_RatioA(sensor::Sensor *sens) {this->RatioA=sens;}
        void set_RatioB(sensor::Sensor *sens) {this->RatioB=sens;}
        void set_RatioC(sensor::Sensor *sens) {this->RatioC=sens;}
        void set_AngleA(sensor::Sensor *sens) {this->AngleA=sens;}
        void set_AngleB(sensor::Sensor *sens) {this->AngleB=sens;}
        void set_AngleC(sensor::Sensor *sens) {this->AngleC=sens;}
        void set_Freq(sensor::Sensor *sens) {this->Freq=sens;}
        void set_ValueAA(sensor::Sensor *sens) {this->ValueAA=sens;}
        void set_ValueRA(sensor::Sensor *sens) {this->ValueRA=sens;}
        void set_ValueAB(sensor::Sensor *sens) {this->ValueAB=sens;}
        void set_ValueRB(sensor::Sensor *sens) {this->ValueRB=sens;}
        void set_ValueAC(sensor::Sensor *sens) {this->ValueAC=sens;}
        void set_ValueRC(sensor::Sensor *sens) {this->ValueRA=sens;}
        // версия
        void set_vers_string(text_sensor::TextSensor *sens) { this->vers_string = sens;}
        // ошибка
        void set_error_string(text_sensor::TextSensor *sens) { this->error_string = sens;}
        // серийный номер
        void set_sn_string(text_sensor::TextSensor *sens) { this->sn_string = sens;}
        // дата изготовления
        void set_fab_date_string(text_sensor::TextSensor *sens) { this->fab_date_string = sens;}
        // нога индикации работы
        void set_active_pin(GPIOPin  *pin){ this->led_active_pin=pin; this->led_active_pin->setup();} 
        // пароль для подключения к счетчику
        void set_pass(const std::string &pass){this->pass=pass; act_pass=true;}
        // вид пароля (HEX ли ASCII) 
        void set_hex_pass(bool pas_in_hex){this->pas_in_hex=pas_in_hex;}
        // тип доступа
        void set_admin(bool admin){this->admin=admin;}
        // пользовательский адрес счетчика
        void set_useraddr(uint8_t addr){this->addr=addr;}
        // период опроса
        void set_update_interval(uint32_t update_interval){this->update_interval_=update_interval;}

        void setup() override {
             
            if (this->update_interval_<5000){
                this->update_interval_=30000;
            }
            
            // СВЕТОДИОД, синий встроенный, показывает активность на шине обмена
            if(this->led_active_pin!=nullptr){
               this->led_active_pin->pin_mode(gpio::FLAG_OUTPUT);
               this->led_active_pin->digital_write(false); // опустить ногу :)
            }
            
            // хочу мощный сигнал
            //esp_wifi_set_max_tx_power(80);
            
            // установим функции обратного вызова, параметров которые хотим получать
            cbPower=(this->Watts!=nullptr || this->WattA!=nullptr || this->WattB!=nullptr || WattC!=nullptr);
            cbVolt=(this->VoltA!=nullptr || this->VoltB!=nullptr || this->VoltC!=nullptr);
            cbCurrent=(this->Amps!=nullptr || this->AmpA!=nullptr || this->AmpB!=nullptr || AmpC!=nullptr);
            cbKoef=(this->RatioA!=nullptr || this->RatioB!=nullptr || this->RatioC!=nullptr);
            cbAngles=(this->AngleA!=nullptr || this->AngleB!=nullptr || this->AngleC!=nullptr);
            cbFreq=(this->Freq!=nullptr);
            cbValuesA=(this->ValueAA!=nullptr || this->ValueRA!=nullptr);
            cbValuesB=(this->ValueAB!=nullptr || this->ValueRB!=nullptr);
            cbValuesC=(this->ValueAC!=nullptr || this->ValueRC!=nullptr);
//            cbValues=(this->ValueRA!=nullptr || this->ValueRA!=nullptr);
//            cbValues=(this->ValueRB!=nullptr || this->ValueRB!=nullptr);
//            cbValues=(this->ValueRC!=nullptr || this->ValueRC!=nullptr);
            // включение отладки (TODO: увязать с флагом отладки)
            debugIn=true;  // будем печатать входящие
            debugOut=true; //  и исходящие пакеты
            // инициализация, адрес устройства 0 - поиск адреса
            setupMerc(minUpdatePeriod); // 
        }

        void loop() override {

            // если подключен uart
            if(this->my_serial!=nullptr){
                // если в буфере приема UART есть данные, значит счетчик что то прислал
                if(this->my_serial->available()){
                    uint8_t data;
                    this->my_serial->read_byte(&data); // получили байт от счетчика
                    getFromMerc(data); // передать байт в работу
                    if(this->led_active_pin!=nullptr){this->led_active_pin->digital_write(false);}
                } 

                uint8_t data=availableMerc(); // количество байт для отправки
                // если в буфере отправки есть данные - отправить счетчику
                // счетчик очень капризен к даймаутам, отправлять нужно непрерывным потоком !!!
                if(data){ 
                    uint8_t* buff=getBuffForMerc();
                    this->my_serial->write_array(buff,data);
                    if(this->led_active_pin!=nullptr){this->led_active_pin->digital_write(true);}
                    //_debugPrintPacket(buff, data, false); 
                    //return; // тут нельзя долго сидеть :(
                }
            }
            
            // если нужно печатаем в лог исходящий пакет
            if(sizeOutPacket){
                this->_debugPrintPacket(outPacket, sizeOutPacket, false); 
                sizeOutPacket=0;
            }
            
            // если нужно печатаем в лог входящий пакет
            if(sizeInPacket){
                this->_debugPrintPacket(inPacket, sizeInPacket, true); 
                sizeInPacket=0;
            }
            
        }

        void update() override {

            // автокоррекция периода опроса
            static uint32_t upd_int = this->update_interval_; 
            static uint32_t upd_period = upd_int-1;
            if(upd_int != upd_period){
                upd_period = upd_int;
                if(upd_period < minUpdatePeriod){
                    upd_period = minUpdatePeriod;   
                }
                this->setUpdatePeriod(upd_period);
                this->_debugMsg(F("Core scan period %u"), ESPHOME_LOG_LEVEL_ERROR, __LINE__, upd_period);               
                upd_period = upd_int;
            }
                    
            // контролируем ошибки связи и момент их возникновения
            static _replyReason oldError = ERROR_CORE_TIME; 
            if(oldError != getLastError()){ //если изменился статус ошибки
                oldError = getLastError();  // запомним новый статус
                if(this->error_string!=nullptr){ // публикация ошибок
                    this->error_string->publish_state(getStrError(oldError));   
                }
                if(oldError==REP_OK){
                    _debugMsg(F("No errors !"), ESPHOME_LOG_LEVEL_INFO, __LINE__);
                } else {
                    _debugMsg(F("Error: %s"), ESPHOME_LOG_LEVEL_ERROR, __LINE__, getStrError(oldError));
                }
            }
            
        }

};

} // namespace energy_meter_mercury230
} // namespace esphome  
