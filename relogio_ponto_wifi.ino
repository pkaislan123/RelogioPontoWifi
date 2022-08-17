// ############# LIBRARIES ############### //
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <MFRC522.h>
#include <SPI.h>
#include <NTPClient.h>
#include <SoftwareSerial.h>
#include <WiFiUdp.h>


// ############# VARIABLES ############### //

constexpr uint8_t RST_PIN = D3;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;     // Configurable, see typical pin layout above



MFRC522 mfrc522(SS_PIN, RST_PIN);
const char* SSID = "FIBRA"; // rede wifi
const char* PASSWORD = "0124578963"; // senha da rede wifi

String BASE_URL = "http://192.168.100.86:8080/";
WiFiClient wifiClient;


SoftwareSerial mySerial(D1, D0); // RX, TX


// ############# PROTOTYPES ############### //

void initSerial();
void initWiFi();
void httpRequest(String path);


struct Date {
  int dayOfWeek;
  int day;
  int month;
  int year;
  int hours;
  int minutes;
  int seconds;
};
// ############### OBJECTS ################# //

WiFiClient client;
HTTPClient http;
WiFiUDP udp;

NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);
// ############## SETUP ################# //

char* dayOfWeekNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

unsigned long millisHorarLogoHora = millis();
String  formattedDate;

boolean ativo = false;
int id_cliente_ativo = 0;
int id_cartao_ativo = 0;

unsigned long millisTarefaRecepcao = millis();


void setup() {



  SPI.begin();              //INICIA A BIBLIOTECA SPI
  mfrc522.PCD_Init();
  Wire.begin(D2, D1);

  initSerial();
  initWiFi();


  ntp.begin();//Inicia o NTP.
  ntp.forceUpdate();//Força o Update.  // Set offset time in seconds to adjust for your timezone, for example:



  Serial.begin(115200);
  mySerial.begin(38400);


}

// ############### LOOP ################# //

void loop() {
  // mostrarLogoHora();

  if (!ativo)
    ficarDisponivel();

  if (ativo) {
    //ler valores do arduino
    if (mySerial.available()) {

      String recebido = mySerial.readString();
      Serial.print("Dado recebido do arduino: ");
      Serial.println(recebido);
      millisTarefaRecepcao = millis();



      int asterisco = recebido.indexOf("*");
      int hashtag = recebido.indexOf("#");
      int arroba = recebido.indexOf("@");
      int ecomercial = recebido.indexOf("&");

      String sTensao = recebido.substring(0, asterisco);
      String sCorrente  = recebido.substring(asterisco + 1,  hashtag);
      String sPotenciaW  = recebido.substring(hashtag + 1,  arroba);
      String sConsumoAtual  = recebido.substring(arroba + 1,  ecomercial);

      String texto =   "Tensão: " + sTensao + "V "
                       + " Corrente: " + sCorrente + "A "
                       + " Potencia(w): " + sPotenciaW + "w "
                       + " Consumo Atual: " +  sConsumoAtual + "kWh ";


      double dTensao = sTensao.toDouble();
      double dCorrente = sCorrente.toDouble();
      double dPotencia = sPotenciaW.toDouble();
      double dConsumoAtual = sConsumoAtual.toDouble();

      String url_inserir_registro_consumo =  "v1/protected/cliente/inserir_registro_consumo";
      String inserido = inserirRegistroConsumo(url_inserir_registro_consumo, sTensao, sCorrente, sPotenciaW, sConsumoAtual);

      Serial.println(texto);


    }

    if ((millis() - millisTarefaRecepcao) > 10000) {

      Serial.println("Sem sinal do arduino");
      ativo = false;
      mySerial.println("stop#*");

    }

  }



}

void ficarDisponivel() {
  String uid = Leitura();

  if (uid != "") {
    Serial.print("Buscando Colaborador, uid: ");
    Serial.println(uid);
    //  limparLinhas();
    mySerial.println("*" + uid + "#");


    String url_buscar_por_uid =  "v1/protected/cliente/buscarporuid";
    uid.trim();
    String resposta = buscarClientePorUID(url_buscar_por_uid, uid);

    resposta.replace("\"", "");
    resposta.replace(":", "");
    resposta.replace("{", "");
    resposta.replace("}", "");
    resposta.replace("resposta", "");

    int asterisco = resposta.indexOf("*");
    int hashtag = resposta.indexOf("#");
    String nome_colaborador = resposta.substring(0, asterisco);
    String id  = resposta.substring(asterisco + 1,  hashtag);
    String id_cartao  = resposta.substring(hashtag + 1,  resposta.length());

    // limparLinhas();
    Serial.print("Nome Cliente: ");
    Serial.println(nome_colaborador);

    Serial.print("ID Cliente: ");
    Serial.println(id);

    Serial.print("ID Cartao: ");
    Serial.println(id_cartao);

    int int_id = id.toInt();
    if (int_id > 0) {

      int int_id_cartao = id_cartao.toInt();
      id_cartao_ativo = int_id_cartao;


      ativo = true;
      id_cliente_ativo = int_id;
      mySerial.println("ativar$%");
      millisTarefaRecepcao = millis();

    }

    // delay(1000);


  }

}


