
/**
  @brief Take a picture every 2 seconds and write it out to the SD Card
  @author Michael A. King
 * /
 
//  For more details about the product please check http://www.seeedstudio.com/depot/

/* arduino includes */
#include <SD.h>
#include <SPI.h> // SPI.h is included for Sd2Card.cpp (this is a bug)

File myFile;
#define PIC_BUF_LEN 96        //data length of each read

char cmdRST[] = {
  0x56,0x00,0x26,0x00};        //Reset command
char cmdCAP[] = {      
  0x56,0x00,0x36,0x01,0x00};  //Take picture command
char cmdGetLen[] = {
  0x56,0x00,0x34,0x01,0x00};   //Read JPEG file size command
char cmdGetDat[] = {           //Read JPEG data command
  0x56,0x00,0x32,0x0c,0x00,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a}; 
char cmdConti[] = {           
  0x56,0x00,0x36,0x01,0x02};   //Take next picture command
  
unsigned int picTotalLen = 0;  // picture length
int picNameNum = 0;

void setup()
{
  Serial.begin(115200); 
  Serial.println("Initializing SD card....");
  pinMode(10,OUTPUT);          // CS pin of SD Card Shield
  
  if (!SD.begin(10)) {
    Serial.print("initialzation failed");
    return;
  } else {
    Serial.println("initialization done.");
    Restart();
  }
} // end setup()

void loop()
{
  int n = 0;
  while(1) {
    Serial.flush();
    delay(2000); // delay for 2 seconds
    if (n == 0) Capture();
    else ContCapture();
    GetData();
    Serial.print("Taking pictures success ,number : ");
    Serial.println(n);
    n ++ ;
  }  
} // end loop()
  
void Restart()
{
   sendCmd(cmdRST,4);
   delay(1000);
   while(Serial.available() > 0)Serial.write(Serial.read());
   Serial.println("Camera initialization done.");
} // end Restart()

void Capture()
{
  sendCmd(cmdCAP,5);
  delay(50);
  while(Serial.available() > 0){
    Serial.read();
  }
} // end Capture()

void GetData()
{
  unsigned int picTotalLen;
  sendCmd(cmdGetLen,5);
  delay(50);
  for(char i = 0; i < 7; i++){
    Serial.read();
  }
  picTotalLen = 0;
  int high = Serial.read();
  picTotalLen |= high ;
  int low  = Serial.read();
  picTotalLen = picTotalLen << 8 ;
  picTotalLen |= low ;                      //get the length of picture 
  
  unsigned int addr = 0;
  cmdGetDat[8] = 0;
  cmdGetDat[9] = 0;
  unsigned int count = picTotalLen / PIC_BUF_LEN;
  char tail = picTotalLen % PIC_BUF_LEN;
  cmdGetDat[13] = PIC_BUF_LEN;              //the length of each read data
  
  char picName[] = "pic00.jpg";
  picName[3] = picNameNum/10 + '0';
  picName[4] = picNameNum%10 + '0';
  
  myFile = SD.open(picName, FILE_WRITE);
  if(!myFile){
    Serial.println("myFile open fail...");
  }
  else{
    for(char i = 0; i < count; i++){      //get and save count*PIC_BUF_LEN data
      sendCmd(cmdGetDat,16);
      delay(10);
      readCamSaveToFile(myFile, PIC_BUF_LEN);
      addr += PIC_BUF_LEN;
      cmdGetDat[8] = addr >> 8;
      cmdGetDat[9] = addr & 0x00FF;
    }
    cmdGetDat[13] = tail;                  //get rest part of the pic data
    sendCmd(cmdGetDat,16);
    delay(10);
    readCamSaveToFile(myFile, tail);      
  }
  myFile.close();
  picNameNum ++;
} // end GetData()

void ContCapture()
{
  sendCmd(cmdConti,5);
  delay(50);
  while(Serial.available() > 0){
    Serial.write(Serial.read());
  }
} // end ContCapture()

void readCamSaveToFile(File &myFile,int toBeReadLen)
{
  char readLen = 0;
  for(char i = 0;i < 5;i ++){//read the signal that tells us that it's ready to send data 
    Serial.read();
  }
  while(readLen < toBeReadLen){//read and store the JPG data that starts with 0xFF 0xDB
      myFile.write(Serial.read());
      readLen++;
  }
  for(char i = 0;i < 5;i ++){ //read the signal of sussessful sending
    Serial.read();
  }
} // end readCamSaveToFile()

void sendCmd(char cmd[] ,int cmd_len)
{
  for(char i = 0; i < cmd_len; i++)Serial.print(cmd[i]);  
} // end sendCmd()

