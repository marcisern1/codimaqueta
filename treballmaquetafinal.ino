// Llibreries
  #include <DHT.h>
  #include <ESP32Servo.h>
  #include <SPI.h>
  #include <MFRC522.h>
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #include <Adafruit_INA219.h>
  #include <WiFi.h>
  #include <WebServer.h>

//Sensors i actuadors
  //Pluja
    #define PinPluja 34
    bool plou=false;

  //Llum
    #define PinLlum 35
    int valorllum=0;

  // Temperatura i humitat
    #define PinTemperaturaExt 32
    #define PinTemperaturaInt 33
    DHT dhtExterior(PinTemperaturaExt, DHT11);
    DHT dhtInterior(PinTemperaturaInt, DHT11);
    float temperaturaexterior=0;
    float humitatexterior=0;
    float temperaturainterior=0;
    float humitatinterior=0;
    String estatClima="";

  //Servos
    #define PinServoSud 27
    #define PinServoNord 14
    #define PinServoPorta 13
    #define PinServoMenjador 26
    Servo servoSud;
    Servo servoNord;
    Servo servoPorta;
    Servo servoMenjador;
    bool finestraSudOberta=false;
    bool finestraNordOberta=false;
    bool menjadorObert=false;
    bool portaOberta=false;
    unsigned long tempsServoSud=0;
    unsigned long tempsServoNord=0;
    unsigned long tempsServoMenjador=0;
    unsigned long tempsServoPorta=0;
    const unsigned long esperaServo=5000;
    
    const int angleSudTancada=0;
    const int angleSudOberta=130;
    const int angleNordTancada=0;
    const int angleNordOberta=130;
    const int angleMenjadorTancada=0;
    const int angleMenjadorOberta=130;
    const int anglePortaTancada=0;
    const int anglePortaOberta=180;
  
  //RFID
    #define PinRFID_MOSI 23
    #define PinRFID_MISO 19
    #define PinRFID_SCK  18
    #define PinRFID_SDA  5
    #define PinRFID_RST  4
    MFRC522 rfid(PinRFID_SDA, PinRFID_RST);
    bool targetaAutoritzada=false;
    unsigned long tempsPorta=0;

  //INA219
    #define PinI2C_SDA 21
    #define PinI2C_SCL 22
    Adafruit_INA219 ina219;
    float voltatgeBateria=0;
    String estatEnergia="";
    float intensitat=0;
    float potencia=0;

  //Pantalla Oled
    #define OLED_WIDTH 128
    #define OLED_HEIGHT 32
    Adafruit_SSD1306 pantalla(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
    unsigned long tempsOLED = 0;
    int pantallaOLED=0;

  //Leds, dividits en dos grups d'interior i un d'exterior
    #define PinLedsExt 17
    #define PinLedsInt 16
    #define PinLedsInt2 12

  //Altres variables
    // Modes casa
      bool modeEstiu = true;
      bool esdeNit=false;
      bool domotic=false;
      bool simulacio=false;
    //Wifi i servidor
      WebServer servidor(80);
      const char* ssidAP = "MaquetaCasa";
      const char* passwordAP = "12345678";
      float temperaturaInteriorManual = 25;
      float temperaturaExteriorManual = 28;
      float humitatInteriorManual = 50;
      float humitatExteriorManual = 60;
      bool finestraSudManual=false;
      bool finestraNordManual=false;
      bool menjadorManual=false;
      bool portaManual=false;
      int llumManual = 5;
      bool plujaManual = false;
      bool ledInteriorManual=false;
      bool ledExteriorManual=false;
      String modeWeb="normal";
      // control actualització web
      bool actualitzarPantalla=false;

void setup() {
  Serial.begin(115200);
  pinMode(PinLedsInt,OUTPUT);
  pinMode(PinLedsInt2,OUTPUT);
  pinMode(PinLedsExt,OUTPUT);
  pinMode(PinPluja,INPUT);
  dhtExterior.begin();
  dhtInterior.begin();
  pinMode(PinLlum, INPUT);
  servoSud.attach(PinServoSud);
  servoNord.attach(PinServoNord);
  servoPorta.attach(PinServoPorta);
  servoMenjador.attach(PinServoMenjador);
  iniciServos();
  SPI.begin(PinRFID_SCK, PinRFID_MISO, PinRFID_MOSI, PinRFID_SDA);
  delay(50);
  rfid.PCD_Init();
  
  Serial.println("RFID preparat");
  rfid.PCD_DumpVersionToSerial();


  Wire.begin(PinI2C_SDA, PinI2C_SCL);
  if(!pantalla.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error OLED");
    while(true);
  }
  if (!ina219.begin()) {
  Serial.println("No s'ha trobat l'INA219");
  while (true);
  }
  Serial.println("INA219 preparat");
  
  iniciarWiFi();
  pantalla.clearDisplay();
  pantalla.setTextColor(SSD1306_WHITE);
  pantalla.display();
}

void loop() {
  llegirpluja();
  llegirtemperatures();
  llegirLlum();
  llegirRFID();
  llegirBateria();

  calcularClima();

  if(domotic==true){
    controlDomotic();
  }
  else{
  controlLlum();
  controlSolar();
  controlVentilacio();
  controlPluja();
  }


  controlRFID();

  // Moure actuadors
  actualitzarActuadors();

  // Mostrar informació
  mostrarEstat();
  mostrarOLED();
  servidor.handleClient();
  delay(500);
}




//Funcions
void llegirpluja(){
  if(simulacio==false){
    int valorpluja = analogRead(PinPluja);
    plou = (valorpluja>2000);
  }
  else{
    plou = plujaManual;
  }
}

void llegirtemperatures() {
  if(simulacio==false){
    temperaturaexterior = dhtExterior.readTemperature();
    humitatexterior = dhtExterior.readHumidity();

    temperaturainterior = dhtInterior.readTemperature();
    humitatinterior = dhtInterior.readHumidity();
  }
  else{
    temperaturaexterior = temperaturaExteriorManual;
    humitatexterior = humitatExteriorManual;
    temperaturainterior = temperaturaInteriorManual;
    humitatinterior = humitatInteriorManual;
  }

  if (isnan(temperaturaexterior) || isnan(humitatexterior)) {
    Serial.println("Error llegint el DHT11 exterior");
  }

  if (isnan(temperaturainterior) || isnan(humitatinterior)) {
    Serial.println("Error llegint el DHT11 interior");
  }
}

void llegirLlum(){
  if(simulacio==false){
    valorllum = analogRead(PinLlum);
  }
  else{
    valorllum = map(llumManual,0,10,0,4095);
  }

  if(valorllum<2000){
    esdeNit=true;
  }
  else{
    esdeNit=false;
  }
}
void iniciServos(){
  servoSud.write(0); //Servo sud màxim 130 graus
  servoNord.write(0); //Servo nord màxim 130 graus
  servoPorta.write(0); //Servo porta principal màxim 180 graus
  servoMenjador.write(0); //Servo jardí màxim 130 graus
}
void llegirRFID(){
  Serial.println("Buscant targetes");

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  Serial.println("Targeta trobada");

  if (!rfid.PICC_ReadCardSerial()) {
    Serial.println("Error llegint UID");
    return;
  }

  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  if (rfid.uid.size == 4 &&
    rfid.uid.uidByte[0] == 0xBB &&
    rfid.uid.uidByte[1] == 0xB6 &&
    rfid.uid.uidByte[2] == 0xFA &&
    rfid.uid.uidByte[3] == 0x06) {

    modeEstiu = !modeEstiu;
    Serial.print("Mode canviat a: ");
    if (modeEstiu){
      Serial.println("ESTIU");
    }else{
      Serial.println("HIVERN");
    }
  }

  // Primera targeta autoritzada
  else if (rfid.uid.size == 4 &&
           rfid.uid.uidByte[0] == 0xF1 &&
           rfid.uid.uidByte[1] == 0x20 &&
           rfid.uid.uidByte[2] == 0x0A &&
           rfid.uid.uidByte[3] == 0x63) {
    targetaAutoritzada = true;
    Serial.println("Accés autoritzat.");
  }
  // Segona targeta autoritzada
  else if (rfid.uid.size == 4 &&
           rfid.uid.uidByte[0] == 0x8A &&
           rfid.uid.uidByte[1] == 0xFB &&
           rfid.uid.uidByte[2] == 0xFD &&
           rfid.uid.uidByte[3] == 0x04) {

    targetaAutoritzada = true;
    Serial.println("Accés autoritzat.");
  }

  // Qualsevol altra targeta
  else {
    Serial.println("Targeta NO autoritzada.");
  }
  rfid.PICC_HaltA();
}


void controlSolar(){
  if(modeEstiu){
    if(valorllum > 3500 && temperaturainterior > 25){
      menjadorObert = false;
    }
    else{
      menjadorObert = true;
    }
  } else{ // Hivern
    if(valorllum > 3000 && temperaturainterior < 22){
      menjadorObert = true;
    } else{
      menjadorObert = false;
    }
  }
  
  if(esdeNit){
  menjadorObert=false;
  }
}
void controlVentilacio(){
  // Estat per defecte
  finestraSudOberta=false;
  finestraNordOberta=false;
  if(!plou && humitatexterior<80){
    if(temperaturainterior>25 && temperaturaexterior<temperaturainterior-1&&modeEstiu){
      finestraSudOberta = true;
      finestraNordOberta = true;
    }
    if(!modeEstiu){
      finestraSudOberta=false;
      finestraNordOberta=false;
    }
    if(esdeNit&&modeEstiu &&temperaturainterior>25&&temperaturaexterior<temperaturainterior){
      finestraSudOberta = true;
      finestraNordOberta = true;
    }
  }
}
void controlPluja(){
  if(plou){
    finestraSudOberta = false;
    finestraNordOberta = false;
    menjadorObert=false;
  }
}

void actualitzarActuadors(){
  if (millis()-tempsServoSud >= esperaServo) {
    if (finestraSudOberta==true) {
      servoSud.write(angleSudOberta);
    } else {
      servoSud.write(angleSudTancada);
    }
    tempsServoSud = millis();
  }
  if (millis()-tempsServoNord >= esperaServo) {
    if (finestraNordOberta) {
      servoNord.write(angleNordOberta);
    } else {
      servoNord.write(angleNordTancada);
    }
    tempsServoNord = millis();
  }
  if (millis()-tempsServoMenjador >= esperaServo) {
    if (menjadorObert) {
      servoMenjador.write(angleMenjadorOberta);
    } else {
      servoMenjador.write(angleMenjadorTancada);
    }
    tempsServoMenjador=millis();
  }
  if (millis()-tempsServoPorta >= esperaServo) {
    if (portaOberta) {
      servoPorta.write(anglePortaOberta);
    } else {
      servoPorta.write(anglePortaTancada);
    }
    tempsServoPorta=millis();
  }
}

void controlRFID(){
  if(targetaAutoritzada){
    portaOberta=true;
    tempsPorta = millis();
    targetaAutoritzada = false;
  }
  if(portaOberta){
    if(millis()-tempsPorta > 5000){
      portaOberta = false;
    }
  }
}

void controlLlum(){
  if(simulacio){
    digitalWrite(PinLedsInt, ledInteriorManual);
    digitalWrite(PinLedsInt2, ledInteriorManual);
    digitalWrite(PinLedsExt, ledExteriorManual);
    return;
  }
  if(modeEstiu){
    if(valorllum>= 3200){
      digitalWrite(PinLedsInt,LOW);
      digitalWrite(PinLedsInt2,LOW);
    }
    else{
      digitalWrite(PinLedsInt,HIGH);
      digitalWrite(PinLedsInt2,HIGH);
    }
  }
  else{
    if(valorllum >= 3800){
      digitalWrite(PinLedsInt,LOW);
      digitalWrite(PinLedsInt2,LOW);
    }
    else{
      digitalWrite(PinLedsInt,HIGH);
      digitalWrite(PinLedsInt2,HIGH);
    }
  }
  if(esdeNit==true){
    digitalWrite(PinLedsExt,HIGH);
  }else{
    digitalWrite(PinLedsExt,LOW);
  }
}

void mostrarEstat(){
  Serial.println("----------------");
  Serial.print("Mode: ");
  if(modeEstiu)
    Serial.println("ESTIU");
    else
      Serial.println("HIVERN");

  Serial.print("Finestra sud: ");
  if(finestraSudOberta==1){
    Serial.println("Oberta");
  } else{
    Serial.println("Tancada");
  }
  Serial.print("Finestra nord: ");
  if(finestraNordOberta==1){
    Serial.println("Oberta");
  } else{
    Serial.println("Tancada");
  }
  Serial.print("Porta menjador: ");
  if(menjadorObert==1){
    Serial.println("Oberta");
  } else{
    Serial.println("Tancada");
  }

  Serial.print("Porta principal: ");
  if(portaOberta==1){
    Serial.println("Oberta");
  } else{
    Serial.println("Tancada");
  }
  Serial.print("Temp. interior: ");
  Serial.print(temperaturainterior);
  Serial.println(" ºC");

  Serial.print("Hum. interior: ");
  Serial.print(humitatinterior);
  Serial.println(" %");

  Serial.print("Temp. exterior: ");
  Serial.print(temperaturaexterior);
  Serial.println(" ºC");

  Serial.print("Hum. exterior: ");
  Serial.print(humitatexterior);
  Serial.println(" %");
  
  Serial.print("Estat del clima: ");
  Serial.println(estatClima);
  
  Serial.print("Pluja: ");
  if(plou==1){
    Serial.println("Sí");
  } else{
    Serial.println("No");
  }

  int llum10 = map(valorllum, 0, 4095, 0, 10);
  Serial.print("Llum: ");
  Serial.print(llum10);
  Serial.println(" /10");

  Serial.print("Voltatge: ");
  Serial.print(voltatgeBateria);
  Serial.println(" V");

  Serial.print("Intensitat: ");
  Serial.print(intensitat);
  Serial.println(" mA");

  Serial.print("Potència: ");
  Serial.print(potencia);
  Serial.println(" mW");

  Serial.print("Estat de la bateria: ");
  Serial.println(estatEnergia);

  Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
}
void mostrarOLED(){
  if(millis() - tempsOLED>3000){
    pantallaOLED++;

    if(pantallaOLED>6){
      pantallaOLED = 0;
    }
    tempsOLED = millis();
  }

  pantalla.clearDisplay();
  pantalla.setTextSize(1);
  pantalla.setCursor(0,0);

  if(pantallaOLED == 0){
    pantalla.println("TEMPERATURA:");
    pantalla.print("Interior: ");
    pantalla.print(temperaturainterior);
    pantalla.println(" C");
    pantalla.print("Exterior: ");
    pantalla.print(temperaturaexterior);
    pantalla.println(" C");
  }
  if(pantallaOLED==1){
    pantalla.println("HUMITAT:");
    pantalla.print("Interior: ");
    pantalla.print(humitatinterior);
    pantalla.println(" %");
    pantalla.print("Exterior: ");
    pantalla.print(humitatexterior);
    pantalla.println(" %");
  }
  if(pantallaOLED==2){
    pantalla.println("CLIMA EXTERIOR:");
    pantalla.println(estatClima);
  }
  if(pantallaOLED==3){
    pantalla.println("LLUM:");
    int llum10 = map(valorllum,0,4095,0,10);
    pantalla.print("Nivell: ");
    pantalla.print(llum10);
    pantalla.println("/10");
  }
  if(pantallaOLED==4){
    pantalla.println("PLUJA:");
    if(plou){
      pantalla.println("SI");
    }else{
      pantalla.println("NO");
    }
  }
  if(pantallaOLED==5){
    pantalla.println("FUNCIONAMENT:");
    if(modeEstiu){
      pantalla.println("Estiu");
    }else{
      pantalla.println("Hivern");
    }
    if(domotic){
      pantalla.println("Control per WiFi");
    }
    else if(simulacio){
      pantalla.println("Dades simulades");
    }
    else{
      pantalla.println("Dades reals");
    }
  }
  if(pantallaOLED==6){
    pantalla.println("BATERIA:");
    pantalla.println(estatEnergia);
    pantalla.print("Voltatge: ");
    pantalla.print(voltatgeBateria);
    pantalla.println(" V");
    pantalla.print("Intensitat: ");
    pantalla.print(intensitat);
    pantalla.println(" mA");
  }
  pantalla.display();
}
void llegirBateria() {
  voltatgeBateria = ina219.getBusVoltage_V();
  intensitat = ina219.getCurrent_mA();
  potencia = ina219.getPower_mW();

  if(voltatgeBateria > 4.1){
    estatEnergia = "Bateries plenes";
  }
  else if(voltatgeBateria < 3.4){
    estatEnergia = "Bateria baixa";
  }
  else{
    estatEnergia = "Estat normal";
  }
}

void calcularClima(){
  if(plou){
    estatClima="Plovent";
  }
  else if(temperaturaexterior < 10){
    estatClima="Fred extrem a l'exterior";
  }
  else if(temperaturaexterior >35){
    estatClima="Calor extrema a l'exterior";
  }
  else if(humitatexterior > 80){
    estatClima="Molta humitat";
  }
  else if(humitatexterior <10){
    estatClima="Aire sec";
  }
  else{
    estatClima="Temps estable";
  }
}
void controlDomotic(){
  finestraSudOberta=finestraSudManual;
  finestraNordOberta=finestraNordManual;
  menjadorObert=menjadorManual;
  targetaAutoritzada=portaManual;
  digitalWrite(PinLedsInt,ledInteriorManual);
  digitalWrite(PinLedsInt2,ledInteriorManual);
  digitalWrite(PinLedsExt,ledExteriorManual);
}
void actualitzarValors(){
  if(servidor.hasArg("tempInt")){
    temperaturaInteriorManual = servidor.arg("tempInt").toFloat();
  }

  if(servidor.hasArg("tempExt")){
    temperaturaExteriorManual = servidor.arg("tempExt").toFloat();
  }

  if(servidor.hasArg("humInt")){
    humitatInteriorManual = servidor.arg("humInt").toFloat();
  }

  if(servidor.hasArg("humExt")){
    humitatExteriorManual = servidor.arg("humExt").toFloat();
  }

  if(servidor.hasArg("llum")){
    llumManual = servidor.arg("llum").toInt();
  }

  if(servidor.hasArg("pluja")){
    plujaManual = servidor.arg("pluja")=="1";
  }

  if(servidor.hasArg("finSud")){
    finestraSudManual = servidor.arg("finSud")=="1";
  }

  if(servidor.hasArg("finNord")){
    finestraNordManual = servidor.arg("finNord")=="1";
  }

  if(servidor.hasArg("menjador")){
    menjadorManual = servidor.arg("menjador")=="1";
  }

  if(servidor.hasArg("porta")){
    portaManual = servidor.arg("porta")=="1";
  }

  if(servidor.hasArg("ledInt")){
    ledInteriorManual = servidor.arg("ledInt")=="1";
  }

  if(servidor.hasArg("ledExt")){
    ledExteriorManual = servidor.arg("ledExt")=="1";
  }

  servidor.send(200,"text/plain","Valors actualitzats");
}
void iniciarWiFi(){
  WiFi.softAP(ssidAP,passwordAP);
  Serial.println("Punt d'accés creat");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  servidor.on("/", mostrarWeb);
  servidor.on("/actualitzar", actualitzarValors);
  servidor.on("/normal", [](){
    simulacio=false;
    domotic=false;
    modeWeb="normal";
    servidor.send(
      200,
      "text/plain",
      "Mode visualitzacio"
    );
  });


  servidor.on("/simulacio", [](){
    simulacio=true;
    domotic=false;
    modeWeb="simulacio";
    servidor.send(
      200,
      "text/plain",
      "Mode simulacio"
    );
  });

  servidor.on("/domotic", [](){
    simulacio=false;
    domotic=true;
    modeWeb="domotic";
    servidor.send(
      200,
      "text/plain",
      "Mode domotic"
    );
  });

  servidor.on("/ledInterior", [](){
    ledInteriorManual = servidor.arg("valor")=="1";
    servidor.send(
      200,
      "text/plain",
      "LED interior canviat"
    );
  });

  servidor.on("/ledExterior", [](){
    ledExteriorManual = servidor.arg("valor")=="1";
    servidor.send(
      200,
      "text/plain",
      "LED exterior canviat"
    );
  });

  servidor.on("/porta", [](){
    portaManual = servidor.arg("valor")=="1";
    servidor.send(
      200,
      "text/plain",
      "Porta canviada"
    );
  });


  servidor.on("/sud", [](){
    finestraSudManual = servidor.arg("valor")=="1";
    servidor.send(
      200,
      "text/plain",
      "Finestra sud canviada"
    );
  });

  servidor.on("/nord", [](){
    finestraNordManual = servidor.arg("valor")=="1";
    servidor.send(
      200,
      "text/plain",
      "Finestra nord canviada"
    );
  });

  servidor.on("/menjador", [](){
    menjadorManual = servidor.arg("valor")=="1";
    servidor.send(
      200,
      "text/plain",
      "Menjador canviat"
    );
  });

  servidor.begin();
}
void mostrarWeb(){
String pagina = R"rawliteral(
<!DOCTYPE html>
<html lang="ca">

<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>Maqueta de casa bioclimàtica</title>

<style>
*{
    margin:0;
    padding:0;
    box-sizing:border-box;
    font-family:"Segoe UI",Tahoma,Geneva,Verdana,sans-serif;
}
body{
    background:#F7F4ED;
    color:#37474F;
    line-height:1.5;
}
header{
    background:linear-gradient(135deg,#4F8A5B,#7BAE7F);
    color:white;
    padding:20px 40px;
    display:flex;
    justify-content:space-between;
    align-items:center;
    box-shadow:0 3px 10px rgba(0,0,0,.15);
}
.capcalera-text{
    display:flex;
    flex-direction:column;
}
header h2{
    font-size:23px;
    font-weight:600;
}
.nom{
    font-size:18px;
    margin-top:3px;
    opacity:.9;
}
#selectorModes{
    display:flex;
    gap:10px;
    flex-wrap:wrap;
    justify-content:center;
}
main{
    max-width:1100px;
    margin:35px auto;
    padding:20px;
}
.titol{
    text-align:center;
    margin-bottom:15px;
}
.titol h2{
    font-size:34px;
    color:#4F8A5B;
    margin-bottom:10px;
}
.titol p{
    color:#636161;
}
.categoria{
    background:white;
    border-radius:15px;
    padding:25px;
    margin-bottom:30px;
    box-shadow:0 6px 15px rgba(0,0,0,.08);
}
.meteorologia{border-left:8px solid #5E9C76;}
.portes{border-left:8px solid #9C7A4D;}
.llums{border-left:8px solid #fffd87;}
.solar{border-left:8px solid #f89a6e;}
.subtitol{
    font-size:24px;
    color:#4F8A5B;
    margin-bottom:18px;
    font-weight:bold;
}
ul{
    list-style:none;
}
li{
    display:flex;
    justify-content:space-between;
    align-items:center;
    padding:12px 0;
    border-bottom:1px solid #ECECEC;
}
li:last-child{
    border-bottom:none;
}
.dada{
    font-weight:600;
}
.valor{
    min-width:120px;
    text-align:center;
    padding:7px 14px;
    border-radius:25px;
    background:#E8F3E6;
    color:#2E5E37;
    border:1px solid #C7DEC1;
    font-weight:bold;
}
.boto-edicio{
    background:#E8F3E6;
    color:#2E5E37;
    border:1px solid #C7DEC1;
    padding:10px 20px;
    border-radius:25px;
    cursor:pointer;
    font-size:16px;
    font-weight:bold;
}
.boto-edicio:hover{
    background:#edf5eb;
}
.edicio{
    display:none;
}
.switch{
    width:50px;
    height:25px;
    appearance:none;
    background:#ccc;
    border-radius:25px;
    position:relative;
    cursor:pointer;
}
.switch:before{
    content:"";
    width:21px;
    height:21px;
    background:white;
    border-radius:50%;
    position:absolute;
    top:2px;
    left:3px;
    transition:.3s;
}
.switch:checked{
    background:#4F8A5B;
}
.switch:checked:before{
    transform:translateX(23px);
}
input,select{
    padding:7px;
    border-radius:10px;
    border:1px solid #ccc;
    text-align:center;
    font-weight:bold;
}
.boto-envia{
    display:none;
    margin:30px auto;
    margin-top:2px;
    background:#4F8A5B;
    color:white;
    border:none;
    padding:12px 30px;
    border-radius:25px;
    cursor:pointer;
    font-size:16px;
    font-weight:bold;
}
.boto-envia:hover{
    background:#3d7048;
}
footer{
    background:#37474F;
    color:white;
    text-align:center;
    padding:18px;
    margin-top:40px;
}
.indicador-mode{
    display:inline-block;
    margin-top:15px;
    padding:8px 20px;
    border-radius:25px;
    background:#E8F3E6;
    color:#2E5E37;
    font-weight:bold;
    border:1px solid #C7DEC1;
}
@media(max-width:700px){
header{
    flex-direction:column;
    gap:10px;
    text-align:center;
}
#selectorModes{
    flex-direction:column;
}
.titol h2{
    font-size:28px;
}
li{
    flex-direction:column;
    align-items:flex-start;
    gap:8px;
}
.valor{
    width:100%;
}
}
</style>

</head>

<body>

<header>
<div class="capcalera-text">
<h2>Treball de Recerca</h2>
<div class="nom">Marc Isern Abelló</div>
</div>

<div id="selectorModes">
<button id="botoNormal" class="boto-edicio" onclick="modeNormal()">🏠 Visualització</button>
<button id="botoSimulacio" class="boto-edicio" onclick="modeSimulacio()">📱 Simulació</button>
<button id="botoDomotic" class="boto-edicio" onclick="modeDomotic()">⚙️ Domòtic</button>
</div>

</header>

<main>

<div class="titol">
<h2>Maqueta de casa bioclimàtica amb domòtica integrada</h2>
<p>Control i visualització dels sensors i actuadors de la maqueta des del punt d'accés de l'ESP32</p>
<div id="modeActual" class="indicador-mode">🏠 Mode de visualització</div>
</div>

<button id="botoEnviar" class="boto-envia" onclick="enviarValors()">📤 Envia els valors a la placa ESP32</button>

<div class="categoria meteorologia zona-simulacio">
<div class="subtitol">🌦 Dades meteorològiques</div>

<ul>

<li>
<div class="dada">Temperatura interior</div>
<div class="valor visual">TEMP_INT °C</div>
<div class="edicio simulacio">
<input id="tempInt" type="number" value="TEMP_INT">
</div>
</li>

<li>
<div class="dada">Temperatura exterior</div>
<div class="valor visual">TEMP_EXT °C</div>
<div class="edicio simulacio">
<input id="tempExt" type="number" value="TEMP_EXT">
</div>
</li>

<li>
<div class="dada">Humitat interior</div>
<div class="valor visual">HUM_INT %</div>
<div class="edicio simulacio">
<input id="humInt" type="number" value="HUM_INT">
</div>
</li>

<li>
<div class="dada">Humitat exterior</div>
<div class="valor visual">HUM_EXT %</div>
<div class="edicio simulacio">
<input id="humExt" type="number" value="HUM_EXT">
</div>
</li>

<li>
<div class="dada">Pluja</div>
<div class="valor visual">PLUJA</div>
<div class="edicio simulacio">
<select id="pluja">
<option value="1">Sí</option>
<option value="0">No</option>
</select>
</div>
</li>

<li>
<div class="dada">Llum</div>
<div class="valor visual">LLUM/10</div>
<div class="edicio simulacio">
<input id="llum" type="number" min="0" max="10" value="LLUM">
</div>
</li>

<li class="nomes-normal">
<div class="dada">Estat meteorològic</div>
<div class="valor">CLIMA</div>
</li>

<li class="nomes-normal">
<div class="dada">Mode de funcionament</div>
<div class="valor">MODE</div>
</li>

</ul>
</div>

<div class="categoria llums zona-domotic">

<div class="subtitol">💡 Estat dels leds</div>

<ul>

<li>
<div class="dada">Llums interiors</div>
<div class="valor visual">LED_INT</div>
<div class="edicio domotic">
<input id="ledInt" class="switch" type="checkbox">
</div>
</li>

<li>
<div class="dada">Llums exteriors</div>
<div class="valor visual">LED_EXT</div>
<div class="edicio domotic">
<input id="ledExt" class="switch" type="checkbox">
</div>
</li>

</ul>

</div>

<div class="categoria portes zona-domotic">

<div class="subtitol">🚪 Estat de portes i finestres</div>

<ul>

<li>
<div class="dada">Porta principal</div>
<div class="valor visual">PORTA</div>
<div class="edicio domotic">
<input id="porta" class="switch" type="checkbox">
</div>
</li>

<li>
<div class="dada">Finestra sud</div>
<div class="valor visual">FIN_SUD</div>
<div class="edicio domotic">
<input id="finSud" class="switch" type="checkbox">
</div>
</li>

<li>
<div class="dada">Finestra nord</div>
<div class="valor visual">FIN_NORD</div>
<div class="edicio domotic">
<input id="finNord" class="switch" type="checkbox">
</div>
</li>

<li>
<div class="dada">Tancament del menjador</div>
<div class="valor visual">MENJADOR</div>
<div class="edicio domotic">
<input id="menjador" class="switch" type="checkbox">
</div>
</li>

</ul>

</div>

<div class="categoria solar zona-normal">

<div class="subtitol">☀ Energia solar</div>

<ul>

<li>
<div class="dada">Voltatge</div>
<div class="valor">VOLTS V</div>
</li>

<li>
<div class="dada">Intensitat</div>
<div class="valor">INTENSITAT mA</div>
</li>

<li>
<div class="dada">Potència</div>
<div class="valor">POTENCIA mW</div>
</li>

<li>
<div class="dada">Estat de la bateria</div>
<div class="valor">BATERIA</div>
</li>

</ul>

</div>

</main>

<footer>

<p>Marc Isern Abelló · Maqueta de casa bioclimàtica amb domòtica integrada</p>
<p>Treball de Recerca de Batxillerat · Curs 2026-2027</p>

</footer>

<script>

let modeActual="normal";

function modeNormal(){
fetch("/normal");
modeActual="normal";
actualitzarVista();
}

function modeSimulacio(){
fetch("/simulacio");
modeActual="simulacio";
actualitzarVista();
}

function modeDomotic(){
fetch("/domotic");
modeActual="domotic";
actualitzarVista();
}

function actualitzarVista(){

document.querySelectorAll(".categoria").forEach(e=>{
e.style.display="none";
});

document.querySelectorAll(".edicio").forEach(e=>{
e.style.display="none";
});

document.querySelectorAll(".visual").forEach(e=>{
e.style.display="block";
});

document.getElementById("botoEnviar").style.display="none";

if(modeActual=="normal"){

document.querySelectorAll(".categoria").forEach(e=>{
e.style.display="block";
});

}

if(modeActual=="simulacio"){

document.querySelector(".meteorologia").style.display="block";

document.querySelectorAll(".meteorologia .visual").forEach(e=>{
e.style.display="none";
});

document.querySelectorAll(".edicio.simulacio").forEach(e=>{
e.style.display="block";
});

document.querySelectorAll(".nomes-normal").forEach(e=>{
e.style.display="none";
});

document.getElementById("botoEnviar").style.display="block";

}

if(modeActual=="domotic"){

document.querySelector(".llums").style.display="block";
document.querySelector(".portes").style.display="block";

document.querySelectorAll(".llums .visual,.portes .visual").forEach(e=>{
e.style.display="none";
});

document.querySelectorAll(".edicio.domotic").forEach(e=>{
e.style.display="block";
});

document.getElementById("botoEnviar").style.display="block";

}

actualitzarBotons();

}
function actualitzarBotons(){

document.getElementById("botoNormal").style.display =
modeActual=="normal" ? "none":"block";

document.getElementById("botoSimulacio").style.display =
modeActual=="simulacio" ? "none":"block";

document.getElementById("botoDomotic").style.display =
modeActual=="domotic" ? "none":"block";


let indicador=document.getElementById("modeActual");


if(modeActual=="normal"){
indicador.innerHTML="🏠 Mode de visualització";
}

if(modeActual=="simulacio"){
indicador.innerHTML="📱 Mode de simulació";
}

if(modeActual=="domotic"){
indicador.innerHTML="⚙️ Mode de control domòtic";
}

}


function enviarValors(){
let url="/actualitzar?";
// Sensors simulació

url+="tempInt="+document.getElementById("tempInt").value;
url+="&tempExt="+document.getElementById("tempExt").value;
url+="&humInt="+document.getElementById("humInt").value;
url+="&humExt="+document.getElementById("humExt").value;
url+="&llum="+document.getElementById("llum").value;
let pluja=document.getElementById("pluja").value;
url+="&pluja="+pluja;


// Domòtica

url+="&ledInt="+
(document.getElementById("ledInt").checked?1:0);
url+="&ledExt="+
(document.getElementById("ledExt").checked?1:0);
url+="&porta="+
(document.getElementById("porta").checked?1:0);
url+="&finSud="+
(document.getElementById("finSud").checked?1:0);
url+="&finNord="+
(document.getElementById("finNord").checked?1:0);
url+="&menjador="+
(document.getElementById("menjador").checked?1:0);

fetch(url)
.then(res=>res.text())
.then(data=>{
console.log(data);
alert("Valors enviats a l'ESP32");
});

}



setInterval(()=>{
if(modeActual=="normal"){
location.reload();
}
},3000);

actualitzarVista();
</script>
</body>
</html>
)rawliteral";
// Substitució de valors dels sensors

pagina.replace("TEMP_INT",
String(temperaturainterior));

pagina.replace("TEMP_EXT",
String(temperaturaexterior));

pagina.replace("HUM_INT",
String(humitatinterior));

pagina.replace("HUM_EXT",
String(humitatexterior));

pagina.replace("PLUJA",
plou ? "Sí":"No");



int llum10=map(valorllum,0,4095,0,10);
pagina.replace("LLUM",
String(llum10));

pagina.replace("CLIMA",
estatClima);


if(modeEstiu){
pagina.replace("MODE",
"Estiu");
}
else{
pagina.replace("MODE",
"Hivern");
}

pagina.replace("LED_INT",
digitalRead(PinLedsInt) ?
"Encesos":"Apagats");
pagina.replace("LED_EXT",
digitalRead(PinLedsExt) ?
"Encesos":"Apagats");

pagina.replace("PORTA",
portaOberta ?
"Oberta":"Tancada");

pagina.replace("FIN_SUD",
finestraSudOberta ?
"Oberta":"Tancada");

pagina.replace("FIN_NORD",
finestraNordOberta ?
"Oberta":"Tancada");

pagina.replace("MENJADOR",
menjadorObert ?
"Oberta":"Tancada");

pagina.replace("VOLTS",
String(voltatgeBateria));

pagina.replace("INTENSITAT",
String(intensitat));

pagina.replace("POTENCIA",
String(potencia));

pagina.replace("BATERIA",
estatEnergia);
servidor.send(
  200,
  "text/html",
  pagina
);

}
