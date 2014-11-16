/*
   Arduino Bubbendorf remote control
   Inspired from the Webserver sample code.
   
   @Blog : http://www.geeek.org/comment-domotiser-ses-volets-radio-pour-moins-de-50-960.html
   @Repository : https://github.com/ltoinel/ibubbendorf
   @Author : Ludovic Toinel
 */

#define DEBUG false
#define BUFSIZE 255

#include <SPI.h>
#include <Ethernet.h>

// UP and Down Button
const int DOWN = 7;
const int UP = 8;

// Allowed commands 
const String COMMAND_UP = "up";
const String COMMAND_DOWN = "down";
 
 
char buffer[BUFSIZE];
int index = 0;
String status = "UNKNOWN";

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0xED };
IPAddress ip(192,168,1,4);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // initialize the digital pin as an output.
  pinMode(DOWN, OUTPUT);    
  pinMode(UP, OUTPUT);    
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  
#if DEBUG
  Serial.print("WebServer started at ");
  Serial.println(Ethernet.localIP());
#endif

}

void loop() {
  
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
        if(command == "up") {
           
          digitalWrite(UP, HIGH); 
          delay(1000);   
          digitalWrite(UP, LOW);
          status = "OPEN";     
          
        // Down
        } else if (command == "down") {
                     
          digitalWrite(DOWN, HIGH); 
          delay(1000);   
          digitalWrite(DOWN, LOW);     
          status = "CLOSE";
          
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
        
        // send a standard http response header
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close"); 
        client.println("Access-Control-Allow-Origin: *");
        client.println("Cache-Control: no-cache, no-store, must-revalidate");
        client.println("Pragma: no-cache");
        client.println("Expires: 0");
        client.println();
        client.print("{\"status\": 0, \"message\" : \"command succeeded\", \"state\": \"");client.print(status);client.println("\"}");
        break;
      }
    }
    // give the web browser time to receive the data
    delay(1);
    
    // close the connection:
    client.stop();
    
#if DEBUG
    Serial.println("Client disonnected");
#endif
  }
}

