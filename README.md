# Messenger-project
Это проект, который пишут 3 студента 1-го курса НИУ ВШЭ:
 * Огурцов Андрей
 * Иванова Арина
 * Шевченко Будимир
 
 Наш метор: Архаров Андрей

# Описание
Данная программа представляет собой приложение-мессенджер со следующим функционалом:
+ Возможность отправлять сообщения другим пользователям
+ Регистрация (имя, фамилия, логин, пароль)
+ Вход (логин, пароль)
+ Измение типа шифрования (RSA, DSA, DH)
+ Изменение имени/фамилии/логина
+ Создание групп из нескольких пользователей
+ Отправка/Изменение/удаление сообщений
+ Возможность пересылки файлов и фотографий

# Инсптрукция по сборке
* В Messenger-project/Crypto-libs/botan-master лежат исходники библиотеки Botan. Её можно собрать следуя инструкции:
  * [`./Crypto-libs/botan-master/configure.py`](..%2F..%2FCrypto-libs%2Fbotan-master%2Fconfigure.py) (Сначала запускаем python скрипт, который сгенерирует файлы для сборки)
  * `make` (Собственно начинаем сборку)
  * `make check`
  * `sudo make install` (можно без sudo, но тогда надо дать права)
* Для работы части с БД необходимо также установить sqlite3.
  * Это можно сделать используя команду `sudo apt-get install sqlite3 libsqlite3-dev`
* Для работы UI нужно...
* Для работы сервера нужены boost и nlohman.
  * Boost можно установить используя команду `sudo apt install cmake libboost-all-dev`
  * Nlohman можно установить используя команду `sudo apt install nlohmann-json-dev`
* Для тестирования нужна библиотека CxxTest:
  * Можно установить используя `sudo apt-get install cxxtest`



# Скриншоты
<img width="420" alt="Register" src="https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/4a9b1cf4-03b5-427b-a8a7-5b2154b4c878">
<img width="678" alt="mainWindow" src="https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/6569167c-f5ca-414f-a3aa-bb5094803813">
<img width="797" alt="userProfile" src="https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/81591d3a-e492-40b8-9f22-764cac2f01d2">
