//Original code: Telegram @RocketFox
//My telegram https://t.me/DieMetRik

#include "esphome.h"

class Mercury : public PollingComponent, public UARTDevice {
	Sensor *Volts {nullptr}; // Сенсоры
	Sensor *Amps {nullptr};
	Sensor *Watts {nullptr};
	Sensor *Tariff1 {nullptr};
	Sensor *Tariff2 {nullptr};
	Sensor *Tariff3 {nullptr};
	Sensor *Sum_Tariff {nullptr};
	Sensor *Freq {nullptr};
	TextSensor *dt_string {nullptr};

	Sensor *CRC_OUT {nullptr};

	uint32_t seriall = 42698492;  // сюда свой серийный номер счетчика

	public:
	Mercury(UARTComponent *parent, Sensor *sensor1, Sensor *sensor2, Sensor *sensor3, Sensor *sensor4, Sensor *sensor5, Sensor *sensor6, Sensor *sensor7, Sensor *sensor8, TextSensor *sensor9, Sensor *sensor10) : UARTDevice(parent) , Volts(sensor1) , Amps(sensor2) , Watts(sensor3), Tariff1(sensor4), Tariff2(sensor5), Tariff3(sensor6), Sum_Tariff(sensor7), Freq(sensor8), dt_string(sensor9), CRC_OUT(sensor10) {}

	unsigned char electrical_parameters[7];	// Байты на получене мгновенных значений
	unsigned char electrical_parameters_F[7];	// Байты на получене мгновенных значений
	unsigned char tarif[7];	// Байты на получение тариффа
	unsigned char dtime[7];	// Байты на получение тариффа

	byte Re_buf[100];
	int counter = 0;
	int step = 0;
	double V_f, A_f, W_f, F_f;
	double sum;
	double T1_f, T2_f, T3_f;
	uint8_t crc_good_f;
	bool crc_good[8];

	int cnt_dt_tariff = 1100; // 	Начинать с 1100 при задании 1200 тогда данные по тарифам подтянутся сразу

	int interval = 500; 		// ИНТЕРВАЛ ОБНОВЛЕНИЯ
	int cnt_dt_tariff_sp = 1200; 	// 600 ПРИ ИНВЕРВАЛЕ 500ms = 5 min


	String hh_s, mm_s, ss_s, dd_s, mon_s, yy_s;	//DateTime from Mercury
	String dt_str;
	uint8_t crc_1, crc_2;
	uint16_t crc_f;

	////////////////////// 
	typedef unsigned char uchar;

	long pow(long a, int s)
	{
		long out = 1;
		for (int i = 0; i < s; i++) out *= a;
		return out;
	}

	template < size_t N = 2 >
		long readLong(uchar *inp)
		{
			long out = 0;

			for (int i = 0; i < N; i++)
			{
				uchar v = inp[i];
				int p = pow(10, ((N - 1) - i) *2);
				out += (((v >> 4) &15) *10 + (v & 15)) *p;
			}

			return out;
		}

	template < size_t N = 2 >
		double readDouble(uchar *inp, int del)
		{
			return (double) readLong<N> (inp) / del;
		}
	//////////////////////

	uint16_t crc16_full(const uint16_t *data, uint16_t len)
	{
		uint16_t crc = 0xFFFF;
		while (len--)
		{
			crc ^= *data++;
			for (uint8_t i = 0; i < 8; i++)
			{
				if ((crc & 0x01) != 0)
				{
					crc >>= 1;
					crc ^= 0xA001;
				}
				else
				{
					crc >>= 1;
				}
			}
		}
		return crc;
	}

