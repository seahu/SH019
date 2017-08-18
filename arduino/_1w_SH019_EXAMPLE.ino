#include <OneWire.h>

// OneWire HC-SR04 Distance measure Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library
uint8_t i;
uint8_t data[12];
uint8_t addr[8];
//uint8_t addr[8]={0xDE,0xB3, 0x5F, 0xDA, 0x02, 0x00, 0x01, 0x7B};
bool ini;
bool is_set=false;


OneWire  ds(10);  // on pin 10 (a 4.7K resistor is necessary)

/*
 * SET IDENTIFICATION NUMBER CARD READER FOR SIMULATION DS1990 
 * 0x00 - of simulation
 * 0x01-0xff - on simulation, this number is also 2.byte of one wire address
 * DS1990 -send card code into one wire adress, when card reader read card then device DS1990 exist, if no card or last card code was readed, then DS1990 device do not  exist.
 * format one wire address:
 * |  1-byte     | 2-byte      | 3-7                                                  | 8-byte            |
 * | Family code | custom code | CARD CODE IN HEX VALUE                               | CRC (of 1-7 byte) |
 * | 0x01        | 0x01-0xff   | one byte= 1.-4.bit one value, 4.-8.bit second value  |                   |
 */
void set_simulation_DS1990(uint8_t custom_code){
  uint8_t a;
  for (uint8_t i; i<10 ;i++){ // try max 10x witre new value
    ds.reset();
    ds.select(addr);
    ds.write(0x59); // send command for read card code
    delayMicroseconds(100); // some time for slave to sum CRC from card code
    ds.write(custom_code); // 0x00 - off simulation DS1990, other values on silulation
    ds.write(~custom_code); // write reverse value
    a=ds.read();
    Serial.println(a);
    //if (ds.read()==0xAA) break; // write was succesfully
    if (a==0xAA) break; // write was succesfully
  }
}

/*
 * SET ACUSTIC SIGNAL FOR DETECT READ CARD (frekvency and duration x0.01s)
 */
void set_reader_acustic_signal(unsigned int frekvency, uint8_t duration){
  uint8_t buf[3];
  
  buf[0]=(uint8_t)(0x00FF & frekvency);
  buf[1]=(uint8_t)(0xFF00 & frekvency)>>8;
  buf[2]=duration;
  for (uint8_t j; j<10 ;j++){ // try max 10x witre new value
    ds.reset();
    ds.select(addr);
    ds.write(0x5C); // send command for set frekvecncy and duration for akustic signal card reader
    for (uint8_t i=0; i<3;i++) ds.write(buf[i]);
    ds.write(OneWire::crc8(buf, 3)); // send CRC
    delayMicroseconds(200); // some time for slave to sum CRC from card code
    if (ds.read()==0xAA) break; // write was succesfully
  }
}

/*
 * BEEP (frekvency and duration x 0.01s)
 */
void set_beep(unsigned int frekvency, uint8_t duration){
  uint8_t buf[3];
  
  buf[0]=(uint8_t)(0x00FF & frekvency);
  buf[1]=(uint8_t)((0xFF00 & frekvency)>>8);
  buf[2]=duration;
  for (uint8_t j; j<10 ;j++){ // try max 10x witre new value
    ds.reset();
    ds.select(addr);
    ds.write(0x5D); // send command for set frekvecncy and duration for immediate beep
    //delayMicroseconds(10);
    for (uint8_t i=0; i<3;i++) ds.write(buf[i]);
    ds.write(OneWire::crc8(buf, 3)); // send CRC
    delayMicroseconds(200); // some time for slave to sum CRC from card code
    if (ds.read()==0xAA) break; // write was succesfully
  }
}

/*
 * SET STATUS BYTE (write to status byte where 1.bit green led, 2.bit red led, 3.bit rele, 4.-8. no matter)
 * bit mask for green_led - 0x01
 * bit mask for red_led   - 0x02
 * bit mask for raley     - 0x04
 */
#define GREEN_LED_MASK 0x01
#define RED_LED_MASK   0x02
#define REALY_MASK     0x04
void set_status_byte(bool on, uint8_t bit_mask){
  uint8_t status_byte;
  uint8_t i;
  
  // read status byte
  ds.reset();
  ds.select(addr);
  ds.write(0xF8); // send command for read status byte green led, red led, relay
  for (i; i<1 ;i++){ // try max 10x 
    status_byte=ds.read(); 
    if ((uint8_t)~status_byte==ds.read()) {
      ds.write(0xAA); // write answer sucessfuly read byte
      break;
    }
    else {
      ds.write(0xFF); // write answer bad read byte
    }
  }
  // set right bit for green led (1. bit)
  status_byte=status_byte & (uint8_t)~bit_mask; // null bit
  if (on==true) status_byte=status_byte | bit_mask; // set bit to 1 other leave null
  // write status byte
  delayMicroseconds(1200); // for send debug information if no debug then no need
  ds.reset();
  ds.select(addr);
  ds.write(0x58); // send command for write ststus byte
  for (i; i<1 ;i++){ // try max 10x witre new value
    ds.write(status_byte); // send new value
    ds.write((uint8_t)~status_byte); // write reverse value
    if (ds.read()==0xAA) break; // write was succesfully
  }
}


