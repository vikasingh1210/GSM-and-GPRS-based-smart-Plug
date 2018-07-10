#include <SoftwareSerial.h>
#include "TimerOne.h"
#include <EEPROM.h>
#include <MemoryFree.h>

#define DEBUG true

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

unsigned int            address = 0;
unsigned int            clock_second=0;
unsigned int            clock_minute=0;
unsigned int            clock_hour=0;
boolean                 inTimerMode=false;
unsigned int            resendIMEIcommand=0;
unsigned long           timeout_start_val;
unsigned int            control_var=0;
unsigned int            no_net_conn=0;
byte                    previousLightTrackker=0;
//unsigned int            GPRS_setting_flow_control_var=0;
//int                     control=1;
boolean                 callResetWhenSMSMemoFull=false;
int                     sent=1;
int                     received=1;
int                     index=0;
int                     ERROR_Tracker=0;
unsigned long           timeout_start_val_1;
unsigned long           delay_multiplyer=1;
char                    cmgl='a';
byte                    CMGL_starter=0;
int                     indexOfCGSN=0;

//////////////***********  Arduino PINS****/////
const int               relayPIN = 4;
const int               redLED = 6; 
const int               greenLED =7;
const int               blueLED = 8;
//const int               SIM800_Reset_Pin = 12;




int                     last_message_slot=1;  // considering message is coming on slot no 1
char scratch_data_from_ESP[2];
SoftwareSerial SIM800L(2,3); // make RX Arduino line is pin 2, make TX Arduino line is pin 3.
                             // This means that you need to connect the TX line from the esp to the Arduino's pin 2
                             // and the RX line from the esp to the Arduino's pin 3
String IMEI1="";   
//String IMEI="";
String Reg_no_1="";
String Reg_no_2="";
//String Reg_no_3="";
String response="";
String gsm="GSM";
//String response1 = "";
String APN="";
String PWD="";
String USERNAME="";
const char keyword_OK[]="OK";


void setup()
{
Serial.begin(9600);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(relayPIN,OUTPUT);

  digitalWrite(relayPIN,LOW);

  glow_none();
  delay(2000);
  
  glow_green();
  delay(1000);

  glow_red();
  delay(1000);

  glow_blue();
  delay(1000);

  glow_none();
  delay(1000);

  glow_red();
  delay(1000);  
  

  /////////**********Problem in SIM800L***Default Baud rate is 115200 everytime we switch ON**********//////////
  SIM800L.begin(115200); // initially at 115200
  SIM800L.print("AT+IPR=9600\r\n");// changing baud rate to 9600
   ////////**********Problem in SIM800L***Default Baud rate is 115200 everytime we switch ON**********////////// 

  SIM800L.begin(9600); 
  inputString.reserve(165);

  //String attention=sendData("AT\r\n",2000,DEBUG); // Attention
  SIM800L.print("AT\r\n");
  if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),2000,0))//go look for keyword "OK" with a 5sec timeout
    Serial.println(F("AT OK"));
  else
    Serial.println(F("AT Fail"));

  SIM800L.print("AT+CGSN\r\n");
  if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
    Serial.println(F("AT CGSN OK"));
  else
  {
     Serial.println(F("AT CGSN FAIL"));
    //Serial.println("IMEI FAIL");
    boolean i=0;
    serial_dump_esp();
        do
        {
          SIM800L.print("AT+CGSN\r\n");
          i=read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0);
          //Serial.println("stuck");
         }while(i!=true);
  }

SIM800L.print("AT+CMGF=1\r\n");
  if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
    Serial.println(F("AT CMGF OK"));
  else
  {
   Serial.println(F("AT CMGF FAIL"));
  }
  //check_error(attention);



  do{

  IMEI1=sendData("AT+CGSN\r\n",1000,DEBUG); // get IMEI number
  check_error(IMEI1);
  }while(resendIMEIcommand==1);

  delete_sms(1,3);
  IMEI1="";
  //added indexOf CGSN because there are some garbage char of unkonwn length occuring so we can't find the position of IMEI
  IMEI1=sendData("AT+CGSN\r\n",1500,DEBUG); // get IMEI again because some problem in SIM800L number
  String IMEI2=sendData("AT+CGSN\r\n",1500,DEBUG); // get IMEI again because some problem in SIM800L number
  //find the index of "AT+CGSN" after that store the data we will always get right data
  indexOfCGSN=IMEI1.indexOf("T+CGSN");
  Serial.print(F("Index Of IMEI:"));
  Serial.println(indexOfCGSN);
  /*indexOfCGSN=IMEI1.indexOf("86");
  Serial.print(F("Index Of 86:"));
  Serial.println(indexOfCGSN);*/
  
  SIM800L.print("AT+CNMI=1,1,0,0,0\r\n");//normal sms reading mode with response CMTI:
  if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
    Serial.println(F("CNMI OK"));
  else
  {
 Serial.println(F("CNMI FAIL"));
  }





  Serial.println(IMEI1);
  
  
  unsigned int IMEI1_len=IMEI1.length();
  Serial.println(IMEI1_len);
  
  serial_dump_esp();

readAllDataFromEEPROM();
Timer1.initialize(1000000);         // initialize timer1, and set a 1 second period
  Timer1.attachInterrupt(callback);
 
  Serial.print(F("FM: "));
  Serial.println(freeMemory());





}

