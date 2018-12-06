/*
 * // If user is using the newer Ethernet shield than this one??!
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet2.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp2.h>
#include <Twitter.h>
#include <util.h>
*/
/*
  MySQL Connector/Arduino Example : Ethernet Connection to SQL server

  This example demonstrates how to issue an INSERT and SELECT query to store 
  data in a table. For this, we will create a special database and table for testing.
  The following link demonstrates to how to set up the wampserver and the database for 
  this scenario cases: 
  https://www.instructables.com/id/Connecting-Arduino-to-MySQl-database-w-USB-using-M/      

  INSTRUCTIONS FOR USE

  0) Install wmamp server program and cofigure the SQL server as described in internet page:
  https://www.instructables.com/id/Connecting-Arduino-to-MySQl-database-w-USB-using-M/  
    - remember to link the user account to created table and to grant priviledges onto it!!!  
  1) Create the database and table as shown above.
  2) Change the address of the server to the IP address of the MySQL server
  3) Change the user and password to a valid MySQL user and password
  4) Connect a USB cable to your Arduino
  5) Select the correct board and port
  6) Compile and upload the sketch to your Arduino
  7) Once uploaded, open Serial Monitor (use 115200 speed) and observe
  8) After the sketch has run for some time, open a mysql client and issue
     the command: "SELECT * FROM arduino.sensors" to see the data
     recorded. Note the field values and how the database handles both the
     auto_increment and timestamp fields for us. You can clear the data with
     "DELETE FROM arduino.sensors".

  Note: The MAC address can be anything so long as it is unique on your network.

  Created by: Dr. Charles A. Bell
  Modified by: Bsc. Jani Ärväs
*/

// The connection_data struct needs to be defined in an external file.
#include "EthernetConnection.h"
#include <Ethernet.h>
#include <SPI.h>

#include <IRremote.h>

#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

//Imports the BitVoicer library to the sketch
#include "BitVoicer11.h"
//Instantiates the BitVoicerSerial class
BitVoicerSerial bvSerial = BitVoicerSerial();

//Stores the data type retrieved by getData()
byte dataType = 0;
//Stores the state of pin 15
byte pinVal = 0;
byte dataStart1 = 255;
byte dataStart5 = 0;


// Internal functions
int sendsqltomydb( MySQL_Connection conn, int readwrite );
void sendCode(int repeat);
void storeCode(decode_results *results);

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac_addr[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };

IPAddress server_addr(192, 168, 1, 80);    // IP of the MySQL *server* here
char user[] = "poop";                   // MySQL user login username
char password[] = "poop";               // MySQL user login password
int port = 3306;

char SELECT_SQL[] = "SELECT value FROM arduino.sensors where id=1";
char LIGHTSONN_SQL[] = "UPDATE arduino.sensors SET value=100 WHERE id=1";
char LIGHTSOFF_SQL[] = "UPDATE arduino.sensors SET value=200 WHERE id=1";
//char COMMIT_SQL[] = "COMMIT;";

EthernetClient client;
MySQL_Connection conn((Client *)&client);

// Status and input pin for inserting the data
//int ledPin = 53; // choose the pin for the LED (pitää vissiin  olla define
//int inPin = 7;   // choose the input pin (for a pushbutton)
int val = 0;     // variable for reading the pin status
int RECV_PIN = 2;
int BUTTON_PIN = 12;
int STATUS_PIN = 13;

int READVALUE = 1;
int WRITEVALUE = 2;

int lastButtonState;

IRrecv irrecv(RECV_PIN);
IRsend irsend;

decode_results results;

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn(); // Start the receiver
  
  pinMode(BUTTON_PIN, INPUT);
  pinMode(STATUS_PIN, OUTPUT);
 // pinMode(voiceInput, INPUT);
 // pinMode(voiceOutput, OUTPUT);

  // Pins for inputs and outputs of SQLs
  pinMode(53, OUTPUT);  // declare LED as output
  digitalWrite(53, HIGH);
  //pinMode(inPin, INPUT);    // declare pushbutton as input

  Serial.println("Initialize Ethernet with DHCP:");
  while (!Serial); // wait for serial port to connect
  Ethernet.begin(mac_addr);
  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  Serial.println("Connecting to SQL server.");
  if (conn.connect(server_addr, 3306, user, password)) {
    delay(1000);
    Serial.println("Connected to the server.");
  }
  else {
    Serial.println("Connection failed.");
    conn.close();
  }
  bvSerial.setAudioInput(15);
