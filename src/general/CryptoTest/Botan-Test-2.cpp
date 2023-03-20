//
// Created by andrey on 19.03.23.
//
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/aead.h>
#include <botan/rsa.h>
#include <botan/types.h>
#include <botan/auto_rng.h>
#include <botan/secmem.h>
#include <botan/cipher_mode.h>
#include <botan/pubkey.h>
#include <botan/base64.h>
#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/data_src.h>
#include <fstream>
#include <iostream>


int main()
{
//    std::string status = "e";
    std::string status = "d";

    std::unique_ptr<Botan::Cipher_Mode> enc = Botan::Cipher_Mode::create("AES-128/CBC/PKCS7", Botan::Cipher_Dir::Encryption);
    std::unique_ptr<Botan::Cipher_Mode> dec = Botan::Cipher_Mode::create("AES-128/CBC/PKCS7", Botan::Cipher_Dir::Decryption);

    const std::string plaintext("(5523224LOMAKDKWJDG#$%)");
    const std::string encText ="A9B7DC28Cdgjlpuy";

    Botan::secure_vector<uint8_t> myText(encText.data(), encText.data()+encText.length());
    Botan::secure_vector<uint8_t> iv = myText;

    Botan::secure_vector<uint8_t> pt (plaintext.data(), plaintext.data()+plaintext.length());
    std::string encordedText;

    const std::vector<uint8_t> key = Botan::hex_decode("2B7E151628AED2A6ABF7158809CF4F3C");

    if(status[0] == 'e')
    {
        std::ofstream myfile;
        myfile.open("encoded.txt");

        enc->set_key(key);
        enc->start(iv);
        enc->finish(pt);

        std::cout <<"enc->name()"<< enc->name() << " with iv " <<std::endl;
        std::cout<<"Botan::hex_encode(iv)"<<Botan::hex_encode(iv) <<std::endl;
        std::cout<<"Botan::hex_encode(pt)"<<Botan::hex_encode(pt) << std::endl;
        myfile <<Botan::hex_encode(pt);
        myfile.close();
    }
    else if (status[0] == 'd')
    {
        std::ifstream readfile;
        readfile.open("encoded.txt");

        readfile>>encordedText;
        std::cout<<encordedText<<std::endl;
//        Botan::secure_vector<uint8_t> tmpPlainText(encordedText.data(), encordedText.data()+encordedText.length());
        Botan::secure_vector<uint8_t> tmpPlainText(Botan::hex_decode_locked(encordedText));

        dec->set_key(key);
        dec->start(iv);
        dec->finish(tmpPlainText);
        std::cout<<tmpPlainText.data()<<std::endl;
        readfile.close();
    }

    return 0;
}