void loop()
{


         if (stringComplete) {

                                
                                //writing sent=0 and received=0 we can disable the GPRS services                                 
                                Serial.println(inputString);
                                Serial.print(F("FM: "));
                                Serial.println(freeMemory());

                                 index=inputString.indexOf("CMTI:");
                                 if(index>0)
                                 {
                                   sent=0;
                                   received=0;      
                                   control_var=0;                                
                                   Serial.println(index);
                                   SMS_Command(index, inputString);
                                   sent=1;
                                   received=1;
                                  inputString="";                            

                                 }
                                

                                 int index=inputString.indexOf("RING");
                                 if(index>0)
                                 { 
                                   sent=0;
                                   received=0;      
                                   control_var=0;
                                   CALL_Command();
                                   delay_multiplyer=1;
                                   //sent=1;
                                   //received=1;                      
                                 }

                               index=inputString.indexOf("+SAPBR=3,1");
                                if((index>=1)&&(sent==2))
                                  {
                                    received=10;     
                                    //Serial.println("1q");   
                                    //Serial.print("Val of index: ");
                                    Serial.println(index);
                                    ERROR_Tracker=0;                                    
                                    }
                                index=inputString.indexOf("3,1,\"APN");
                                if((index>=1)&&(sent==10))
                                  {
                                    received=2;  
                                    sent=2;   
                                    //Serial.println("1q");   
                                    //Serial.print("Val of index: ");
                                    Serial.println(index);
                                    ERROR_Tracker=0;                                    
                                    }
                                index=inputString.indexOf("OK");
                                 if((index>=1))
                                  {
                                      if((sent==3))
                                      {
                                        received=3;      
                                        //Serial.println("2q");  
                                        //Serial.print("Val of index: ");
                                        Serial.println(index);
                                        ERROR_Tracker=0;
                                      }
                                      else
                                      Serial.println(F("KK"));                                    

                                    }

                                     index=inputString.indexOf("ERROR");
                                     if((index>=1)&&(sent==3))
                                      {
                                        received=3;      
                                        //Serial.println("2q2");  
                                        //Serial.print("Val of index: ");
                                        Serial.println(index);  
                                        //do not put ERROR_Tracker=0 here                                      

                                        }

                                     index=inputString.indexOf("+SAPBR:");
                                     if((index>=1)&&(sent==4))
                                      {
                                        received=4;     
                                       // Serial.println("3q");   
                                        //Serial.print("Val of index: ");
                                        Serial.println(index);
                                        ERROR_Tracker=0;

                                        }

                                       index=inputString.indexOf("+HTTPINIT");
                                     if((index>=1)&&(sent==5))
                                      {
                                        received=5;  
                                        //Serial.println("4q");   
                                        //Serial.print("Val of index: ");
                                        Serial.println(index);
                                        ERROR_Tracker=0;

                                        }


                                         index=inputString.indexOf("+HTTPPARA");
                                       if((index>=1)&&(sent==6))
                                        {
                                          received=6;    
                                          //Serial.println("5q");  
                                          //Serial.print("Val of index: ");
                                          Serial.println(index);
                                          ERROR_Tracker=0;

                                          }


                                       index=inputString.indexOf("+HTTPPARA=\"URL\",");
                                       if((index>=1)&&(sent==7))
                                        {
                                          received=7;    
                                          //Serial.println("6q"); 
                                          //Serial.print("Val of index: ");
                                          Serial.println(index);
                                          ERROR_Tracker=0;

                                          }

                                    index=inputString.indexOf("+HTTPACTION: 0,6");//there is space in case of SIM800L (+HTTPACTION: 0,6)so take care
                                     if((index>=1)&&(sent==8))
                                      {
                                        byte oldValue=previousLightTrackker;
                                        Serial.print(F("Value of PLT:"));
                                        Serial.println(oldValue);
                                        received=6; 
                                        sent=6;
                                        glow_white();
                                        delay(700);
                                        glow_none();
                                        delay(300);
                                        glow_white();
                                        delay(700);
                                        glow_none();
                                        previousLightTrackker=oldValue;
                                        Serial.print(F("Value of PLT AFTER:"));
                                        Serial.println(previousLightTrackker);
                                        switchToPreviousColorLight(previousLightTrackker);
                                       // Serial.println("7q");
                                        //Serial.print("Val of index: ");
                                        Serial.println(index);
                                       //do not put ERROR_Tracker=0; here
                                       no_net_conn++;
                                       if(no_net_conn>=4)
                                           {
                                             sent=0;
                                             received=0;//disableing GPRS Services 
                                             no_net_conn=0;                                                                         
                                           }
                                        }//end index of HTTPACTION:0,6  

                                          index=inputString.indexOf("+HTTPACTION: 0,4");//there is space in case of SIM800L (+HTTPACTION: 0,6)so take care
                                     if((index>=1)&&(sent==8))
                                      {
                                        byte oldValue=previousLightTrackker;
                                        Serial.print(F("Value of PLT:"));
                                        Serial.println(oldValue);
                                        received=6; 
                                        sent=6;
                                        glow_white();
                                        delay(700);
                                        glow_none();
                                        delay(300);
                                        glow_white();
                                        delay(700);
                                        glow_none();
                                        previousLightTrackker=oldValue;
                                        Serial.print(F("Value of PLT AFTER:"));
                                        Serial.println(previousLightTrackker);
                                        switchToPreviousColorLight(previousLightTrackker);
                                       // Serial.println("7q");
                                        //Serial.print("Val of index: ");
                                        Serial.println(index);
                                       //do not put ERROR_Tracker=0; here
                                       no_net_conn++;
                                       if(no_net_conn>=4)
                                           {
                                             sent=0;
                                             received=0;//disableing GPRS Services 
                                             no_net_conn=0;                                                                         
                                           }
                                        }//end index of HTTPACTION: 0,4

                                        index=inputString.indexOf("+HTTPACTION: 0,200");//there is space in case of SIM800L so take care
                                     if((index>=1)&&(sent==8))
                                      {
                                        received=8; 
                                        //Serial.println("8q");
                                        //Serial.print("Val of index: ");
                                        Serial.println(index);
                                        ERROR_Tracker=0;
                                        delay_multiplyer=1;
                                        Serial.print(F("FM: "));
                                        Serial.println(freeMemory());
                                       } 

                                       index=inputString.indexOf("HTTPREAD:");//going forwards in command chain
                                     if((index>=1)&&(sent==9))
                                      {
                                        received=7; 
                                        sent=7;
                                        //Serial.println("9q");
                                        //Serial.print("Val of index: ");
                                        Serial.println(index);
                                        ERROR_Tracker=0;
                                       }  
                                       
                                        index=inputString.indexOf("ERROR");//checking error so that we can suspend GPRS services SIM800L
                                        if(index>=1)
                                        {
                                          ERROR_Tracker++;      
                                          if(ERROR_Tracker>=20)
                                            {
                                              sent=0;
                                              received=0;//disableing GPRS Services 
                                              ERROR_Tracker=0;
                                            }
                                          }
                                          
                                      index=inputString.indexOf("kinedyna");//checking error so that we can suspend GPRS services SIM800L
                                        if(index>=1)
                                        {
                                         take_decision(inputString);
                                         delay(15);                                   

                                          }                         

                                   Serial.print(F("ET: "));
                                   Serial.println(ERROR_Tracker);
                                   Serial.print(F("se: "));
                                   Serial.println(sent);
                                   Serial.print(F("re: "));
                                   Serial.println(received);
                                   
                                 serial_dump_esp();                                  

                                inputString = "";
                                stringComplete = false;
                               timeout_start_val_1=millis();//


                            }// end of if(inputString)



if((millis()-timeout_start_val_1)>(delay_multiplyer*30000))// after fix time module will check for GPRS connections
    {
      SIM800L.print("AT\r\n");        
      delay(1100);      
      sent=1;
      received=1;
      delay_multiplyer++;
      if(delay_multiplyer>=200)
      {
        delay_multiplyer=1;
      }
      Serial.print(F("Q: "));
      Serial.println(delay_multiplyer); 
      cmgl='a';
      CMGL_command();
      delay(15);      
    }

 // Serial.println(delay_multiplyer*30000);
 
 if((sent==0)&&(received==0))
  {
    if(cmgl=='a')
      {
        CMGL_command();
        delay(15);
      }
       
    }

 if((sent==1)&&(received==1))//sent is 1 r is 1
  {
    Check_GPRS_Attached_COM1();
    delay(15);
    sent=2;     //s ++
    }

if((sent==2)&&(received==2))
  {
    Check_GPRS_Attached_COM2();
    delay(15);
    sent=3;     
    }

if((sent==3)&&(received==3))
  {
    Check_GPRS_Attached_COM3();
    delay(15);
    sent=4;     
    }

if((sent==4)&&(received==4))
  {
    HTTP_Start_initialization_COM4();
    delay(15);
    sent=5;     
    }
if((sent==5)&&(received==5))
  {
    HTTP_Start_para_COM5();
    delay(15);
    sent=6;     
    }
if((sent==6)&&(received==6))
  {
   
    HTTP_Start_URL_COM6();
    delay(15);
    sent=7;     
    }

if((sent==7)&&(received==7))
  {
    CMGL_starter++;    
   HTTP_Start_Action_COM7();
   delay(15);
    sent=8;     
    }
if((sent==8)&&(received==8))
  {
   HTTP_Start_Read_COM8();
   delay(15);
    sent=9;     
    }
    if((sent==2)&&(received==10))
  {
   Check_GPRS_Attached_COM_1_1();
   delay(15);
    sent=10;     
    }

    if(CMGL_starter>=5)
      { 
        inputString="";//free the memory for further use
        start_CMGL();
        delay(15);
        CMGL_starter=0;
        Serial.println(F("Voila Mil Gaya Re++++++++++++"));
        }


          if((clock_hour <= 0) && (clock_minute <= 0)&&(inTimerMode==1))//39020202 sbi
         {
                                digitalWrite(relayPIN, LOW);
                                glow_red(); 
                                inTimerMode=false;  
                                Serial.println(F("TIMER MODE SAAAAAAAALLLLLLLLLLLLLLAAAAAAA"));                                    
                                send_sms(4);
                                if(callResetWhenSMSMemoFull==true)// doing this because SMS memory may become full and we will reset in off message and timer message
                                {
                                  callResetWhenSMSMemoFull=false;
                                  softReset();
                                }
        }
}


