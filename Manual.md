# Интрукция
## Установка и настройка ОС
### Подготовка micro SD карты
Подключаем нашу micro SD карту. Заходим сюда https://www.raspberrypi.org/software/. Скачиваем и запускаем Raspberry Pi Imager. Нажимаем Choose OS. Потом выбираем Raspberry Pi OS (other) -> Raspberry Pi OS Full (32-bit). Если вы готовы потом всё доустанавливать, то можете выбрать Raspberry Pi OS (32-bit). Потом выбираем Choose Storage, тут надо выбрать micro SD карту. И потом нажимаем на кнопку Write. Подробней можно прочитать тут: https://www.raspberrypi.org/documentation/installation/installing-images/.

### Подготовка ОС
Для этого шага вам потребуется монитор с HDMI проводом, клавиатура, мышка и либо пароль от WiFi, либо LAN. Вставьте mirco SD в Raspberry Pi. Подключите монитрор. Включите питание. Далее следует обыкновенная установка дистрибутива линукс. После этого рекомендую открыть терминал(`Ctrl+Alt+T`) и запустить `sudo apt update`. 

### Дистанционное управление (для Windows 10)
Далее вам потребуется разрешить доступ по SSH. Для этого достаточно исполинть две команды: `sudo systemctl enable ssh` и  `sudo systemctl start ssh`. Далее вы можете сделать SSH ключ, но это не обязательно. Самый простой способ подключиться к Raspberry Pi по SSH будет через Putty. Также, можно воспользоваться Remote Desktop Control(RDP), но не рекомендую, так как это будет работать очень медленно.
