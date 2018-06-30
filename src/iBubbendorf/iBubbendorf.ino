/*
   Arduino Bubbendorf remote control
   Inspired from the Webserver sample code.
   
   @Blog : http://www.geeek.org/comment-domotiser-ses-volets-radio-pour-moins-de-50-960.html
   @Repository : https://github.com/ltoinel/ibubbendorf
   @Author : Ludovic Toinel
   @Contributor : Nicolas Richeton
 */

#define DEBUG false
#define BUFSIZE 255

#include <SPI.h>
#include <Ethernet.h>

// UP and Down Button attached the the arduino inputs
// You should adapt these values to your arduino setup.
const int PIN_DOWN = 7;
const int PIN_UP = 8;
const int BUTTON_PUSH = LOW; 
const int BUTTON_RELEASE = HIGH; 

// Press delay on the buttons
const int BUTTON_PRESS_DELAY = 500;

// Half Open configuration 
const int SHUTTER_CLOSE_DELAY = 20000; 
const int SHUTTER_HALF_OPEN_DELAY = 2000; 


// Allowed commands 
const String COMMAND_UP = "up";
const String COMMAND_DOWN = "down";
const String COMMAND_HALF = "half";
const String COMMAND_SWITCH = "switch";

// Available status
const String STATUS_OPEN = "open";
const String STATUS_CLOSE = "close";
const String STATUS_HALF = "half";
const String STATUS_UNKNOWN = "unknown";


// Buffer
char buffer[BUFSIZE];
int index = 0;

// Current shutters status
String status = STATUS_UNKNOWN;

// Enter a MAC address for your controller below.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0xED };

// Initialize the Ethernet server library
// with the port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // initialize the digital pin as an output.
  pinMode(PIN_DOWN, OUTPUT);    
  pinMode(PIN_UP, OUTPUT);    
  digitalWrite(PIN_DOWN, BUTTON_RELEASE);
  digitalWrite(PIN_UP, BUTTON_RELEASE);
   
  // start the Ethernet connection and the server:
  Ethernet.begin(mac);
  server.begin();
  
#if DEBUG
  Serial.print("WebServer started at ");
  Serial.println(Ethernet.localIP());
#endif
}

// Open the shutters
void openShutters(){
    digitalWrite(PIN_UP, BUTTON_PUSH); 
    delay(BUTTON_PRESS_DELAY);   
    digitalWrite(PIN_UP, BUTTON_RELEASE);
    status = STATUS_OPEN;     
}

// Close the shutters
void closeShutters(){             
    digitalWrite(PIN_DOWN, BUTTON_PUSH); 
    delay(BUTTON_PRESS_DELAY);   
    digitalWrite(PIN_DOWN, BUTTON_RELEASE);     
    status = STATUS_CLOSE;
}

// Close the shutters
void sendResponse(EthernetClient client, String status){             
    // send a standard http response header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close"); 
    client.println("Access-Control-Allow-Origin: *");
    client.println("Cache-Control: no-cache, no-store, must-revalidate");
    client.println("Pragma: no-cache");
    client.println("Expires: 0");
    client.println();
    client.print("{\"status\": 0, \"message\" : \"command succeeded\", \"state\": \"");
    client.print(status);
    client.println("\"}");
}

// Half-close the shutters
void halfCloseShutters(){ 
  // First : close the shutters
  if( status != STATUS_CLOSE  ) {
    closeShutters();     
    delay(SHUTTER_CLOSE_DELAY);    
  }

    // Start opening
    openShutters();
    delay(SHUTTER_HALF_OPEN_DELAY);

    // Stop movement after delay
    closeShutters();
    status = STATUS_HALF;
}

// infinite loop function
void loop() {
  // Keep DHCP lease
  Ethernet.maintain();
  
  // listen for incoming clients
  EthernetClient client = server.available();
  
  if (client) { 
    //  Reset input buffer
    index = 0;
    for (int i = 0; i < (sizeof(buffer)); i++) {
       buffer[i] = '\0';
    }

#if DEBUG    
    Serial.println("New HTTP client");
#endif

    // While new clients connect to the Arduino
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        //  fill the buffer
        if(c != '\n' && c != '\r' && index < BUFSIZE){ // Reads until either an eol character is reached or the buffer is full
          buffer[index++] = c;
          continue;
        }  
        buffer[index] = '\0' ;
        
        client.flush();
          
#if DEBUG
        Serial.print("Request = "); 
        Serial.println(buffer);
#endif        
        
        // Transform the char buffer into a String
        String request = String(buffer);
        
        //  we're only interested in the first part...
        int qr = request.indexOf('/') + 1;
        String command = request.substring(qr, request.indexOf(' ', qr));
        
#if DEBUG
        Serial.print("Command = "); 
        Serial.println(command);
#endif    

        // Up 
        if(command == COMMAND_UP){ 
          openShutters();
          sendResponse(client, status);
          
        // Down
        } else if (command == COMMAND_DOWN) {
          closeShutters();
          sendResponse(client, status);
          
        // Half
        } else if (command == COMMAND_HALF) {
          //Send response first because we will wait a lot during process
          sendResponse(client, STATUS_HALF);
          halfCloseShutters();

        // Switch
        } else if (command == COMMAND_SWITCH) {
          if(status == STATUS_OPEN ){
             closeShutters();
          } else {
              openShutters();
          }
          
          sendResponse(client, status);
           
        } else {
         
          // send a standard http 404 response header
          client.println("HTTP/1.1 404 OK");
          client.println("Content-Type: application/json");
          client.println("Connnection: close");
          client.println("Access-Control-Allow-Origin: *");
          client.println("Cache-Control: no-cache, no-store, must-revalidate");
          client.println("Pragma: no-cache");
          client.println("Expires: 0");
          client.println();
          client.println("{\"status\": -1, \"message\": \"command not found\", \"state\": \"");client.print(status);client.println("\"}");
          break;
        }
        

        break;
      }
    }
    // give the web browser time to receive the data
    delay(1);
    
    // close the connection:
    client.stop();
    
#if DEBUG
    Serial.println("Client disconnected");
#endif
  }
}

