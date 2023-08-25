#include <WiFi.h>
#include "config_edit_me.h"
#include "minecraft.h"

typedef struct{
    WiFiClient socket;
    uint8_t id;
} clients;

clients serverClients[MAX_PLAYERS];
TaskHandle_t listener;
WiFiServer server(server_port);
minecraft mc;

int timeoutTime = 2000;

void serverHandler(void * parameter){
    while(1){
        mc.handle();
        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}

void playerHandler(void * parameter){
    clients client = *(clients*)parameter;

    mc.players[client.id].loginfo("started task " + String(client.id) + " pinned to core " + String(xPortGetCoreID()));

    if(!mc.players[client.id].join()){  // try to join, end task if fail
        goto end;
    }

    while (client.socket.connected()) {  // if client timeouts end task
    // while (true) {
        mc.players[client.id].handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    mc.broadcastEntityDestroy(mc.players[client.id].id);
    mc.broadcastChatMessage(mc.players[client.id].username + " left the server", "Server");
    end:
    mc.players[client.id].loginfo("client " + String(client.id) + " disconnected");
    client.socket.stop();
    mc.players[client.id].connected = false;
    vTaskDelete(NULL);
}

void setup() {
    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();

    Serial.begin(115200);
    delay(100);
    WiFi.mode(WIFI_AP); 
  WiFi.softAP(ssid, password); 
  Serial.println("Wait 100 ms for AP_START...");
  delay(100);
  Serial.println("Setting the AP");
  IPAddress Ip(192, 168, 123, 1); 
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
    Serial.println("Connected to the WiFi network");
    Serial.println(WiFi.localIP());

    delay(1000);

    for(int i = 0; i < MAX_PLAYERS; i++){
        mc.players[i].S = &serverClients[i].socket;
        mc.players[i].id = i;
        serverClients[i].id = i;
        mc.players[i].mc = &mc;
    }

    xTaskCreatePinnedToCore(serverHandler, "main_task", 10000, NULL, 2, NULL, 1);

    server.begin();
    server.setTimeout(1);
    Serial.println("[INFO] server started");
}

void loop(){
    uint8_t i;
    //check if there are any new clients
    if (server.hasClient()){
        for(i = 0; i < MAX_PLAYERS; i++){
            //find free/disconnected spot
            if (!serverClients[i].socket || !serverClients[i].socket.connected()){
                if(serverClients[i].socket) serverClients[i].socket.stop();
                serverClients[i].socket = server.available();
                Serial.print("[INFO] New client connected: "); Serial.println(i);
                char name[20];
                snprintf(name, 20, "playerHandler%d", i);
                xTaskCreatePinnedToCore(playerHandler, name, 50000, (void*)&serverClients[i], 2, NULL, i % 2);
                return;  // restart loop
            }
        }
        //no free/disconnected spot so reject
        WiFiClient serverClient = server.available();
        serverClient.stop();
        Serial.println("[INFO] server is full!");
    }
}