String sendData(String command, const int timeout, boolean debug)
{
      String response = "";

      SIM800L.print(command); // send the read character to the SIM800L

      long int time = millis();

      while( (time+timeout) > millis())
      {
        while(SIM800L.available())
        {

          // The esp has data so display its output to the serial window 
          char c = SIM800L.read(); // read the next character.
          response+=c;
        }  
      }

      if(debug)
      {
        Serial.print(response);
      }

      return response;
}
void serial_dump_esp()
{
  char temp;
  while(SIM800L.available()){
    temp =SIM800L.read();
    delay(1);//could play around with this value if buffer overflows are occuring
  }//while


}//serial dump

void read_sms(int slot_no)
    {
        IMEI1=sendData("AT+CGSN\r\n",1500,DEBUG); // get IMEI again because some problem in SIM800L number
        
        indexOfCGSN=IMEI1.indexOf("T+CGSN");
        Serial.print(F("Index Of IMEI:"));
        Serial.println(indexOfCGSN);
        if(indexOfCGSN>20)
        indexOfCGSN=1;
        if(slot_no>=10)//minimum sms storage will be 10 
        {
          callResetWhenSMSMemoFull=true;// doing this because SMS memory may become full and we will reset in off message and timer message
        }

                              String inComingnumber="";
                              String response1 = "";
                              String message="";
                              int j=0;
                              SIM800L.print("AT+CMGR=");
                              SIM800L.print(slot_no);
                              SIM800L.print("\r\n");
                              
                              long int time = millis();
                              while( (time+2000) > millis())
                               {
                                         while(SIM800L.available())
                                        {
                                            char c = SIM800L.read(); // read the next character.
                                            response1 += c; 
                                            //Serial.print(c);  
                                            //Serial.println(response1);                                          

                                          }


                               }

                                //Serial.println("Reading SMS");
                                Serial.println(response1);
                                //Serial.println(inputString);
                                int firstClosingBracket = response1.indexOf('"');
                                //Serial.println(firstClosingBracket);
                                int secondClosingBracket = response1.indexOf('"',firstClosingBracket+1);
                                //Serial.println(secondClosingBracket);
                                int thirdClosingBracket = response1.indexOf('"',secondClosingBracket+1);
                                //Serial.println(thirdClosingBracket);
                                int fourthClosingBracket = response1.indexOf('"',thirdClosingBracket+1);
                                //Serial.println(fourthClosingBracket);
                                //Serial.println("newLineposition");
                                int firstNewLine = response1.indexOf('\n');
                                //Serial.println(firstNewLine);
                                int secondNewLine = response1.indexOf('\n',firstNewLine+1);
                                //Serial.println(secondNewLine);
                                int thirdNewLine = response1.indexOf('\n',secondNewLine+1);
                                //Serial.println(thirdNewLine);
                                int fourthNewLine = response1.indexOf('\n',thirdNewLine+1);
                                //Serial.println(fourthNewLine);
                                int fifthNewLine = response1.indexOf('\n',fourthNewLine+1);
                                //Serial.println(fifthNewLine);
                                int sixthNewLine = response1.indexOf('\n',fifthNewLine+1);
                                //Serial.println(sixthNewLine);
                                int seventhNewLine = response1.indexOf('\n',sixthNewLine+1);
                                //Serial.println(seventhNewLine);
                                
                                for(j=thirdClosingBracket+1;j<fourthClosingBracket;j++)//same for all
                                    {
                                      char k=response1[j];//byte by byte add to string
                                      inComingnumber+=k;

                                    }
                                //Serial.println("PH No SE");
                                Serial.println(inComingnumber);


                                for(j=fourthNewLine+1;j<fifthNewLine;j++)//storing message for way2sms only
                                //for(j=thirdNewLine+1;j<fourthNewLine;j++)//storing message for normal number
                                    {
                                      char k=response1[j];//byte by byte add to string
                                      message+=k;
                                    }
                                  Serial.println(message);

                                 if(message.startsWith("IMEI",0))//register number
                                   {
                                       j=0;
                                       //Serial.println("IMEI sms");
                                       for(int i=4;i<20;i++)
                                         {
                                              //added indexOf CGSN because there are some garbage char of unkonwn length occuring so we can't find the position of IMEI
                                             if(message[i]==IMEI1[i+indexOfCGSN+5])//diffrent for 6 for SIM800L  SIM900 is 7 IMEI1="AT+CGSN\r\n123456789012345\r\nOK\r\n"
                                             j++;   
                                             Serial.print(message[i]);
                                             Serial.println(IMEI1[i+indexOfCGSN+5]);                                          
                                         }
                                         Serial.println(j);
                                        if(j>=15)//15 for way2sms 16 for normal nuumber
                                            {


                                                Serial.println(F("RT IMEI"));
                                                address=0;
                                                /**********************STROING REG NO1 START ************************/
                                                //for(j=fourthNewLine+1;j<fourthNewLine+14;j++)//fourth for normal number   
                                                for(j=fifthNewLine+1;j<fifthNewLine+14;j++)//   fifth for way2sms
                                                {
                                                char value=response1[j];
                                                Serial.println(value);
                                                EEPROM.write(address, value);
                                                address++;
                                                }
                                               /**********************STROING REG NO1 ENDS************************/ 
                                               address=15;
                                                /**********************STROING REG NO2 START ************************/
                                                //for(j=fifthNewLine+1;j<fifthNewLine+14;j++)//fifth for normal number   
                                                for(j=sixthNewLine+1;j<sixthNewLine+14;j++)//   sixth for way2sms
                                                {
                                                char value=response1[j];
                                                Serial.println(value);
                                                EEPROM.write(address, value);
                                                address++;
                                                }
                                               /**********************STROING REG NO2 ENDS************************/ 
                                               send_sms(1);
                                               readAllDataFromEEPROM();
                                            }

                                        else
                                        Serial.println(F("Wr IM"));
                                        send_sms(2);
                                   }
                                 else if(message.startsWith("On",0))//ON
                                   {
                                     j=0;
                                     //Serial.println("On message");
                                     address=0;//for Reg no 1
                                     j=check_valid_no(address, inComingnumber);
                                     if(j>=8)
                                     {
                                       digitalWrite(relayPIN,HIGH);
                                       glow_green();
                                       send_sms(3);
                                       Serial.println("Sahi No hai");
                                       inTimerMode=false;
                                     }

                                     j=0;
                                     address=15;//for Reg no 2
                                     j=check_valid_no(address, inComingnumber);
                                     if(j>9)
                                     {
                                       digitalWrite(relayPIN,HIGH);
                                       glow_green();
                                       send_sms(3);
                                       Serial.println("Sahi No hai");
                                       inTimerMode=false;
                                     } 

                                   }
                                 else if(message.startsWith("Off",0))//
                                   {

                                     //Serial.print("Off wala message");
                                      j=0;
                                     address=0;//for Reg no 1
                                     j=check_valid_no(address, inComingnumber);
                                     if(j>=8)
                                     {
                                       digitalWrite(relayPIN,LOW);
                                       glow_red();
                                       Serial.println("Sahi No hai");
                                       inTimerMode=false;
                                       send_sms(4);
                                       if(callResetWhenSMSMemoFull==true)// doing this because SMS memory may become full and we will reset in off message and timer message
                                        {
                                          callResetWhenSMSMemoFull=false;
                                          softReset();
                                        }
                                     }
                                     j=0;
                                     address=15;//for Reg no 2
                                     j=check_valid_no(address, inComingnumber);
                                     if(j>12)
                                     {
                                       digitalWrite(relayPIN,LOW);
                                       glow_red();
                                       Serial.println("Sahi No hai");
                                       inTimerMode=false;
                                       send_sms(4);
                                       if(callResetWhenSMSMemoFull==true)// doing this because SMS memory may become full and we will reset in off message and timer message
                                        {
                                          callResetWhenSMSMemoFull=false;
                                          softReset();
                                        }
                                     }
                                   }
                                 else if(message.startsWith("Timer",0))//timer mode
                                   {

                                       //Serial.println("Timer wala message");
                                       inTimerMode=true;
                                       String inString="";
                                       //Serial.println("timer set 123456"+message);
                                       for(int i=5;i<9;i++)
                                         {
                                             char y=message[i];
                                             inString+=y;
                                             Serial.println(y);
                                         }
                                        //Serial.println("This is timer string"+inString);
                                        int setTime=inString.toInt();
                                        clock_hour=setTime/60;
                                        clock_minute=setTime%60;
                                        //Serial.print("Hours");
                                        //Serial.println(clock_hour);
                                        //Serial.print("Minutes");
                                        Serial.println(clock_minute);
                                        j=0;
                                       address=15;//for Reg no 2
                                       j=check_valid_no(address, inComingnumber);
                                       if(j>12)
                                       {
                                          digitalWrite(relayPIN,HIGH);
                                          glow_blue();
                                          Serial.println("Sahi No hai");
                                          send_sms(5);
                                       }
                                       j=0;
                                       address=0;//for Reg no 1
                                       j=check_valid_no(address, inComingnumber);
                                       if(j>=8)
                                       {
                                          digitalWrite(relayPIN,HIGH);
                                          Serial.println("Sahi No hai");
                                          glow_blue();
                                          send_sms(5);
                                       }
                                   }

                                  else if(message.startsWith("GPRS",0))
                                  {
                                    address=40;
                                        if(message.startsWith("GPRS",0))//register number
                                   {
                                       int j=0;
                                       //Serial.println("GPRS sms");
                                       for(int i=4;i<20;i++)
                                         {
                                           if(message[i]==IMEI1[i+indexOfCGSN+5])//6 for SIM800L, 7 for SIM900
                                             j++;   
                                             Serial.print(message[i]);
                                             Serial.print(IMEI1[i+indexOfCGSN+5]);  
                                             Serial.println(i+indexOfCGSN+5);                                                 
                                         }
                                         Serial.println(j);
                                        if(j>=15)//15 for way2sms 16 for normal nuumber
                                            {


                                                Serial.println(F("IM GP"));
                                                address=40;  //to 60
                                                /**********************STROING REG NO1 START ************************/
                                                //for(j=fourthNewLine+1;j<fourthNewLine+14;j++)//fourth for normal number   same as storing no
                                                for(j=fifthNewLine+1;j<fifthNewLine+20;j++)//   fifth for way2sms same as storing no
                                                {
                                                char value=response1[j];
                                                Serial.println(value);
                                                EEPROM.write(address, value);
                                                address++;
                                                }
                                               /**********************STORING REG NO1 ENDS************************/ 
                                               address=61;
                                                /**********************STROING REG NO2 START ************************/
                                                //for(j=fifthNewLine+1;j<fifthNewLine+14;j++)//fifth for normal number   same as storing no
                                                for(j=sixthNewLine+1;j<sixthNewLine+20;j++)//   sixth for way2sms  same as storing no
                                                {
                                                char value=response1[j];
                                                Serial.println(value);
                                                EEPROM.write(address, value);
                                                address++;
                                                }
                                               /**********************STROING REG NO2 ENDS************************/ 
                                               address=81;
                                                /**********************STROING REG NO2 START ************************/
                                                //for(j=sixthNewLine+1;j<sixthNewLine+14;j++)//sixth for normal number   
                                                for(j=seventhNewLine+1;j<seventhNewLine+20;j++)//   sixth for way2sms
                                                {
                                                char value=response1[j];
                                                Serial.println(value);
                                                EEPROM.write(address, value);
                                                address++;
                                                }
                                               /**********************STROING REG NO2 ENDS************************/ 
                                                readAllDataFromEEPROM();
                                               send_sms(6);
                                            }

                                        else
                                        Serial.println(F("WRIM"));
                                        send_sms(2);
                                   }

                                    }



                                 else
                                 {
                                    Serial.println(F("UKC"));
                                 }
                                 serial_dump_esp(); 
                                 
                                 response1="";  



    }

