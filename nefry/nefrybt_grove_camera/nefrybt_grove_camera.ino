#include <Nefry.h>
#include <WiFiClientSecure.h>
HardwareSerial Serial1(1);

#define PIC_PKT_LEN    128        //data length of each read, dont set this too big because ram is limited
#define PIC_FMT_VGA    7
#define PIC_FMT_CIF    5
#define PIC_FMT_OCIF   3
#define CAM_ADDR       0
#define PIC_FMT        PIC_FMT_VGA
#define DELAY_TIME 3000 //3秒おきに撮影&アップロード
#define SEREAL_RX 19 //Nefry BT D3
#define SEREAL_TX 18 //Nefry BT D4

const byte cameraAddr = (CAM_ADDR << 5);  // addr
unsigned long picTotalLen = 0;            // picture length
unsigned long uptime;
const char* host = "192.168.0.2"; //ここにサーバーホストを指定
const char* page = "/";

/*********************************************************************/
void setup() {
  Nefry.print(F("POST to "));
  Nefry.print(host);
  Nefry.println(page);

  Serial1.begin(115200, SERIAL_8N1, SEREAL_RX, SEREAL_TX);
  initialize();
  preCapture();
}

/*********************************************************************/
void loop() {
  Nefry.setLed(0, 255, 0);
  delay(500);
  Capture();
  GetData();
  delay(DELAY_TIME);
}

void clearRxBuf() {
  while (Serial1.available()) {
    Serial1.read();
  }
}

void sendCmd(char cmd[], int cmd_len) {
  for (int i = 0; i < cmd_len; i++) Serial1.print(cmd[i]); 
}

void initialize() {   
  char cmd[] = {0xaa, 0x0d | cameraAddr, 0x00, 0x00, 0x00, 0x00};
  unsigned char resp[6];

  Nefry.print("initializing camera...");

  Serial1.setTimeout(500);
  while (true) {
    sendCmd(cmd, 6);
    if (Serial1.readBytes((char *)resp, 6) != 6) {
      Nefry.print(".");
      continue;
    }
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x0d && resp[4] == 0 && resp[5] == 0) {
      if (Serial1.readBytes((char *)resp, 6) != 6) continue; 
      if (resp[0] == 0xaa && resp[1] == (0x0d | cameraAddr) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) break; 
    }
  }
  cmd[1] = 0x0e | cameraAddr;
  cmd[2] = 0x0d;
  sendCmd(cmd, 6); 
  Nefry.println("\nCamera initialization done.");
}

void preCapture() {
  char cmd[] = { 0xaa, 0x01 | cameraAddr, 0x00, 0x07, 0x00, PIC_FMT };  
  unsigned char resp[6]; 
  
  while (true) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (Serial1.readBytes((char *)resp, 6) != 6) continue; 
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x01 && resp[4] == 0 && resp[5] == 0) break; 
  }
}

void Capture() {
  char cmd[] = { 0xaa, 0x06 | cameraAddr, 0x08, PIC_PKT_LEN & 0xff, (PIC_PKT_LEN>>8) & 0xff ,0}; 
  unsigned char resp[6];

  while (true) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (Serial1.readBytes((char *)resp, 6) != 6) continue;
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x06 && resp[4] == 0 && resp[5] == 0) break; 
  }
  cmd[1] = 0x05 | cameraAddr;
  cmd[2] = 0;
  cmd[3] = 0;
  cmd[4] = 0;
  cmd[5] = 0; 
  while (true) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (Serial1.readBytes((char *)resp, 6) != 6) continue;
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x05 && resp[4] == 0 && resp[5] == 0) break;
  }
  cmd[1] = 0x04 | cameraAddr;
  cmd[2] = 0x1;
  while (true) {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (Serial1.readBytes((char *)resp, 6) != 6) continue;
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x04 && resp[4] == 0 && resp[5] == 0) {
      Serial1.setTimeout(1000);
      if (Serial1.readBytes((char *)resp, 6) != 6) continue;
      if (resp[0] == 0xaa && resp[1] == (0x0a | cameraAddr) && resp[2] == 0x01) {
        picTotalLen = (resp[3]) | (resp[4] << 8) | (resp[5] << 16);
        Nefry.print("picTotalLen:");
        Nefry.println(picTotalLen);
        break;
      }
    }
  }
}

void GetData() {
  unsigned int pktCnt = (picTotalLen) / (PIC_PKT_LEN - 6); 
  if ((picTotalLen % (PIC_PKT_LEN-6)) != 0) pktCnt += 1;
  
  char cmd[] = { 0xaa, 0x0e | cameraAddr, 0x00, 0x00, 0x00, 0x00 };  
  unsigned char pkt[PIC_PKT_LEN];

  WiFiClient client;
  reconnect:
  Nefry.setLed(255, 0, 0);
  Nefry.println("Try to connect..");
  if (!client.connect(host, 80)) {
    Nefry.setLed(0, 0, 0);
    client.stop();
    delay(1000);
    goto reconnect;
  }

  Nefry.println("SendPict()..");
  Nefry.setLed(255, 0, 0);

  client.print(String("POST ") + page + F(" HTTP/1.1\n") +
               F("Host: ") + host + F("\n") +
               F("Content-Type: image/jpeg\n") +
               F("Content-Length: ") + String(picTotalLen) + F("\n") +
               F("Connection: close\n\n"));
  Nefry.print(F("Content-Length: ")); Nefry.println(picTotalLen);
  Nefry.print(F("HTTP Sending..... "));

  Serial1.setTimeout(1000);
  for (unsigned int i = 0; i < pktCnt; i++) {
    Nefry.print(".");
    cmd[4] = i & 0xff;
    cmd[5] = (i >> 8) & 0xff;
    
    int retry_cnt = 0;
    retry:
    delay(10);
    clearRxBuf();
    sendCmd(cmd, 6);
    uint16_t cnt = Serial1.readBytes((char *)pkt, PIC_PKT_LEN);
    unsigned char sum = 0; 
    for (int y = 0; y < cnt - 2; y++) {
      sum += pkt[y];
    }
    if (sum != pkt[cnt-2]) {
      if (++retry_cnt < 100) goto retry;
      else break;
    }
      
    client.write((const uint8_t *)&pkt[4], cnt-6);
    client.flush();
  }
  cmd[4] = 0xf0;
  cmd[5] = 0xf0; 
  sendCmd(cmd, 6); 

  delay(1000);
  if (client.available()) {
    String line = client.readStringUntil('\r');
    Nefry.print(F("Responce: ")); Nefry.println(line);
  }
  client.stop();
  Nefry.println("Picture sent.");
}