/*
  void limparLinhas() {
  //limpar linha 3
  for (int coluna = 0; coluna <= 19; coluna++) {
    lcd.setCursor(coluna, 2);
    lcd.print(" ");
  }

  //limpar linha 4
  for (int coluna = 0; coluna <= 19; coluna++) {
    lcd.setCursor(coluna, 3);
    lcd.print(" ");
  }
  }
*/
String buscarClientePorUID(String path, String uid)
{
  http.begin(wifiClient, BASE_URL + path);
  http.addHeader("content-type", "application/json");
  String body = "{ \"uid\": \"" + uid + "\"}";
  int httpCode = http.POST(body);
  if (httpCode < 0) {
    Serial.println("request error - " + httpCode);
    return "request error - " + httpCode;
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("request error - " + httpCode);
    return "";
  }
  String response =  http.getString();
  http.end();

  return response;
}


String inserirRegistroConsumo(String path, String tensao, String corrente, String potencia, String consumo_atual)
{
  http.begin(wifiClient, BASE_URL + path);
  http.addHeader("content-type", "application/json");
  //String body = "{ \"id_cliente\": \"" + id_cliente_ativo + "\",\"id_cartao\":" + id_cartao + "\",       }";

  String sIdCliente = String(id_cliente_ativo);
  String sIdCartao = String(id_cartao_ativo);

  String body = "{\"id_cliente\":\"" + sIdCliente +  "\", \"id_cartao\":\"" + sIdCartao + "\", \"tensao\":\"" + tensao + "\", \"corrente\":\"" + corrente + "\", \"potencia\":\"" + potencia + "\", \"consumo_atual\":\"" + consumo_atual + "\"}";
  
  //  String body = "{\"id_cliente\":\"" + sIdCliente +  "\", \"id_cartao\":\"" + sIdCartao + "\", \"tensao\":\"" + tensao + "\", \"corrente\":\"" + corrente + "\", \"potencia\":\"" + potencia + "\"}";

  int httpCode = http.POST(body);
  if (httpCode < 0) {
    Serial.println("request error - " + httpCode);
    return "request error - " + httpCode;
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("request error - " + httpCode);
    return "";
  }
  String response =  http.getString();
  http.end();

  return response;
}



String makeLogin(String path)
{
  http.begin(wifiClient, BASE_URL + path);
  http.addHeader("content-type", "application/json");
  String body = "{\"login\":\"aislan1\",\"senha\":\"1234\"}";
  int httpCode = http.POST(body);
  if (httpCode < 0) {
    Serial.println("request error - " + httpCode);
    return "";
  }
  if (httpCode != HTTP_CODE_OK) {
    return "";
  }
  String response =  http.getString();
  http.end();

  return response;
}


void initSerial() {
  Serial.begin(115200);
}

void initWiFi() {
  delay(1000);
  // lcd.setCursor(0, 0);
  // lcd.print("Aguarde....");
  // lcd.setCursor(0, 1);
  // lcd.print("Conectando Wifi....");
  Serial.println("Conectando-se em: " + String(SSID));

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado na Rede " + String(SSID) + " | IP => ");
  Serial.println(WiFi.localIP());
  delay(1000);
  // lcd.clear();

}


String Leitura()
{
  if ( ! mfrc522.PICC_IsNewCardPresent())    //ESCUTANDO LEITOR
  {
    return "";                                 //RETORNA SE nao DETECTOU ALGO
  }
  if ( ! mfrc522.PICC_ReadCardSerial())      //AQUI FAZ A LEITURA SERIADO DO TAG OU CARTÃO
  {
    return "";
  }
  //LIMPA O LCD
  String Codigo = "";                        //CRIAMOS UMA VARIÁVEL LOCAL PARA ARMAZENAR O CODIGO DOS TAGS OU CARTÃO
  for (byte i = 0; i < mfrc522.uid.size; i++) //LAÇO FOR PARA INCREMENTAR I COM O UID7
  {
    Codigo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "" : "")); //CRIAR A STRING COM O CÓDIGO DA TAG OU CARTÃO
    Codigo.concat(String(mfrc522.uid.uidByte[i], DEC));                //CONVERTE PARA HEXA DECIMAL
    Codigo.toUpperCase();                                              //CONVERTE AS LETRAS DO HEXA PARA MAIÚSCULAS

  }
  Codigo.replace(" ", "");
  Serial.println(Codigo);
  return Codigo;

}

void mostrarLogoHora() {

  if ((millis() - millisHorarLogoHora) > 10000) {

    // lcd.setCursor(0, 0);
    // lcd.print("LD Armazens");
    // lcd.setCursor(0, 1);

    mySerial.print("(" + getDataHoraCompleta() + ")");

    millisHorarLogoHora = millis();


  }
}

String getDataHoraCompleta() {

  while (!ntp.update()) {
    ntp.forceUpdate();
  }

  String hora = ntp.getFormattedTime();//Armazena na váriavel HORA, o hor
  String data = ntp.getFormattedDate();

  String data_completa =  data + " " + hora;
  Serial.print("\ndata: ");
  Serial.println(data_completa);
  return data_completa;

}