void delete_sms(int startSlotNo, int endSlotNo )
{
    /*SIM800L.print("AT+CMGD=");
    SIM800L.print(startSlotNo);
    SIM800L.print("\r\n");
    serial_dump_esp();*/
    //String response1="";
    for(int j=startSlotNo;j<endSlotNo;j++)
      {
        //String response1="";
        SIM800L.print("AT+CMGD=");
        SIM800L.print(j);
        SIM800L.print("\r\n");
       /* long int time = millis();
        while( (time+2000) > millis())
         {
                   while(SIM800L.available())
                  {

                      char c = SIM800L.read(); // read the next character.
                      response1+=c;


                    }


         }
       Serial.print(response1);*/
        if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))
        {
          Serial.println(F("SMS Delete OK"));
          }

          else
          {
            Serial.println(F("SMS Delete Fail"));
            }
       
       serial_dump_esp(); 
      }


}

void callback()
{



    clock_second++;

    if(clock_second==60)
    {
      clock_minute--;
      clock_second=0;

      if(clock_minute==-1)
      {
        clock_hour--;
        clock_minute=59;
      }
    }

}

int check_valid_no(int address, String inComingnumber)
    {
      //char charBuffer[14];
      //inComingnumber.toCharArray(charBuffer,14);
      int j=0;  
      for(int i=0;i<13;i++)//checking valid number
             {
               char c;
               c=EEPROM.read(address);

               if(inComingnumber[i]==c)
                 j++; 

                 address++;     

             }

        return j;

    }

