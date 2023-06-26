# Messenger-projServer client interactionect -->> components-merge -->> json-exchange
* В данной ветку я заменяю устаревший класс Request на EncryptedRequest и DecryptedRequest 
* Новые классы работают с JSON строками: библиотека nlohman::json

## Про установку Crypto в нашем проекте

* В Messenger-project/Crypto-libs/botan-master лежат исходники библиотеки Botan.
   * Её можно собрать следуя инструкции:
      * [`./Crypto-libs/botan-master/configure.py`](..%2F..%2FCrypto-libs%2Fbotan-master%2Fconfigure.py) (Сначала запускаем python скрипт, который сгенерирует файлы для сборки)
      * `make` (Собственно начинаем сборку)
      * `make check`
      * `sudo make install` (можно без sudo, но тогда надо дать права)
   * Установка пройдет, надо запомнить куда поставились пакеты, скорее всего что-то такое: "/usr/local/include/botan-3" там лежит папочка botan
   * Больше можно почитать в [Crypto-libs/botan-master/readme.rst](..%2F..%2FCrypto-libs%2Fbotan-master%2Freadme.rst)