bvSerial.startStopListening();
bvSerial.sendToBV( dataStart1 );
bvSerial.sendToBV( dataStart1 );
bvSerial.sendToBV( dataStart1 );
bvSerial.sendToBV( dataStart1 );
bvSerial.sendToBV( dataStart5 );
bvSerial.sendToBV( dataStart5 );
bvSerial.sendToBV( dataStart5 );
bvSerial.sendToBV( dataStart5 );
}

void loop() {
  //delay(1000);

   //Updates the state of pin 4 on every loop
  digitalWrite(52, pinVal);
  //Serial.println("Storing the value of pin 15");
  
  // If button pressed, send the code.
  int buttonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && buttonState == LOW) {
    Serial.println("Released");
    irrecv.enableIRIn(); // Re-enable receiver
  }
  //Check if the remote control used?
  if (buttonState) {
    Serial.println("Pressed, sending");
    digitalWrite(STATUS_PIN, HIGH);
    sendCode(lastButtonState == buttonState);
    digitalWrite(STATUS_PIN, LOW);
    // Send values to MySQL database
    delay(50); // Wait a bit between retransmissions
  } 
  else if (irrecv.decode(&results)) {
    digitalWrite(STATUS_PIN, HIGH);
    storeCode(&results);
    irrecv.resume(); // resume receiver
    digitalWrite(STATUS_PIN, LOW);
  }
  lastButtonState = buttonState;

      //Reads the serial buffer and stores the received data type
//  dataType = bvSerial.getData();
/*  Serial.println("Datatype: ");
  Serial.println(dataType);
  //Checks if the data type is the same as the one in the
  //Voice Schema
 if(dataType == BV_STR){
   if(bvSerial.strData == "onn")
    {
      Serial.println("Lights on...");
    }
    else if(bvSerial.strData == "off")
    {
      Serial.println("Lights off...");
    }
    else
    {
    }
 }*/
}

int sendsqltomydb( MySQL_Connection conn, int readwrite )
{
  row_values *row = NULL;
  long head_count = 0;
  int i=0;

  Serial.println("Tying to communicate with SQL server");
  // Initiate the query class instance
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  //if(readwrite == READVALUE){
  // Execute the query
  if( cur_mem->execute( SELECT_SQL ) == true ){
    // Fetch the columns and print them
    column_names *cols = cur_mem->get_columns();
    //cur_mem->show_results();
    // Read the row (we are only expecting the one)
    do {
      row = cur_mem->get_next_row();
      if (row != NULL) {
        head_count = atol(row->values[0]);
        Serial.println(head_count);
      }
    } while (row != NULL);
switch (head_count){
case 100:
  if( cur_mem->execute( LIGHTSOFF_SQL ) == true ){
  }
  digitalWrite(53, LOW);
  Serial.println("Lights On!");
  delay(100);
 break;
case 200:
  if( cur_mem->execute( LIGHTSONN_SQL ) == true ){
  }
  digitalWrite(53, HIGH);
  Serial.println("Lights Off!");
  delay(100);
  break;
 default:
 Serial.println("Doing nothing...");
 break;
  }

    // Deleting the cursor also frees up memory used
    // Note: since there are no results, we do not need to read any data
    // Deleting the cursor also frees up memory used
   // delete cur_mem;
    return true;
  }
  else {
    // Note: since there are no results, we do not need to read any data
    // Deleting the cursor also frees up memory used
    delete cur_mem;
    return false;
  }
}

// Storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) {
  codeType = results->decode_type;
  //int count = results->rawlen;
  if (codeType == UNKNOWN) {
    Serial.println("Received unknown code, saving as raw");
    codeLen = results->rawlen - 1;
    // To store raw codes:
    // Drop first value (gap)
    // Convert from ticks to microseconds
    // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
    for (int i = 1; i <= codeLen; i++) {
      if (i % 2) {
        // Mark
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK - MARK_EXCESS;
        Serial.print(" m");
      } 
      else {
        // Space
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK + MARK_EXCESS;
        Serial.print(" s");
      }
      Serial.print(rawCodes[i - 1], DEC);
    }
    Serial.println("");
  }
  else {
    if (codeType == NEC) {
      Serial.print("Received NEC: ");
      if (results->value == REPEAT) {
        // Don't record a NEC repeat value as that's useless.
        Serial.println("repeat; ignoring.");
        return;
      }
    } 
    else if (codeType == SONY) {
      Serial.print("Received SONY: ");
    } 
    else if (codeType == PANASONIC) {
      Serial.print("Received PANASONIC: ");
    }
    else if (codeType == JVC) {
      Serial.print("Received JVC: ");
    }
    else if (codeType == RC5) {
      Serial.print("Received RC5: ");
    } 
    else if (codeType == RC6) {
      Serial.print("Received RC6: ");
    } 
    else {
      Serial.print("Unexpected codeType ");
      Serial.print(codeType, DEC);
      Serial.println("");
    }
    Serial.println(results->value, HEX); // Print HEX Value of the pressed button
    Serial.println(results->value);      // Print integer value of the pressed button
  }
  if(results->value == 0xFFB04F || results->value == 16756815 ){
      Serial.println("Reading");
      if( sendsqltomydb( conn, READVALUE ) > 0 ){  // Try to send the values to SQL server
        Serial.println("Communication with SQL server was Ok.");
        //conn.close();             // Close used connection
        //delay(1000);
      }
    }
 /* if(results->value==16738455 || results->value==0xFF6897){
    Serial.println("Writing");
    if( sendsqltomydb( conn, WRITEVALUE ) > 0 ){  // Try to send the values to SQL server
      Serial.println("Communication with SQL server was Ok.");
      //conn.close();             // Close used connection
      //delay(1000);
    }
  }*/
    codeValue = results->value;
    codeLen = results->bits;
  
}

