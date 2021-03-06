# Меркурий 206 <-> ESP8266 <-> Home Assistant
disclaimer:
Сразу скажу, что я не проф. программист, поэтому код может где то быть не элегантен.

За основу взят проект https://github.com/RocketFox2409/MercuryESPHome

И так, была задача передавать показания со счетчика Меркурий 206 (RS-485) в Home Assistant. 

Изначально провод от счетчика шел через преобразователь USB-RS485 в сервер HomeAssistant на Raspberry и через Node-Red формировался запрос и принимался ответ. Все работало, но пришло время развиваться и убрать лишние провода. 

Было собрано 3 сетапа:

1. Первый сетап: Wemos D1 mini + RS485 (MAX485 chipset) + DC-DC конвертер для стабильного питания.
   Работает, но очень не стабильно, постоянно отваливается Wemos.
   ![Image alt](https://github.com/DieMetRik/mercury_esphome/blob/main/wemos/Schemes/Mercury_wemos_1.png)
2. Во втором сетапе добавил дополнительный DС-DC для питания модуля RS485
   Стало постабильнее, но все равно отваливается, так как в Wemos использовал SoftwareSerial, думаю дело в этом
   ![Image alt](https://github.com/DieMetRik/mercury_esphome/blob/main/wemos/Schemes/Mercury_wemos_2.png)
3. Третий сетап аналогичен второму, только Wemos заменил на ESP-01s с HardwareSerial, работает стабильно. Больше трогать не буду.
   Если будут проблемы, можно еще попробовать Wemos с HardwareSerial (пока не изучал вопрос).
   ![Image alt](https://github.com/DieMetRik/mercury_esphome/blob/main/esp01s/Schemes/Mercury_EPS01s.png)
4. Купил новую плату для Modbus https://aliexpress.ru/item/32817720482.html c ней стало намного проще работать. Рекомендую именно ее.
   Там все просто:
   - 3.3 <-> VCC 
   - RX <-> RX
   - TX <-> TX
   - GND <-> GND
   В этой плате не нужно подавать дополнительно сигнал на чтение\запись, что упрощает и сборку и код
   ![Image alt](https://github.com/DieMetRik/mercury_esphome/blob/main/esp01s/Schemes/Mercury_EPS01s_4.jpg)
   
Для соответствующего сетапа потребуется:
Скопировать 2 файла в /config/esphome/mercury/
- mercury-200.02.h
- sensor.py

Сожержимое yaml файла скопировать в свой проект в ESPHome 
Поменять адресс счетчика в файле "mercury-200.02.h"

В Home Assistant сделал вот так:
![Image alt](https://github.com/DieMetRik/mercury_esphome/blob/main/HomeAssistant/Dashboard.png)
Прокинул все в Energy Management и получилось вот так:
![Image alt](https://github.com/DieMetRik/mercury_esphome/blob/main/HomeAssistant/Energy.JPG)
Цены по тарифам взял из интеграции "Личный кабинет Интер РАО"

настроил отправку показаний в Энергосбыт через интеграцию "Личный кабинет Интер РАО" (устанавливается через HACS) + Node-Red для формирования данных.

Оборудование:

ESP01s заказывал у Win Win с Алика https://aliexpress.ru/item/2037396994.html
Для ESP01s первый раз нужен программатор, я использовал: https://aliexpress.ru/item/32688280601.html
Нужно сделать 2 перемычки:
GND + IO0
3.3V + EN
Я припаял 8 pin к программатору с другой сторны и просто сделал перемычками. Получилось универсально.

![Image alt](https://github.com/DieMetRik/mercury_esphome/blob/main/esp01s/Schemes/ESP01s_USB.jpg)

Wemos D1 mini V3.0.0 там же https://aliexpress.ru/item/32651747570.html


