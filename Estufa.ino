#if defined(ESP8266)
#include <ESP8266WiFi.h>  //ESP8266 Core WiFi Library         
#else
#include <WiFi.h>      //ESP32 Core WiFi Library    
#endif

#include <DHT.h>
#include <DHT_U.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#define PINO_VENTOINHA_UMIDADE D7
#define PINO_AUMENTA_TEMPERATURA D5
#define PINO_DHT D0
#define DHTTYPE DHT11

//Variáveis de configuração ---------
  //Wifi
  const char* ssid = "";
  const char* wifiPassword = "";
  
  //MQTT
  const char* mqttServer = "";
  const char* mqttUser = "";
  const char* mqttPassword = "";
  const int mqttPort = 0;
  
  //Topicos MQTT
  const char* mqttTopicoTemperaturaAtual = "TemperaturaNodeMCU";
  const char* mqttTopicoTemperaturaConfiguracao = "TemperaturaApp";
  const char* mqttTopicoUmidadeAtual = "UmidadeNodeMCU";
  const char* mqttTopicoUmidadeConfiguracao = "UmidadeApp";
  
  //Niveis padroes
  int temperaturaMinima = 18;
  int umidadeMinima = 60;
//-----------------------------------


DHT dht(PINO_DHT, DHTTYPE);
WiFiManager wifiManager; //Cria tela para informar conexão wifi
WiFiClient espWifiClient;
PubSubClient clientMqtt(espWifiClient);

void Executa(float, float);
void configModeCallback(WiFiManager*);
void saveConfigCallback();
void callbackMqtt(char*, byte*, unsigned int);

void ConfiguraWifi();
void ConfiguraClienteMqtt();

void configuraTemperatura(char*);
void configuraUmidade(char*);

void setup() {
  Serial.println("----------- Iniciando execução ----------");
  Serial.begin(9600);
  dht.begin();

  ConfiguraWifi();
  ConfiguraClienteMqtt();

  clientMqtt.subscribe(mqttTopicoTemperaturaConfiguracao);
  clientMqtt.subscribe(mqttTopicoUmidadeConfiguracao);
  Serial.println("\n\n------> ESP iniciado com sucesso!");
}

void ConfiguraWifi()
{
  char* hotspotNetworkName = "ESP_AP";
  char* passwordHotspot = "12345678";

  Serial.println("Iniciando configuração de hotspot wifi...");

  wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.autoConnect(hotspotNetworkName, passwordHotspot);
  Serial.println("Hotspot configurado com sucesso.");
  Serial.print("Nome da rede: ");
  Serial.print(hotspotNetworkName);
  Serial.print("  -----  Senha: ");
  Serial.println(passwordHotspot);

  if (!wifiManager.startConfigPortal(hotspotNetworkName, passwordHotspot))
  {
    Serial.println("Falha na inicialização da página de conexão wifi. O sistema será reiniciado.");
    delay(1000);
    ESP.restart();
  }

  Serial.println("\n\nConexão com wifi estabelecida com sucesso!");
}

void ConfiguraClienteMqtt()
{
  Serial.println("\nIniciando conexão com servidor MQTT...");

  clientMqtt.setServer(mqttServer, mqttPort);
  clientMqtt.setCallback(callbackMqtt);

  do
  {
    Serial.println("Conectanto ao Broker MQTT...");
    if (clientMqtt.connect("ESP_Estufa_1", mqttUser, mqttPassword))
    {
      Serial.println("\n\nConectado ao Broker");
    }
    else
    {
      Serial.print("Erro ao conectar ao broker: ");
      Serial.println(clientMqtt.state());
      delay(2000);
    }
  } while (!clientMqtt.connected());
}

void loop() {
  static int contadorDeExecucao = 0;

  delay(1000);
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  
  char* strTemperatura;
  char* strUmidade;
  sprintf(strTemperatura, "%.2f", temperatura);
  sprintf(strUmidade, "%.2f", umidade);
  
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Falha na leitura do sensor");
  }
  else {
    clientMqtt.publish(mqttTopicoTemperaturaAtual, strTemperatura);
    clientMqtt.publish(mqttTopicoUmidadeAtual, strUmidade);
    Executa(temperatura, umidade);
  }

  contadorDeExecucao++;
  if (contadorDeExecucao >= 10)
  {
    Serial.println("ESP em execução");
    contadorDeExecucao = 0;
  }
}

void Executa(float temperatura, float umidade) {
  //Verifica se a temperatura está abaixo de temperaturaMinima
  if (temperatura < temperaturaMinima) {
    digitalWrite(PINO_AUMENTA_TEMPERATURA, HIGH);
  }
  else
  {
    digitalWrite(PINO_AUMENTA_TEMPERATURA, LOW);
  }

  //Verifica se a umidade está abaixo de umidadeMinima
  if (umidade < umidadeMinima) {
    digitalWrite(PINO_VENTOINHA_UMIDADE, HIGH);
  }
  else
  {
    digitalWrite(PINO_VENTOINHA_UMIDADE, LOW);
  }
}

void callbackMqtt(char* topic, byte* data, unsigned int length)
{
  Serial.println("\nIniciado tratamento de callback");

  data[length] = '\0';


  if (strcmp(topic, mqttTopicoTemperaturaConfiguracao) == 0) configuraTemperatura((char*)data);
  if (strcmp(topic, mqttTopicoUmidadeConfiguracao) == 0) configuraUmidade((char*)data);
}

void configuraTemperatura(char* strTemperatura)
{
  temperaturaMinima = atoi(strTemperatura);
}

void configuraUmidade(char* strUmidade) {
  umidadeMinima = atoi(strUmidade);
}

void configModeCallback(WiFiManager manager)
{
  
}

void saveConfigCallback()
{
  
}
