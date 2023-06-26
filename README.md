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
* Для работы части с БД необходимо также установить sqlite3. это можно сделать используя команду `sudo apt-get install sqlite3 libsqlite3-dev`
* Для работы UI нужно...



# Скриншоты
![image](https://github.com/OgurtsovAndrei/Messenger-project/assets/75212610/c0be8206-75a3-49db-b421-aebd5297fa71)
![image](https://github.com/OgurtsovAndrei/Messenger-project/assets/75212610/f33ebe6c-6d57-48be-8dd3-aee0d144cf2d)
![image](https://github.com/OgurtsovAndrei/Messenger-project/assets/75212610/d23a810f-15f9-494e-a8c5-e60d4d95c7ff)