void send_sms(int z)
    {
      /*SIM800L.print("AT+CSCS=\"");
      SIM800L.print(gsm);
      SIM800L.print("\"\r\n");*/
      const char keyword_CMGS[]="+CMGS";
      SIM800L.print("AT+CMGF=1\r\n");
     
      delay(1000);
      SIM800L.print("AT+CMGS=\"");
      SIM800L.print(Reg_no_2);
      SIM800L.print("\"\r\n");

      delay(1000);
      switch (z) 
      {
        case 1:
        SIM800L.print("Nos are registered \x1A");
        break;
        
        case 2:
        SIM800L.print("NOT reg \x1A");
        break;
        
        case 3:
        SIM800L.print("Device is ON \x1A");
        break;
        
        case 4:
        SIM800L.print("Device is OFF \x1A");
        break;
        
        case 5:
        SIM800L.print("In timer mode \x1A");
        break;
        
        case 6:
        SIM800L.print("GPRS setting done \x1A");
        break;
        
        default: 
        z=0;
        break;

        
      }
      
       if(read_until_SIM800L(keyword_CMGS,sizeof(keyword_CMGS),5000,0))
      {
        Serial.println(F("Send SMS OK"));        
      }
       /*if(z==1)
       SIM800L.print("Nos are registered \x1A");
       if(z==2)
       SIM800L.print("NOT reg \x1A");
       if(z==3)
       SIM800L.print("Device is ON \x1A");
       if(z==4)
       SIM800L.print("Device is OFF \x1A");
       if(z==5)
       SIM800L.print("In timer mode \x1A");
       if(z==6)
       SIM800L.print("GPRS done \x1A");*/

       delay(1000);
       serial_dump_esp();
    }

