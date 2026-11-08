# 🏠 La casa del futur: és possible aconseguir habitatges intel·ligents i sostenibles?

Aquest repositori conté el codi desenvolupat per a la part pràctica del meu **Treball de Recerca de Batxillerat**, basat en una maqueta de casa bioclimàtica amb domòtica integrada controlada per una ESP32.
## Fitxers disponibles al repositori
- Fitxer .ino amb el programa que controla la maqueta i la connexió per WiFi (programat des de l'Arduino IDE).
- Fitxer .html addicional amb la pàgina web que es visualitza durant la connexió amb l'ESP32. Aquest codi està incorporat també al fitxer principal .ino.

## Funcionalitats
- Control automàtic de finestres i portes.
- Monitoratge de temperatura, humitat, llum i pluja.
- Control d'accés mitjançant RFID.
- Monitoratge energètic amb INA219.
- Pàgina web per visualització i simulació de dades i control domòtic.

## Components principals
- ESP32
- Sensors DHT11
- Sensor de llum
- Sensor de pluja
- RFID MFRC522
- INA219
- Pantalla OLED
- Servomotors
