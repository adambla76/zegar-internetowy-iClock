# zegar internetowy iClock
![IMG_2087](https://user-images.githubusercontent.com/17962241/168326243-8f71a963-484c-4b51-847f-2ad88b9cb562.JPG)

# Opis funkcjonalny
Zegar został zbudowany w opartciu na procesor ESP8266 Wemos D1 Mini. Jego główną funkcją jest stałe wyświetlanie aktualnej godziny synchronizowanej z czasem atomowym pobieranym z serwerów NTP. Przy starcie urządzenie loguje się do domowej sieci WIFI, a następnie pobiera dane czasowe i przechodzi do prezentacji godziny. Co 30 sek zegar pokazuje dodatkowo aktualną datę, temperaturę , wilgotność, ciśnienie oraz prognozowany opad deszczu u mm.   Wszystkie parametry pogodowe pobierane są z serwera IMGW zgodnie z bieżącą lokalizacją gdzie znajduje się zegar. Jako wyświetlacz zosał użyty panel led dotmatrix 8x8 o długości 5 modułów. Za wyświetlaniem liter i cyfr na panelu odpowiedzialna jest biblioteka https://github.com/MajicDesigns/MD_Parola Ze względu na to że biblioteka ta jest bardzo uniwersalna wymaga odpowiedniej konfiguracji przed kompilacją tak aby zgadzał się rodzaj modułów oraz sposób adresacji pixeli (zigzag/progressive/top-left/bottom-right). Za pobieranie aktulnego czasu z NTP odpowiedzialna jest biblioteka https://github.com/PaulStoffregen/Time . Ma ona wewnętrznie zaimpementowaną automatyczną zmianę czasu DST (letni/zimowy) . Projekt pracuje już ok 5 lat bez żadnych awarii ani zawieszeń. 

# Konstrukcja zegara

![IMG_2085](https://user-images.githubusercontent.com/17962241/168324303-0017baa9-4e82-473a-8cd3-f4a06a7fd927.JPG)
![IMG_2080](https://user-images.githubusercontent.com/17962241/168326194-e6cf5c81-57d6-4cc1-b927-68b1b8b2db94.JPG)
![IMG_2081](https://user-images.githubusercontent.com/17962241/168326203-6167b2df-d18c-40bc-b405-d1600cb18190.JPG)
![IMG_2082](https://user-images.githubusercontent.com/17962241/168326214-33da57d9-376d-4871-9dd1-351dd8dc6a01.JPG)
![IMG_2083](https://user-images.githubusercontent.com/17962241/168326223-c87859bf-5f76-420e-b07b-52c0647b14af.JPG)
![IMG_2084](https://user-images.githubusercontent.com/17962241/168326228-a9d27a8d-17f9-46a1-9ace-3bd9b675f3c1.JPG)

# Zegar w akcji ...

https://user-images.githubusercontent.com/17962241/168326096-cf94f09a-9b0a-4e30-8d7f-0d54d0796bc8.mp4