void check_error(String check)
{
  if(check.indexOf("ERROR")>1)
  {
      sendData("AT\r\n",2000,DEBUG); // Attention
      serial_dump_esp();
      resendIMEIcommand=1;
     // digitalWrite(13, HIGH);
  } 
 else 
 {
      resendIMEIcommand=0;
      //digitalWrite(13, LOW);
 }
}

boolean read_until_SIM800L(const char keyword1[], int key_size, int timeout_val, byte mode){
  timeout_start_val=millis();//for the timeout
  char data_in[20];//this is the buffer - if keyword is longer than 20, then increase this
  int scratch_length=1;//the length of the scratch data array
  key_size--;//since we're going to get an extra charachter from the sizeof()

 //FILL UP THE BUFFER
 for(byte i=0; i<key_size; i++){//we only need a buffer as long as the keyword

            //timing control
            while(!SIM800L.available()){//wait until a new byte is sent down from the ESP - good way to keep in lock-step with the serial port
              if((millis()-timeout_start_val)>timeout_val)
              {//if nothing happens within the timeout period, get out of here
                Serial.println(F("timeout"));
                return 0;//this will end the function
              }//timeout
            }// while !avail

    data_in[i]=SIM800L.read();// save the byte to the buffer 'data_in[]

    if(mode==1){//this will save all of the data to the scratch_data_from
      scratch_data_from_ESP[scratch_length]=data_in[i];//starts at 1
      scratch_data_from_ESP[0]=scratch_length;// [0] is used to hold the length of the array
      scratch_length++;//increment the length
    }//mode 1

  }//for i

//THE BUFFER IS FULL, SO START ROLLING NEW DATA IN AND OLD DATA OUT
  while(1){//stay in here until the keyword found or a timeout occurs

     //run through the entire buffer and look for the keyword
     //this check is here, just in case the first thing out of the ESP was the keyword, meaning the buffer was actually filled with the keyword
     for(byte i=0; i<key_size; i++){
       if(keyword1[i]!=data_in[i])//if it doesn't match, break out of the search now
       break;//get outta here
       if(i==(key_size-1)){//we got all the way through the keyword without breaking, must be a match!
       return 1; //return a 1 and get outta here!
       }//if
     }//for byte i


    //start rolling the buffer
    for(byte i=0; i<(key_size-1); i++){// keysize-1 because everthing is shifted over - see next line
      data_in[i]=data_in[i+1];// so the data at 0 becomes the data at 1, and so on.... the last value is where we'll put the new data
    }//for


           //timing control
            while(!SIM800L.available()){// same thing as done in the buffer
              if((millis()-timeout_start_val)>timeout_val){
                Serial.println(F("timeout"));
                return 0;
              }//timeout
            }// while !avail



    data_in[key_size-1]=SIM800L.read();//save the new data in the last position in the buffer

      if(mode==1){//continue to save everything if thsi is set
      scratch_data_from_ESP[scratch_length]=data_in[key_size-1];
      scratch_data_from_ESP[0]=scratch_length;
      scratch_length++;
    }//mode 1

      //JUST FOR DEBUGGING
    if(SIM800L.overflow())
    Serial.println(F("*OVER"));


  }//while 1




}//read until ESP

void serialEvent() {
  while (SIM800L.available()) {
    // get the new byte:
    char inChar = (char)SIM800L.read();
    // add it to the inputString:
     Serial.println("Serial");
    inputString += inChar;
    Serial.print(inChar);
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {      
      stringComplete = true;
      //Serial.println(inputString);
    }
  }
}

void SMS_Command(int index1, String inputString)
{


      String slot = "";      
      int slot_no;

      //Serial.println(inputString);
      //int firstComma = inputString.indexOf(',');
      //Serial.print("Value of firstComma: ");
      //Serial.println(firstComma);
      int firstComma=index1+10;//String is "+CMTI: "SM"," so value of addtion is 6

      slot=(char)inputString[firstComma+1];
      Serial.print(F("slot_no1: "));
      Serial.println(slot);

      slot +=(char)inputString[firstComma+2];
      Serial.print(F("final solt: "));
      Serial.println(slot);


      /*while(SIM800L.available())
                      {
                            int c = SIM800L.read(); // read the next character.
                            if (isDigit(c))  
                            {
                            slot += (char)c;
                             }
                           Serial.println(slot);
                      }  */


      slot_no=slot.toInt();
      Serial.println(slot_no);
      serial_dump_esp();



      if(slot_no==0)// some error had taken place 
        {
          //read the incoming message by keeping track of last message slot
          read_sms(last_message_slot);
          delete_sms(last_message_slot,last_message_slot+2);

          //reset the SIM800L
           SIM800L.print("AT\r\n");
           delay(1100);

        }
      else  // everthing is fine and we got the slot no
        {

           read_sms(slot_no);
           delete_sms(slot_no,slot_no+1);
           
           //if delete is OK than change the slot no
           last_message_slot=slot_no;
           // if delete is not OK then
           //last_message_slot=slot_no+1; Beacuse new message will come at new slot
          }
  }


void CALL_Command()
{
  Serial.println(F("call"));
  }

/*void Check_GPRS_Attached()
{
      SIM800L.print("AT+SAPBR=3,1,\"Contype\", \"GPRS\"\r\n");
      if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
      {
            //Serial.println("GP COM 1 OK"); 
            GPRS_setting_flow_control_var=1;       
      }      
      else
            Serial.println("GP1 FAIL"); 

      if(GPRS_setting_flow_control_var==1)
      {
              SIM800L.print("AT+SAPBR=1,1,\r\n");
              if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
              {
                //Serial.println("GP COM 2 OK"); 
                GPRS_setting_flow_control_var=1;       
              }      
              else
              Serial.println("GP2 FAIL"); 

      }

      if(GPRS_setting_flow_control_var==1)
      {
              SIM800L.print("AT+SAPBR=2,1,\r\n");
              if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
              {
                //Serial.println("GP COM 3OK"); 
                GPRS_setting_flow_control_var=1;       
              }      
              else
              Serial.println("GP3 FAIL"); 

      }

  }*/

