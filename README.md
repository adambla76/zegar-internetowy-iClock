# zegar internetowy iClock
![IMG_2087](https://user-images.githubusercontent.com/17962241/168326243-8f71a963-484c-4b51-847f-2ad88b9cb562.JPG)

# Opis funkcjonalny
  Zegar został zbudowany w oparciu o procesor ESP8266 Wemos D1 Mini. Jego główną funkcją jest stałe wyświetlanie aktualnej godziny synchronizowanej z czasem atomowym pobieranym z serwerów NTP. Przy starcie urządzenie loguje się do domowej sieci WIFI, a następnie pobiera dane czasowe i przechodzi do prezentacji godziny. Co 30 sek zegar pokazuje dodatkowo aktualną datę, temperaturę , wilgotność, ciśnienie oraz prędkość wiatru.   Wszystkie parametry pogodowe pobierane są z serwera IMGW zgodnie z bieżącą lokalizacją gdzie znajduje się zegar. 
  Jako wyświetlacz został użyty panel led dotmatrix 8x8 o długości 5 modułów. Za wyświetlanie liter i cyfr na panelu odpowiedzialna jest biblioteka https://github.com/MajicDesigns/MD_Parola Ze względu na to że biblioteka ta jest bardzo uniwersalna wymaga odpowiedniej konfiguracji przed kompilacją tak aby zgadzał się rodzaj modułów oraz sposób adresacji pixeli (zigzag/progressive/top-left/bottom-right). 
  Za pobieranie aktulnego czasu z serwera NTP odpowiedzialna jest biblioteka https://github.com/PaulStoffregen/Time . Ma ona wewnętrznie zaimpementowaną automatyczną zmianę czasu DST (letni/zimowy) . Projekt pracuje już ok 5 lat bez żadnych awarii ani zawieszeń. Posiada mechanizm automatycznego update'u firmware'u jeżeli taki pojawi się na serwerze z którego się aktualizuje. 
  Kolejną ciekawostką jest przyklejony na tylniej stronie pasek RGB LED WS2812. Ma on za zadanie podświetlać tył zegara (ala Philips Ambilight). W zależności od aktualnej temperatury na zewnątrz kolor podświetlenia zmienia się na niebieski jeżeli temp < 19 stC, zielony <22 stC, żółty <27 stC i czerwony powyżej 27 stC.
  Jako obudowy użyto taniej uniwersalnej karafki na wodę z korkiem dostępnej w sklepach IKEA ;-) 

# Konstrukcja zegara

  Komunikacja pomiędzy ESP8266 a dekoderem MAX7219A zrealizowana jest poprzez 3 piny CLK, DATA, CS . Moduły dotmatrix są ze sobą połączone w szereg 

![8x8-Dot-Display-ESP8266-WeMos-D1-Mini_Steckplatine](https://user-images.githubusercontent.com/17962241/168330084-34ce4f1d-fa7a-4fb9-b42b-58e7311253f7.png)

![IMG_2085](https://user-images.githubusercontent.com/17962241/168324303-0017baa9-4e82-473a-8cd3-f4a06a7fd927.JPG)
![IMG_2080](https://user-images.githubusercontent.com/17962241/168326194-e6cf5c81-57d6-4cc1-b927-68b1b8b2db94.JPG)
![IMG_2081](https://user-images.githubusercontent.com/17962241/168326203-6167b2df-d18c-40bc-b405-d1600cb18190.JPG)
![IMG_2082](https://user-images.githubusercontent.com/17962241/168326214-33da57d9-376d-4871-9dd1-351dd8dc6a01.JPG)
![IMG_2083](https://user-images.githubusercontent.com/17962241/168326223-c87859bf-5f76-420e-b07b-52c0647b14af.JPG)
![IMG_2084](https://user-images.githubusercontent.com/17962241/168326228-a9d27a8d-17f9-46a1-9ace-3bd9b675f3c1.JPG)

# Zegar w akcji ...

https://user-images.githubusercontent.com/17962241/168326096-cf94f09a-9b0a-4e30-8d7f-0d54d0796bc8.mp4
