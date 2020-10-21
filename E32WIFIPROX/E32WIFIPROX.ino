#include <WiFi.h>
#include <Wire.h>
#include "esp_wifi.h"
#include <SPI.h>
#define LED_PIN 5

#include "EspMQTTClient.h"

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;
int loopcnt;
int listcount = 0;

String KnownMac[10][5] = {  // Put device MAC addresses you want to be reconized.  Use LANSCAN or other tools to identify MAC addresses
  {"Person 1","MAC1","0","START","1"},
  {"Person 2","MAC2","0","START","1"},
  {"Person 3","MAC3","0","START","1"},
  {"Person 4","MAC4","0","START","1"},
  {"NAME","MACADDRESS","","AWAY","0"},
  {"NAME","MACADDRESS","","AWAY","0"},
  {"NAME","MACADDRESS","","AWAY","0"},
  {"NAME","MACADDRESS","","AWAY","0"},
  {"NAME","MACADDRESS","","AWAY","0"}
  
};  // Note that status defaults to START to avoid any erroneous NodeRed status changes on restart
    //Home automation system looks only for HOME or AWAY so START is ignored

EspMQTTClient client( // Update SSID, Password and MQTT IP Address
  "SSID",
  "WIFIPWD",
  "MQTT IP ADDRESS",  // MQTT Broker server ip
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed
  "TestClient",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);


String defaultTTL = "900"; // Maximum time (seconds) elapsed before device is identified as offline


const wifi_promiscuous_filter_t filt={ //Idk what this does
    .filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
};

typedef struct { // or this
  uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct {
  int16_t fctl;
  int16_t duration;
  MacAddr da;
  MacAddr sa;
  MacAddr bssid;
  int16_t seqctl;
  unsigned char payload[50];
} __attribute__((packed)) WifiMgmtHdr;

#define maxCh 13 //max Channel -> US = 11, EU = 13, Japan = 14
int curChannel = 1;

void setup() {

  /* start Serial */
  Serial.begin(115200);
  /* setup wifi */
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("starting!");
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) { //This is where packets end up after they get sniffed
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf; // Dont know what these 3 lines do
  int len = p->rx_ctrl.sig_len;
  WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;
  //len -= sizeof(WifiMgmtHdr);
  //if (len < 0){
  //  Serial.println("Received 0");
  //  return;
  //}
  String packet;
  String mac;
  String shex;

  uint8_t itmp;
  int fctl = ntohs(wh->fctl);
  for(int i=0;i<=45;i++){ // This reads the first part of the packet
    shex = String(p->payload[i],HEX);
    if (shex.length() == 1) shex = "0" + shex; // This ensures leading 0 for HEX values is not lost
    packet += shex;
  }
  //Serial.println(packet);
  
  for(int i=32;i<=43;i++){ // This removes the surrounding bits so we only get the mac address
    mac += packet[i];
    //Serial.println(mac);
  }
  
  mac.toUpperCase();
  
  for(int i=0;i<=9;i++){ // checks if the MAC address has been added before
    if(mac == KnownMac[i][1]){
      KnownMac[i][2] = "0"; // Sets time counter to 0
      if (!(KnownMac[i][3] == "HOME")) { // If MAC detected is in the KnownMac list, the status is changed to HOME
        KnownMac[i][3] = "HOME";
        KnownMac[i][4] = "1"; // Change Flag set to 1       
        //client.publish(KnownMac[i][0], KnownMac[i][3]);
        Serial.println("Found Known MAC");
      }   
    }
  }
}

void addtime(){ // This maanges the TTL
  for(int i=0;i<=9;i++){
    if(!(KnownMac[i][2] == "")){  // This incremenets the time and sets status to AWAY if threshold is exceeded
      int ttl = (KnownMac[i][2].toInt());
      ttl ++;
      if((ttl >= defaultTTL.toInt()) * (!(KnownMac[i][3] == "AWAY"))) {
        KnownMac[i][3] = "AWAY";
        KnownMac[i][4] = "1"; // Change Flag Set to 1
        KnownMac[i][2] = "0"; //Resets time counter to 0
      }else{
        KnownMac[i][2] = String(ttl);
      }
    }
  }
}

void publishstatus()
{

  for(int j=0;j<=9;j++){ // Publishes the status to NodeRed if the Change Flag is set to 1
    if (KnownMac[j][4] == "1") {
      delay(500);
      client.publish(KnownMac[j][0], KnownMac[j][3]);
      delay(500);
      Serial.println(KnownMac[j][0] + " : " + KnownMac[j][3] + " : " + KnownMac[j][4] + "\n --");
      KnownMac[j][4] = "0";
    }
  }
  
}

void onConnectionEstablished()
{
  // Publish a message to "mytopic/test"
  //client.publish("spark", "Connected...");
  for(int j=0;j<=9;j++){
    if (KnownMac[j][4] == "1") {
      delay(500);
      client.publish(KnownMac[j][0], KnownMac[j][3]);
      delay(500);
      Serial.println(KnownMac[j][0] + " : " + KnownMac[j][3] + " : " + KnownMac[j][4] + "\n --");
      KnownMac[j][4] = "0";
    }
  }
}

//===== LOOP =====//
void loop() {

  client.enableMQTTPersistence();
  client.loop();
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  addtime();
  publishstatus();
  curChannel++;
  
  digitalWrite(LED_PIN, LOW); 

  loopcnt++;
  if (loopcnt == 9000) { // Restarts every 9000 iterations to avoid memory issues.  When starting, the START status prevents unintended events in Home Automation
    ESP.restart();
    client.publish("spark", "Restarting");
  }
  client.publish("spark", "Loop " + String(loopcnt) + " : Person 1-" + KnownMac[0][2] + " : Person 2-" + KnownMac[1][2] + " : Person 3-" + KnownMac[2][2] + " : Person 4-" + KnownMac[3][2]);  
}