/*void GPRS_END()
{

  SIM800L.print("AT+SAPBR=0,1\r\n");
              if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
              {
                //Serial.println("GP END OK"); 
               // GPRS_setting_flow_control_var=1;       
              }      
              else
              Serial.println("GP END FA");   

}*/


/*void HTTP_Start()
{



          SIM800L.print("AT+HTTPINIT\r\n");
              if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
              {
                //Serial.println("gprS ini OK"); 
                GPRS_setting_flow_control_var=10;       
              }      
              else
              Serial.println("gprS ini FAIL");  

              SIM800L.print("AT+HTTPPARA=\"CID\",1\r\n");
              if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
              {
                //Serial.println("gprS para OK"); 
                GPRS_setting_flow_control_var=11;       
              }      
              else
              Serial.println("gprS para FAIL");  

              if(GPRS_setting_flow_control_var==11)
                {

                      SIM800L.print("AT+HTTPPARA=\"URL\",\"http://kinedyna.in/timeshort/receive.php\"\r\n");
                      if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
                      {
                        //Serial.println("GP URL OK"); 
                        GPRS_setting_flow_control_var=12;       
                      }      
                      else
                      Serial.println("GP URL FAIL");  

                  }


                  if(GPRS_setting_flow_control_var==12)
                {

                      SIM800L.print("AT+HTTPACTION=0\r\n");
                      if(read_until_SIM800L(keyword_HTTP_ACTION_OK,sizeof(keyword_HTTP_ACTION_OK),13000,0))//go look for keyword "OK" with a 5sec timeout
                      {
                        //Serial.println("GP ACT OK"); 
                        GPRS_setting_flow_control_var=13;       
                      }      
                      else
                      Serial.println("GP ACT FAIL");  

                  }


                  if(GPRS_setting_flow_control_var==13)
                        {

                              SIM800L.print("AT+HTTPREAD\r\n");
                              if(read_until_SIM800L(keyword_HTTP_READ,sizeof(keyword_HTTP_READ),7000,0))//go look for keyword "OK" with a 5sec timeout
                              {
                                //Serial.println("GP Read OK"); 
                                GPRS_setting_flow_control_var=13;       
                              }      
                              else
                              Serial.println("GP Read FAIL");  

                          }          


  }*/

void Check_GPRS_Attached_COM1()
{ 
  SIM800L.print("AT+SAPBR=3,1,\"Contype\", \"GPRS\"\r\n");  

  }

void Check_GPRS_Attached_COM_1_1()
{  
  String command="AT+SAPBR=3,1,\"APN\",\"";
  command+=APN;
  command+="\"\r\n";
  SIM800L.print(command);

  }
 
void Check_GPRS_Attached_COM2()
{ 
  SIM800L.print("AT+SAPBR=1,1\r\n");

  }
void Check_GPRS_Attached_COM3()
{ 
  SIM800L.print("AT+SAPBR=2,1\r\n");

  }

void HTTP_Start_initialization_COM4()
{

  SIM800L.print("AT+HTTPINIT\r\n");

  }


void HTTP_Start_para_COM5()
{

   SIM800L.print("AT+HTTPPARA=\"CID\",1\r\n");

  }


void HTTP_Start_URL_COM6()
{

  SIM800L.print("AT+HTTPPARA=\"URL\",\"http://kinedyna.in/timeshort/receive.php\"\r\n");
  //SIM800L.print("AT+HTTPPARA=\"URL\",\"http://kinedyna.in/timeshort/receive.php\"\r\n");
   //delay(1000);
  }


  void HTTP_Start_Action_COM7()
{

  SIM800L.print("AT+HTTPACTION=0\r\n");
  delay(10);
  }


    void HTTP_Start_Read_COM8()
{

  SIM800L.print("AT+HTTPREAD\r\n");
  }

 void GPRS_END_COM9()
{

  SIM800L.print("AT+SAPBR=0,1\r\n");
  }

void CMGL_command()
{
//SIM800L.print("AT+CMGL=\"REC UNREAD\"\r\n");
cmgl='z';
  }

void take_decision(String inputString_aaya)
{
  int index_Jane=inputString_aaya.indexOf("Jane");
  /*Serial.print(index_Jane);
  Serial.print(" Val of ISA: ");
  Serial.println(inputString_aaya[index_Jane-1]);*/
  int index_kinedyna=inputString_aaya.indexOf("kinedyna");
  if(index_Jane>=1)//because in URL printing there is "kinedyna" avaialable so it taking false reading as true for relay pin operation 
  {
    if(inputString_aaya[index_Jane-1]=='N')
    {
      digitalWrite(relayPIN,HIGH);
      glow_green();
    }
    else if(inputString_aaya[index_Jane-1]=='F')
    {
      digitalWrite(relayPIN,LOW);
      glow_red();
    }
    else
    {
      String timerString="";
      for(int i=index_kinedyna+8;i<=index_kinedyna+10;i++)
        {
          char c=inputString_aaya[i];
          if(c!='J')
            {
               timerString+=c;
               Serial.println(timerString);

              }
           else
           break;
          }
        int setTime=timerString.toInt();
        clock_hour=setTime/60;  
        clock_minute=setTime%60; 
        glow_blue(); 
        digitalWrite(relayPIN,HIGH);   

      }
  }
  else
  Serial.println(F("qwerty"));
  //glow_none();


}