void sendCode(int repeat) {
  if (codeType == NEC) {
    if (repeat) {
      irsend.sendNEC(REPEAT, codeLen);
      Serial.println("Sent NEC repeat");
    } 
    else {
      irsend.sendNEC(codeValue, codeLen);
      Serial.print("Sent NEC ");
      Serial.println(codeValue, HEX);
    }
  } 
  else if (codeType == SONY) {
    irsend.sendSony(codeValue, codeLen);
    Serial.print("Sent Sony ");
    Serial.println(codeValue, HEX);
  } 
  else if (codeType == PANASONIC) {
    irsend.sendPanasonic(codeValue, codeLen);
    Serial.print("Sent Panasonic");
    Serial.println(codeValue, HEX);
  }
  else if (codeType == JVC) {
    irsend.sendJVC(codeValue, codeLen, false);
    Serial.print("Sent JVC");
    Serial.println(codeValue, HEX);
  }
  else if (codeType == RC5 || codeType == RC6) {
    if (!repeat) {
      // Flip the toggle bit for a new button press
      toggle = 1 - toggle;
    }
    // Put the toggle bit into the code to send
    codeValue = codeValue & ~(1 << (codeLen - 1));
    codeValue = codeValue | (toggle << (codeLen - 1));
    if (codeType == RC5) {
      Serial.print("Sent RC5 ");
      Serial.println(codeValue, HEX);
      irsend.sendRC5(codeValue, codeLen);
    } 
    else {
      irsend.sendRC6(codeValue, codeLen);
      Serial.print("Sent RC6 ");
      Serial.println(codeValue, HEX);
    }
  } 
  else if (codeType == UNKNOWN /* i.e. raw */) {
    // Assume 38 KHz
    irsend.sendRaw(rawCodes, codeLen, 38);
    Serial.println("Sent raw");
  }
}

//This function runs every time serial data is available
//in the serial buffer after a loop
void serialEvent()
{
  //Reads the serial buffer and stores the received data type
  dataType = bvSerial.getData();
  Serial.println("Datatype: ");
  Serial.println(dataType);
  //Checks if the data type is the same as the one in the
  //Voice Schema
  if (dataType == BV_BYTE)
  {
    Serial.println("Changing the value...");
    //Checks the stored value in byteData by getData() and
    //changes the value of the pin
    if (bvSerial.byteData == 0)
      pinVal = LOW;
    else
      pinVal = HIGH;
  }
 if(dataType == BV_STR){
   if(bvSerial.strData == "onn")
    {
      Serial.println("Lights on...");
      bvSerial.sendToBV( dataStart1 );
      bvSerial.sendToBV( dataStart1 );
      bvSerial.sendToBV( dataStart1 );
      bvSerial.sendToBV( dataStart1 );
      bvSerial.sendToBV( dataStart5 );
      bvSerial.sendToBV( dataStart5 );
      bvSerial.sendToBV( dataStart5 );
      bvSerial.sendToBV( dataStart5 );
      bvSerial.startStopListening();
      digitalWrite(53, LOW);
    }
    else if(bvSerial.strData == "off")
    {
      Serial.println("Lights off...");
      bvSerial.sendToBV( dataStart1 );
      bvSerial.sendToBV( dataStart1 );
      bvSerial.sendToBV( dataStart1 );
      bvSerial.sendToBV( dataStart1 );
      bvSerial.sendToBV( dataStart5 );
      bvSerial.sendToBV( dataStart5 );
      bvSerial.sendToBV( dataStart5 );
      bvSerial.sendToBV( dataStart5 );
      bvSerial.startStopListening();
      digitalWrite(53, HIGH);
    }
    else
    {
    }
 }
}

