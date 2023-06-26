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
* Для тестирования нужна библиотека CxxTest:
  * Можно установить используя `sudo apt-get install cxxtest`



# Скриншоты
![image](![welcome](https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/1c2289b3-3710-4b3b-bfbd-cfe828f5ac53)
![image](![mainWindow](https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/71a05482-1749-4d4c-87ad-e6392b7bb00c)
![image](![userProfile](https://github.com/OgurtsovAndrei/Messenger-project/assets/22873912/a1be0e88-652f-47f6-8bd9-bde02ceb60bd)