/*
 * STANDART READ CARD CODE
 */
void familyDE_red_card_code(){
  // PRINT CARD CODE
  bool new_card_code;
  ds.reset();
  ds.select(addr);
  ds.write(0xF6); // send command for read card code
  delayMicroseconds(100); // some time for slave to sum CRC from card code
  new_card_code=false; // if no card slave send card_code="0000000000"
  for ( i = 0; i < 10; i++) { // read 10 bytes representative (ascii) text value card code 
    data[i] = ds.read();
    if (data[i]!='0') new_card_code=true;
  }
  data[i] = ds.read(); // + 1 byte CRC
  if ( data[10]==OneWire::crc8(data, 10) ) { // compare CRC
    ds.write(0xAA); // send confirmation to slave
    // card code is ready 
    if (new_card_code==true) {
      Serial.print("New card code = ");
      Serial.print(" ");
      for ( i = 0; i < 10; i++) {           // we need 9 bytes
        Serial.write(data[i]);
      }
      delay(300); //wait to end carddetect beep (card code sended immediately when detect, in this time can be still sound card detec beep, thefore wait)
      Serial.println("");
      if (false) { // examle inform person by beep for anable acces (only example, must result your own acces system)
        set_status_byte(true, GREEN_LED_MASK); // light on green led
        set_beep(2500, 50); // beep
        delay(500);
        set_status_byte(true, GREEN_LED_MASK); // light off green led
      }
      else { // examle inform person by beep for disable acces
        set_status_byte(true, RED_LED_MASK); // light on green led
        set_beep(200, 30); // beep
        delay(300);
        set_status_byte(true, RED_LED_MASK); // light off green led
      }
    }
    else {
      Serial.println("No new card code.");
    }
  }
}

/*
 * READ CARD CODE WITH SIMULATION DS1990 (CARDCODE IS INCLUDED INTO ONE WIRE ADRESS)
 */
void family01_print_card_code(){
  // PRINT CARD CODE ONLY FROM ONE WIRE ADDRES [1. BYTE=0x01 (FAMILY CODE OF DS1990), 2. BYTE=CUSTOM IDENTIFICATION CARD READER, 3.-7. BYTES CONTAIN CARD CODE, 8. BYTE=CRC]
  uint8_t hex;
  
  Serial.print("Identification number card reader = ");
  Serial.println(addr[1]);
  Serial.print("New card code = ");
  Serial.print(" ");
  for ( i = 2; i < 7; i++) { // 3-7 byte
    hex=(addr[i] & 0xF0) >> 4;
    Serial.print(hex, HEX);
    hex=(addr[i] & 0x0F);
    Serial.print(hex, HEX);
  }
  Serial.println("");
}

/*
 * example function demostrate howto iniciate card reader (no used there)
 */
void ini_card_reader(void) {
  if (is_set==true) return;
  set_reader_acustic_signal(2000, 5); // sed beep at new card detect
  delayMicroseconds(500); // for send debug information if no debug then no need
  set_reader_acustic_signal(2000, 5); // sed beep at new card detect
  delayMicroseconds(500); // for send debug information if no debug then no need
  set_status_byte(false, GREEN_LED_MASK); // light off greeen led
  delayMicroseconds(500); // for send debug information if no debug then no need
  set_status_byte(false, REALY_MASK); // off relay
  delayMicroseconds(500); // for send debug information if no debug then no need
  set_simulation_DS1990(1); // this unset simulatin, other valu then 0 start simulation
  delayMicroseconds(5000); // for send debug information if no debug then no need
  set_beep(5000, 500);
  is_set=true;
}

void setup(void) {
  Serial.begin(9600);
  //ini_card_reader();
}
 
void loop(void) {
  uint8_t present = 0;

  //return;
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0xDE:
      Serial.println("  Chip = RDM6300");
      //familyDE_red_card_code();
      ini_card_reader();
      break;
    case 0x01:
      Serial.println("  Chip = DS1990");
      family01_print_card_code();
      break;
    default:
      Serial.println("Device is not a RDM6300 or DS1990 family device.");
      return;
  } 
  
  delay(20);
}