void glow_green()
  {
    digitalWrite(redLED, LOW);   
  digitalWrite(greenLED, HIGH);
  digitalWrite(blueLED, HIGH);  
  previousLightTrackker=1; 
  Serial.print(F("Value of PVT"));
  Serial.println(previousLightTrackker);
    }

  void glow_red()
  {
  digitalWrite(blueLED, LOW);   
  digitalWrite(redLED, HIGH);
  digitalWrite(greenLED, HIGH);   
  previousLightTrackker=2;
  Serial.print(F("Value of PVT"));
  Serial.println(previousLightTrackker);
  }

  void glow_blue()
  {
  digitalWrite(greenLED, LOW);   
  digitalWrite(blueLED, HIGH);
  digitalWrite(redLED, HIGH);   
  previousLightTrackker=3;
  Serial.print(F("Value of PVT"));
  Serial.println(previousLightTrackker);
  }

  void glow_white()
  {
  digitalWrite(greenLED, LOW);   
  digitalWrite(blueLED, LOW);
  digitalWrite(redLED, LOW);   
  previousLightTrackker=4;
  Serial.print(F("Value of PVT"));
  Serial.println(previousLightTrackker);
  }

  void glow_none()
  {

  digitalWrite(greenLED, HIGH);   
  digitalWrite(blueLED, HIGH);
  digitalWrite(redLED, HIGH);   
  previousLightTrackker=0;
  Serial.print(F("Value of PVT"));
  Serial.println(previousLightTrackker);
  }

  void glow_magenta()
  {
  digitalWrite(redLED, HIGH);   
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, LOW);   
  previousLightTrackker=5;
  Serial.print(F("Value of PVT"));
  Serial.println(previousLightTrackker);
    }

    void glow_yellow()
  {
  digitalWrite(redLED, LOW);   
  digitalWrite(greenLED, HIGH);
  digitalWrite(blueLED, LOW); 
  previousLightTrackker=6;  
  Serial.print(F("Value of PVT"));
  Serial.println(previousLightTrackker);
    }

    void glow_sky_blue()
  {
  digitalWrite(redLED, LOW);   
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, HIGH); 
  previousLightTrackker=7;  
  Serial.print(F("Value of PVT"));
  Serial.println(previousLightTrackker);
    }
void start_CMGL()
{
  //Response of this command is like +CMGL: 1,"REC UNREAD","+91928870634",,"11/01/17,10:26:26+04"
  const char keyword_rn1[] = "+CMGL: ";
  const char keyword_rn2[] = ",\"REC";
  String slot_no="";
  char ip_address[16];
  //for(int j=0;j<16;j++)
  //ip_address[j]='\0';
  SIM800L.print("AT\r\n");
  if(read_until_SIM800L(keyword_OK,sizeof(keyword_OK),5000,0))//go look for keyword "OK" with a 5sec timeout
    Serial.println(F("CMGL OK"));
  else
    Serial.println(F("CMGL Fail"));
 delay(10);
  SIM800L.print("AT+CMGL=\"REC UNREAD\"\r\n");  
  if(read_until_SIM800L(keyword_rn1,sizeof(keyword_rn1),3000,0)){//look for first \r\n(+CMGL:)after AT+CIFSR echo - note mode is '0', the ip address is right after this
  if(read_until_SIM800L(keyword_rn2,sizeof(keyword_rn2),2000,1)){//look for second \r\n, and store everything it receives, mode='1'
    //store the ip adress in its variable, ip_address[]
    for(int i=1; i<=(scratch_data_from_ESP[0]-sizeof(keyword_rn2)+1); i++)//that i<=... is going to take some explaining, see next lines
       ip_address[i] = scratch_data_from_ESP[i];//fill up ip_address with the scratch data received
  //i=1 because i=0 is the length of the data found between the two keywords, BUT this includes the length of the second keyword, so i<= to the length minus
  //size of the keyword, but remember, sizeof() will return one extra, which is going to be subtracted, so I just added it back in +1
    ip_address[0] = (scratch_data_from_ESP[0]-sizeof(keyword_rn2)+1);//store the length of ip_address in [0], same thing as before
    Serial.print(F("IAD="));//print it off to verify
    for(int i=1; i<=ip_address[0]; i++)//send out the ip address
    {
      Serial.print(ip_address[i]);
      slot_no+=ip_address[i];
    
    }
    delay(2000);
    serial_dump_esp();
    int slot_no1=slot_no.toInt(); 
    Serial.println(F(""));
    Serial.print(F("FM: "));
    Serial.println(freeMemory());
    read_sms(slot_no1);
    Serial.print(F("FM: "));
    Serial.println(freeMemory());
    delete_sms(slot_no1, slot_no1+2);     
    
  }//if second \r\n
  }//if first \r\n
  else
  Serial.print(F("F"));
   
  //serial_dump_esp();
  sent=7;
  received=7;
  
}

void readAllDataFromEEPROM()
{
  APN="";
  PWD="";
  USERNAME="";
  Reg_no_1="";
  Reg_no_2="";
   for(address=0;address<13;address++)//Reading Reg_no_1 from EEPROM
      {
        char value = EEPROM.read(address);
        if(value!='\n')
        Reg_no_1+=value;
        else
        break;
      }
  for(address=15;address<28;address++)//Reading Reg_no_2 from EEPROM
      {
        char value = EEPROM.read(address);
        if(value!='\n')
        Reg_no_2+=value;
        else
        break;
      }

      for(address=40;address<60;address++)//Reading APN from EEPROM
      {
        char value = EEPROM.read(address);
         if(value!='\n')
        APN+=value;
        else
        break;
      }

       for(address=61;address<80;address++)//Reading PWD from EEPROM
      {
        char value = EEPROM.read(address);
        if(value!='\n')
        PWD+=value;
        else
        break;
      }

       for(address=81;address<100;address++)//Reading USERID from EEPROM
      {
        char value = EEPROM.read(address);
        if(value!='\n')
        USERNAME+=value;
        else
        break;
        }
  //Serial.print("1 reg no:");
  Serial.println(Reg_no_1);
  //Serial.print("2 reg no:");
  Serial.println(Reg_no_2);
  Serial.print(F("APN"));
  Serial.println(APN);
  Serial.print(F("PWD"));
  Serial.println(PWD);
  Serial.print(F("UNE"));
  Serial.println(USERNAME);
  
  
  }

void switchToPreviousColorLight(int previousLightTrackker)
{
    switch (previousLightTrackker) 
    {
      case 0:
        glow_none();
        break;
      case 1:
        glow_green();
        break;
      case 2:
        glow_red();
        break;
         case 3:
       glow_blue();
        break;
         case 4:
        glow_white();
        break;
         case 5:
        glow_magenta();
        break;
         case 6:
         glow_yellow();
        break;
         case 7:
        glow_sky_blue();
        break;
      default: 
         glow_none();
      break;
    }  
}

void softReset(){
  asm volatile(" jmp 0");
  }
