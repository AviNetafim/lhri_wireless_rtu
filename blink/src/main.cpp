/*
* test mode of operation of serial monitor 
*/
#include <Arduino.h>

#define PRINT_ON 1
const String  ParameterList = "srldtcp";                                // serial monitor command code string

struct lhri_msg{                                                        // message fields structure, command & response 
  uint8_t dir;
  uint8_t level;
  uint8_t dlevel;
  uint32_t path;
  uint8_t cmd;
  uint16_t pload;
  uint8_t err;
  uint8_t port; 
};

String inString;                                                        // serial monitor input buffer 
unsigned long new_ms;
unsigned long old_ms;
lhri_msg trs_msg;                                                       // transmit message fields structure
int send = 0;

void parse_parameter();
void show_lhri_msg(String arg_title,lhri_msg arg_msg);

void setup() {
  Serial.begin(115200);
  Serial.print("serial,mointor test");
  pinMode(13,OUTPUT);

}

void loop() {
    while(Serial.available()>0){  
    inString = Serial.readStringUntil('\n');  
    parse_parameter();
  }
  // new_ms = millis();
  // if (new_ms > old_ms){
  //   uint8_t state = digitalRead(13);
  //   digitalWrite(13,!state); 
  //   if (state ==1) Serial.println("on");    
  //   else  Serial.println("off"); 
  //   old_ms = new_ms + 5000;
  // }
}

void parse_parameter(){
  // parse serial monitor input
  char ParameterCode;                                                   // paremeter code - the first character in paramater input string
  int ParNo;                                                            // Paramter location in the code string 
  long ParVal;                                                          // parameter value 
  char char_array[16];
  ParameterCode = inString.charAt(0);   
  ParNo = ParameterList.indexOf(ParameterCode);
  inString.substring(1,8).toCharArray(char_array, sizeof(char_array));  // get serial monitor command data into char array 
  ParVal = strtol(char_array, NULL, 16);                                //  convert char array to hex number
  if (ParNo >= 0)  {
    switch(ParNo){
      case 0:                                                           // paramater "s", send message
        send = 1;
        break;
      
      case 1:                                                           // paramater "r", set dir 
        if (ParVal >= 0x0 && ParVal <= 0x02){
          trs_msg.dir = ParVal;
          Serial.print("dir is set to "); Serial.println(trs_msg.dir,HEX);
        }
        else{
          Serial.println("dir value not in range (0-2) "); Serial.println(ParVal);
        }
        break;

      case 2:                                                           // paramater "l", set  level 
        if (ParVal >= 0x0 && ParVal <= 0x20){
          trs_msg.level = ParVal;
          Serial.print("level is set to "); Serial.println(trs_msg.level,HEX);
        }
        else{
          Serial.println("level value not in range (0 - 0x20): "); Serial.println(ParVal);
        }
        break;

      case 3:                                                           // paramater "d", set dest_level
        if (ParVal >= 0x0 && ParVal <= 0x20){
          trs_msg.dlevel = ParVal;
          Serial.print("dest level is set to "); Serial.println(trs_msg.dlevel,HEX);
        }
        else{
          Serial.print("dest level not in range (0 - 0x20): "); Serial.println(ParVal);
        }
        break;

      case 4:                                                           // paramater "t", set path
        if (ParVal >= 0 && ParVal <= 0xffffffff){
          trs_msg.path = ParVal;
          Serial.print("path is set to "); Serial.println(trs_msg.path,HEX);
        }
        else{
          Serial.print("Wrong path value: "); Serial.println(ParVal);
        }
        break;

      case 5:                                                           // paramater "c", set command
        if (ParVal >= 0 && ParVal <= 0x30){
          trs_msg.cmd = ParVal;
          Serial.print(" command is set to "); Serial.println(trs_msg.cmd,HEX);
        }
        else{
          Serial.print("Wrong command value: "); Serial.println(ParVal);
          }
        break;

      case 6:                                                           // paramater "p", set payload
        trs_msg.pload = ParVal;
        Serial.print("pload is set to "); Serial.println(trs_msg.pload,HEX);
        break;

      default:
        Serial.println("Wrong parameter type");
        break;
    }
  }
  else{
    Serial.println("Wrong parameter type");
  }
  show_lhri_msg("client msg",trs_msg);
}


void show_lhri_msg(String arg_title,lhri_msg arg_msg){
  // display transmit message fields strucutre
  if (PRINT_ON == 1){  
    Serial.print(arg_title); Serial.print(": ");
    Serial.print("dir="); Serial.print(arg_msg.dir,HEX);
    Serial.print(", level="); Serial.print(arg_msg.level,HEX);
    Serial.print(", dlevel=") ;Serial.print(arg_msg.dlevel,HEX);
    Serial.print(", path="); Serial.print(arg_msg.path,HEX);
    Serial.print(", cmd=");Serial.print(arg_msg.cmd,HEX);
    Serial.print(", pload=");Serial.print(arg_msg.pload,HEX);
    Serial.print(", error=");Serial.println(arg_msg.err,HEX);
  }  
}
