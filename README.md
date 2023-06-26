# Messenger-project
Это проект, был реальзован тремя студентами 1-го курса НИУ ВШЭ СПБ:
 * Огурцов Андрей @OgurtsovAndrei
 * Иванова Арина @Arishkamu
 * Шевченко Будимир @BrudLord
 
 Наш метор: Архаров Андрей

## Содержание
1. [Содержание](#Содержание)
1. [Описание](#Описание)
1. [Инструкция по сборке](#Инструкция-по-сборке)
1. [Скриншоты](#Скриншоты)

## Описание
Данный проект представляет собой приложение-мессенджер со следующим функционалом:
+ Возможность отправлять сообщения другим пользователям
+ Регистрация (имя, фамилия, логин, пароль)
+ Вход (логин, пароль)
+ Измение типа шифрования (RSA, DSA, DH)
+ Изменение имени/фамилии/логина
+ Создание групп из нескольких пользователей
+ Отправка/Изменение/удаление сообщений
+ Возможность пересылки файлов и фотографий

## Инструкция по сборке
* В Messenger-project/Crypto-libs/botan-master лежат исходники библиотеки Botan. Её можно собрать следуя инструкции:
  * [`./Crypto-libs/botan-master/configure.py`](..%2F..%2FCrypto-libs%2Fbotan-master%2Fconfigure.py) (Сначала запускаем python скрипт, который сгенерирует файлы для сборки)
  * `make` (Собственно начинаем сборку)
  * `make check`
  * `sudo make install` (можно без sudo, но тогда надо дать права)
* Для работы части с БД необходимо также установить sqlite3.
  * Это можно сделать используя команду `sudo apt-get install sqlite3 libsqlite3-dev`
* Для работы UI нужна библиотека QT
  * Для установки воспользуйьесь оффициальным [сайтом](https://www.qt.io/download), qt6 без дополнительных модулей (в базовой комплектации). Возможно потребуется VPN
* Для работы сервера нужены boost и nlohman.
  * Boost можно установить используя команду `sudo apt install cmake libboost-all-dev`
  * Nlohman можно установить используя команду `sudo apt install nlohmann-json-dev`
* Для тестирования нужна библиотека CxxTest:
  * Можно установить используя `sudo apt-get install cxxtest`



## Скриншоты
<img width="420" alt="Register" src="https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/4a9b1cf4-03b5-427b-a8a7-5b2154b4c878">
<img width="678" alt="mainWindow" src="https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/6569167c-f5ca-414f-a3aa-bb5094803813">
<img width="797" alt="userProfile" src="https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/81591d3a-e492-40b8-9f22-764cac2f01d2">
