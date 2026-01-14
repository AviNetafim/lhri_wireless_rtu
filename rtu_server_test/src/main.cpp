/* 
* test wifi server  class 
* created 13/1/2026
*/
#include <Arduino.h>
#include <RtuServer.h>

const char* ssid = "Schweitzer";
const char* password = "plmqa212";
const uint16_t port = 8080;                                             // Port for the TCP server

RtuServer rs(port);

//---------------------------7-------  RTU registers ---------------------------------------------------

uint16_t r_send[1] = {0x0};	                                  					// send command to lhri tree
uint16_t r_trs[7] = {0x0,0x05,0x05,0x1,0x0c,0x05,0x0};                  // transmit msg fields
uint16_t r_rec[9] = {0x0,0x05,0x05,0x1,0x0c,0x05,0x0,0x0,0x0};          // receive msg fields

registers regs[] = {                                                    // register list metadata
  {1,0,0,&r_send[0]},
	{7,0,0,&r_trs[0]},
	{9,0,0,&r_rec[0]}
};
void show_regs(String arg_header);

void setup() {
	Serial.begin(115200);
	delay(1500);
	Serial.println("esp32_client_rtu_1");
    rs.Init(ssid,password);
}

void loop(){
    if(rs.cmd_available(regs)){
        rs.reply();
		show_regs("after command");        

    }
}

void show_regs(String arg_header){
	//  show register map at setup 
  String prt_str="";
  int r;
  int e;
  Serial.print(arg_header); Serial.print(": ");
  for (r=0 ; r < REG_MAP_SIZE ; r++){
	prt_str = "reg[" + String(r)+"]=";
	Serial.print(prt_str);
	Serial.print(*(regs[r].point),HEX);
	Serial.print(",");
  }
  Serial.println();
}

