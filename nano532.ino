//Nano532 Final Version Commit Jan.20 2018
unsigned char receive_ACK[35];
unsigned char UID1[4]={0x3B,0x29,0xEE,0xAD};
unsigned char key1[6]={0x2E,0xE7,0xFB,0xD3,0x04,0x4C};//G.MFD
unsigned char UID2[4]={0xBB,0x9E,0xDD,0xAD};
unsigned char key2[6]={0xAE,0x72,0xDB,0xD3,0x68,0x48};//P.MFD
unsigned char dataWriteIntoCard[16]={0x11,0x27,0x00,0x00,0x00,0x00,0x15,0x10,0x27,0x00,0x00,0x00,0xE7,0x00,0x5A,0xC5};

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); //Activate LED_L on Pin13
  wakeUp();
  delay(10);
  readAck(15);
  //Success response: 00 00 FF 00 FF 00 00 00 FF 02 FE D5 15 16 00 
  if(receive_ACK[13] == 0x16) blink();//Blink on success
}

void loop()
{ 
  Scan();
  switch(receive_ACK[13]) //judge first byte of UID
  {
    case 0x3b://Card G
      if(passWordCheck(0x09,UID1,key1)==1) 
      {
        writeData(0x09,dataWriteIntoCard);
        writeData(0x0a,dataWriteIntoCard);  
      }
      break;
    case 0xbb://Card P
      if(passWordCheck(0x09,UID2,key2)==1) 
      {
        writeData(0x09,dataWriteIntoCard);
        writeData(0x0a,dataWriteIntoCard);
      }
      break;
    default:break;
  }
  delay(10);
}

void blink()
{
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                        // wait for 0.5 second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(500);                        // wait for 0.5 second                 
}

void wakeUp()
{
  const unsigned char wake[24]={
  0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03, 0xfd, 0xd4, 0x14, 0x01, 0x17, 0x00};//wake up NFC module
  for(int i=0;i<24;i++) //send command
  {
    UART_Send_Byte(wake[i]);
  }
}

void Scan()
{
  const unsigned char cmd[11]={ 0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00};
  //0 0 FF 0 FF 0 0 0 FF C F4 D5 4B 1 1 0 4 8 4 D1 AA 40 EA 29 0 
  //0 0 FF 0 FF 0 ----ACK
  //0 0 FF C F4 
  //D5---PN532 to Arduino
  //4B----respond command 
  //1 1----target ID  and target amount
  //0 4----atq 
  //8----capacity of the card  is 8K
  //4 ---- 4 numbers of the UID
  //D1 AA 40 EA----UID 
  //29 0------DCS  POST---  DCS=0xff&(SUM(0 0 FF C F4 D5 4B 1 1 0 4 8 4 D1 AA 40 EA))
  for(int i=0;i<11;i++)  UART_Send_Byte(cmd[i]);
  delay(10); 
  readAck(25); 
  delay(10); 
}

int passWordCheck(int block,unsigned char id[],unsigned char st[])
{
  //-------head------ cmd  card 1  check blocknumber  password               UID D1 AA 40 EA      DCS+POST
  //00 00 FF 0F F1 D4  40    01     60    07          FF FF FF FF FF FF       02 F5 13 BE           C2 00 
  //receive: 00 00 FF 00 FF 00 00 00 FF 03 FD D5 41 00 EA 00  ---16Bytes--- //Success sign 41 00   
  unsigned char cmdPassWord[22]={0x00,0x00,0xFF,0x0F,0xF1,0xD4,0x40,0x01,0x60,\
                                 0x07,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xD1,0xAA,0x40,0xEA,0xC2,0x00};                                 
  unsigned char sum=0,count=0;
  cmdPassWord[9]=block;
  for(int i=10;i<16;i++) cmdPassWord[i]=st[i-10];// KeyA
  for(int i=16;i<20;i++) cmdPassWord[i]=id[i-16];// UID
  for(int i=0;i<20;i++) sum+=cmdPassWord[i];
  cmdPassWord[20]=0xff-sum&0xff;

  while(Serial.available())   char xx=Serial.read();//clear the serial data
  
  for (int i=0;i<22;i++)  UART_Send_Byte(cmdPassWord[i]);
  delay(100);
  while(Serial.available())
     {
        receive_ACK[count]=Serial.read();
        count++;
     }
  if(checkDCS(16)==1 && receive_ACK[12]==0x41 && receive_ACK[13]==0x00)  return 1;
  else return 0; 
}

void writeData(int block,unsigned char dwic[])//  block: Block No. to be written，dwic[]: Data to be written
{
  //------head------  cmd card1 read block  -------------------datawrt-------------------  DCS POST
  //00 00 ff 15 EB D4  40   01   A0    06  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  CD 00
  //receive: 00 00 FF 00 FF 00 00 00 FF 03 FD D5 41 00 EA 00  ---16Bytes--- //Success sign 41 00 
  unsigned char cmdWrite[]={0x00,0x00,0xff,0x15,0xEB,0xD4,0x40,0x01,0xA0, \
                            0x06,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, \
                            0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xCD,0x00};
  unsigned char sum=0,count=0;
  cmdWrite[9]=block;
  for(int i=10;i<26;i++) cmdWrite[i]=dwic[i-10];// Data to be written
  for(int i=0;i<26;i++) sum+=cmdWrite[i];//get sum
  cmdWrite[26]=0xff-sum&0xff;//DCS=lower 4 bits of sum
  
  while(Serial.available())   char xx=Serial.read();//clear the serial data
  
  for(int i=0;i<28;i++) UART_Send_Byte(cmdWrite[i]);
  while(Serial.available())
     {
        receive_ACK[count]=Serial.read();
        count++;
     }
  if(checkDCS(16)==1 && receive_ACK[12]==0x41 && receive_ACK[13]==0x00) blink();//Blink on success
}

void readAck(int x) //read x byte from serial
{
  unsigned char i;
  for(i=0;i<x;i++)
  {
    receive_ACK[i]= Serial.read();
  }
}

void UART_Send_Byte(unsigned char command_data)  //Send the command in a fixed format
{
  Serial.write(command_data);
  Serial.flush();//complete the transmission of outgoing serial data 
} 

char checkDCS(int x)  //DCS verification function
{
  unsigned char sum=0,dcs=0;
  for(int i=6;i<x-2;i++)
  {
    sum+=receive_ACK[i];
  }
  dcs=0xff-sum&0xff;
  if(dcs==receive_ACK[x-2])  return 1;
  else   return 0;
}