	boolean crc_check(const byte *data, uint16_t len)
	{
		uint16_t fr[100];
		for (int i = 0; i <= len; i++)
		{
			fr[i] = data[i];
		}

		uint16_t crc_f = crc16_full(fr, len + 1);
		uint8_t crc_1 = crc_f >> 0;
		uint8_t crc_2 = crc_f >> 8;

		if (crc_1 == data[len + 1] && crc_2 == data[len + 2])
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	//BSD TO STRING FOR DATE_TIME WITH LEADING ZERO
	String readInt(byte value)
	{
		int val = 0;
		val = (value / 16 *10) + (value % 16);
		String val_s = "";
		if (val < 10)
		{
			val_s = "0" + String(val);
		}
		else
		{
			val_s = String(val);
		}

		return val_s;
	}
	unsigned char ToByte(bool b[8])
	{
		unsigned char c = 0;
		for (int i=0; i < 8; ++i)
			if (b[i])
				c |= 1 << i;
		return c;
	}
	void calculateParams(unsigned char *frame, uint32_t serial_, unsigned char comm)
	{
		frame[0] = serial_ >> 24;
		frame[1] = serial_ >> 16;
		frame[2] = serial_ >> 8;
		frame[3] = serial_;
		frame[4] = comm;
		uint16_t fr[6];
		for (int i = 0; i <= 5; i++)
		{
			fr[i] = frame[i];
		}
		auto crc = crc16_full(fr, 5);
		frame[5] = crc >> 0;
		frame[6] = crc >> 8;
	}

	void setup() override
	{
		this->set_update_interval(interval);	
		calculateParams(electrical_parameters, seriall, 0x63);
		calculateParams(electrical_parameters_F, seriall, 0x81);
		calculateParams(tarif, seriall, 0x27);
		calculateParams(dtime, seriall, 0x21);
		pinMode(0, OUTPUT);
		digitalWrite(0, LOW);
	}

	void loop() override {}

	void main_uart_read(byte *command)
	{
		digitalWrite(0, HIGH);
		write_array(command, 7);
		delay(12);
		digitalWrite(0, LOW);
		//delay(13);

		while (available())
		{
			// Читение и запись данных из UART
			Re_buf[counter] = read();
			counter++;
		}

		delay(100);
	}

	void update() override
	{
		cnt_dt_tariff++;
//==========================================================================================================
// ФОРМИРОВАНИЕ ЗАПРОСОВ
		if (step == 0){
			if (cnt_dt_tariff > cnt_dt_tariff_sp) { 	// ПРОВЕРЯТЬ ДАННЫЕ НЕ КАЖДЫЙ ЦИКЛ
				main_uart_read(tarif);			// ТАРИФЫ
			}
		}

		if (step == 1){
			main_uart_read(electrical_parameters);		// НАПРЯЖЕНИЕ, ТОК, МОЩНОСТЬ
		}

		if (step == 2){
			main_uart_read(electrical_parameters_F);	// ЧАСТОТА
		}

		if (step == 3) {
			if (cnt_dt_tariff > cnt_dt_tariff_sp) { 	// ПРОВЕРЯТЬ ДАННЫЕ НЕ КАЖДЫЙ ЦИКЛ
				main_uart_read(dtime);			// ДАТА И ВРЕМЯ
			}
		}

		counter = 0;
//==========================================================================================================
// ПРОВЕРКА ПОЛУЧЕНЫЙ ДАННЫХ
// ТАРИФЫ
		if (Re_buf[0] == 0x00 && Re_buf[4] == 0x27)		
		{

			if (crc_check(Re_buf, 20)==1) {
				crc_good[0] = 1;
				double T1 = readDouble<4> (&Re_buf[5], 100);
				if (T1 > 0 && T1 < 50000){T1_f = T1;};		// Проверяем данные на адекватность

				double T2 = readDouble<4> (&Re_buf[9], 100);
				if (T2 > 0 && T2 < 50000){T2_f = T2;};		// Проверяем данные на адекватность

				double T3 = readDouble<4> (&Re_buf[13], 100);
				if (T3 > 0 && T3 < 50000){T3_f = T3;};		// Проверяем данные на адекватность

				sum = T1_f + T2_f + T3_f;
			}
			else
			{
			crc_good[0] = 0;
			}
		}
// НАПРЯЖЕНИЕ, ТОК, МОЩНОСТЬ
		if (Re_buf[0] == 0x00 && Re_buf[4] == 0x63)		
		{
			if (crc_check(Re_buf, 11)==1) {				// Проверяем CRC пришедшего сообщения
				crc_good[1] = 1;
				double V = readDouble(&Re_buf[5], 10);		// Парсинг байтов и перевод в нормальные значения
				if (V > 190 && V < 260){V_f = V;};		// Проверяем данные на адекватность

				double A = readDouble(&Re_buf[7], 100);		// Парсинг байтов и перевод в нормальные значения
				if (A >= 0 && A < 100){	A_f = A;};		// Проверяем данные на адекватность

				double W = readDouble<3> (&Re_buf[9], 1000);	// Парсинг байтов и перевод в нормальные значения
				if (W >= 0 && W < 100){	W_f = W;};		// Проверяем данные на адекватность
			}
			else
			{
			crc_good[1] = 0;
			}
		}
// ЧАСТОТА
		if (Re_buf[0] == 0x00 && Re_buf[4] == 0x81){		
			if (crc_check(Re_buf, 14)==1) {				// Проверяем CRC пришедшего сообщения
				crc_good[2] = 1;
				double F = readDouble(&Re_buf[5], 100);		// Парсинг байтов и перевод в нормальные значения
				if (F >= 20 && F < 80){F_f = F;	};		// Проверяем данные на адекватность
			}
			else
			{
			crc_good[2] = 0;
			}

		}
// ДАТА И ВРЕМЯ
		if (Re_buf[0] == 0x00 && Re_buf[4] == 0x21){		
			if (crc_check(Re_buf, 11)==1) {				// Проверяем CRC пришедшего сообщения
				crc_good[3] = 1;

				hh_s = readInt(Re_buf[6]);
				mm_s = readInt(Re_buf[7]);
				ss_s = readInt(Re_buf[8]);
	
				dd_s = readInt(Re_buf[9]);
				mon_s = readInt(Re_buf[10]);
				yy_s = "20" + readInt(Re_buf[11]);
				//------------------
				// СФОРМИРОВАТЬ СТРОКУ ВРЕМЕНИ
				dt_str = yy_s + "-" + mon_s + "-" + dd_s + " " + hh_s + ":" + mm_s + ":" + ss_s;	
			}
			else
			{
			crc_good[3] = 0;
			}
		}
//==========================================================================================================
// ФОРМИРУЕМ ШАГ
		step++;
		if (step>= 4){
			step = 0;
		}
//==========================================================================================================

		
//		crc_good_f = (int)ToByte(crc_good);
//		if (CRC_OUT != nullptr) CRC_OUT->publish_state(crc_good_f);

//		if (CRC_OUT != nullptr) CRC_OUT->publish_state(cnt_dt_tariff);


//==========================================================================================================
// Передача данные в HA

// НАПРЯЖЕНИЕ, ТОК, МОЩНОСТЬ
		if (crc_good[1]==1){
			if (Volts != nullptr) Volts->publish_state(V_f);	// Отправка в как сенсор
			if (Amps != nullptr) Amps->publish_state(A_f);
			if (Watts != nullptr) Watts->publish_state(W_f);
			crc_good[1] = 0;

		}
// ТАРИФЫ + ДАТА И ВРЕМЯ
		if (crc_good[0]==1 && crc_good[3]==1){
		
			if (Tariff1 != nullptr) Tariff1->publish_state(T1_f);
			if (Tariff2 != nullptr) Tariff2->publish_state(T2_f);
			if (Tariff3 != nullptr) Tariff3->publish_state(T3_f);
			if (Sum_Tariff != nullptr) Sum_Tariff->publish_state(sum);
			if (dt_string != nullptr) dt_string->publish_state(dt_str.c_str());
			crc_good[0] = 0;
			crc_good[3] = 0;
			cnt_dt_tariff = 0;
		}
// ЧАСТОТА
		if (crc_good[2]==1) {
			if (Freq != nullptr) Freq->publish_state(F_f);
			crc_good[2] = 0;

		}
	};
};