#include <SPI.h>
#include <WiFi101.h>

#define DEBUG


/*
  // declare a static string
  #ifdef __AVR__
  #define P(name)   static const unsigned char name[] __attribute__(( section(".progmem." #name) ))
  #else
  #define P(name)   static const unsigned char name[]
  #endif

   store the HTML in program memory using the P macro
   P(message) =
     "<html><head><title>Webduino Control Example</title>"
     "<body>"
     "<h1>Test the Control!</h1>"
     "<form action='/led' method='POST'>"
     "<p><button name='led' value='0'>Turn if Off!</button></p>"
     "<p><button name='led' value='1'>Turn it On!</button></p>"
     "</form></body></html>";

   server.printP(message);
*/

char apssid[] = "ZEV";
WiFiServer server(80);

boolean needCredentials = true;
boolean needWiFi = false;
String xnetwork;
String xpassword;

struct fields {
  byte type; // 0=combo, 1=radio, 2=text, 3+=custom
  String fieldPrompt;
  String fieldName;
  String textDefault; // used for text field
  byte numDefault; // default element for dropdown
  String heading; // displays before field
  String fieldPrompts[11]; // Only 10 alpha prompts are allowed, should be enough. If not specified, valid[] value is prompt. For fieldPrompts[], use valid[] range.
  byte valid[100]; // if valid[2] = 255, valid[0] and valid[1] are low and high range. otherwise, valid[0]-valid[100] specify individual selections as 1,4,7,12, etc., and set last element to 254
};

const int numberFields = 7; // add one for zero element
fields settings[numberFields];

WiFiClient client;

void setup() {


  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");

#ifdef DEBUG
  Serial.print("Creating access point named: ");
  Serial.println(apssid);
#endif

// following is for testing ********************************************

  settings[0].fieldPrompt = "Weather Station Setup"; // first element is used for title

  settings[1].type = 2;
  settings[1].fieldPrompt = "User ID";
  settings[1].fieldName = "userid";
  settings[1].textDefault = "UserID";
  settings[1].heading = "Credentials"; // used for heading

  settings[2].type = 2;
  settings[2].fieldPrompt = "Password";
  settings[2].fieldName = "password";
  settings[2].textDefault = "Password";

  settings[3].type = 0; // text dropdown
  settings[3].fieldPrompt = "Select Auto";
  settings[3].fieldName = "auto";
  settings[3].numDefault = 2;
  settings[3].valid[0] = 0;
  settings[3].valid[1] = 2;
  settings[3].valid[2] = 255;
  settings[3].fieldPrompts[0] = "Tesla";
  settings[3].fieldPrompts[1] = "BMW";
  settings[3].fieldPrompts[2] = "Mercedes";
  settings[3].fieldPrompts[3] = "Alfa Romeo";

  settings[4].type = 0; // specific items
  settings[4].fieldPrompt = "Specific Items";
  settings[4].fieldName = "specificItems";
  settings[4].numDefault = 3;
  settings[4].valid[0] = 0;
  settings[4].valid[1] = 2;
  settings[4].valid[2] = 4;
  settings[4].valid[3] = 6;
  settings[4].valid[4] = 8;
  settings[4].valid[5] = 254;


  settings[5].type = 0; // select SSID
  settings[5].fieldPrompt = "Network SSID";
  settings[5].fieldName = "ssid";
  settings[5].numDefault = 0;
  int numSsid = WiFi.scanNetworks();
  if (numSsid > 9) numSsid = 9;
  for (byte x = 0; x < numSsid; x++) {
    String ss = WiFi.SSID(x);
    if (ss.length() == 0) settings[5].valid[x] = 254;
    else settings[5].fieldPrompts[x] = ss;
  }
  settings[5].valid[9] = 254; // just in case there are 10

  settings[6].type = 0; // range option
  settings[6].fieldPrompt = "Range";
  settings[6].fieldName = "range";
  settings[6].numDefault = 12;
  settings[6].valid[0] = 0;
  settings[6].valid[1] = 50;
  settings[6].valid[2] = 255;

//****************************************************************************

  if (WiFi.beginAP(apssid) != WL_AP_LISTENING) {
#ifdef DEBUG
    Serial.println("Creating access point failed");
#endif

    while (true);
  }

  delay(1000);
  server.begin();
  printAPStatus();
}

void loop() {
  if (needCredentials) {
    getCredentials();
  }
  if (needWiFi) {
    getWiFi();
  }
}

void getCredentials() {
  client = server.available();
  if (client) {
    Serial.println("new client");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            sendRequestHeaders();
            sendHTMLHead();
            sendHTMLBody();
            sendHTMLFooter();
            break;
          }
          else {
            currentLine = "";
          }
        }
        else if (c != '\r') {
          currentLine += c;
        }
        if (c == ',' && currentLine.substring(0, 3) == "GET") {
          currentLine.replace("%20", "");

          for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) {
            String fieldName = settings[fieldIndex].fieldName;
            byte startPosition = currentLine.indexOf("?" + fieldName + "=") + fieldName.length() + 2;
            if (startPosition != -1) {
              byte endPosition = currentLine.indexOf("&", startPosition);
              String fieldValue = currentLine.substring(startPosition, endPosition);
              Serial.println();
              Serial.println(fieldValue);
            }
          }
          client.stop();
          WiFi.end();

          needCredentials = false;
          needWiFi = true;
        }
      }
    }
    client.stop();
#ifdef DEBUG
    Serial.println("client disconnected");
    Serial.println();
#endif
  }
}

void sendRequestHeaders() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
}

void sendHTMLHead() {
  client.println(F("<html>"));
  client.println(F("<head>"));
  client.println(F("<style type=\"text/css\"> body {font-family: sans-serif; margin:50px; padding:20px; line-height: 250% } </style>"));
  client.println("<title>" + settings[0].fieldPrompt + "</title>");
  client.println(F("</head>"));
}

void sendHTMLBody() {
  byte xLow;
  byte xHigh;
  byte xFlag;
  String idVal;
  String xOpt;
  String sel;

  client.println("<body>");
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) { // note that 0 is used for title
    String fieldName = settings[fieldIndex].fieldName;
    if (settings[fieldIndex].heading != "") client.println("<h2>" + settings[fieldIndex].heading + "</h2>");
    switch (settings[fieldIndex].type) {
      case 0: // dropdown
        {
          client.print(settings[fieldIndex].fieldPrompt + "<br>");
          client.println("<select id=" + String("\"") + settings[fieldIndex].fieldName + "\">");
          if (settings[fieldIndex].valid[2] == 255) { // valid[0] and valid[1] specifiy low and high range
            xLow = settings[fieldIndex].valid[0];
            xHigh = settings[fieldIndex].valid[1];
            xFlag = true;
          }
          else // use all valid[] for specific options
          {
            xLow = 0;
            xHigh = 99;
            xFlag = false;
          }
          for (int option = xLow; option <= xHigh; option++) {

            if (xFlag) { // range
              if (option < 10 && settings[fieldIndex].fieldPrompts[option] != "") xOpt = settings[fieldIndex].fieldPrompts[option]; //only 10 alpha options allowed
              else xOpt = String(option); // after 10 or null value, use option as prompt
              idVal = option;
            }
            else { // specific options
              if (option < 10 && settings[fieldIndex].fieldPrompts[option] != "") xOpt = settings[fieldIndex].fieldPrompts[option]; //only 10 alpha options allowed
              else  xOpt = settings[fieldIndex].valid[option];
              idVal = xOpt;
              if (settings[fieldIndex].valid[option] == 254) break; // leave if last option
            }
            if (option == settings[fieldIndex].numDefault) sel = "selected"; // set default selection
            else sel = "";
            client.println("<option value=\"" + idVal + "\"" + sel + ">" + xOpt + "</option>");
          }
          client.println(F("</select>"));
          client.print(F("<br>"));
          break;
        }
      case 1: // radio
        {
          client.print(settings[fieldIndex].fieldPrompt + "<br>");
          for (int opt = 0; opt < 11; opt++) {
            if (settings[fieldIndex].fieldPrompts[opt] == "") break;
            //      client.print("<input type=\"radio\"" + String("name=\"r1\"")  + "value=\"" + String(opt) + "\">" + settings[fieldIndex].fieldPrompts[opt] + "<br>");
            client.print("<input type=\"radio\"name=\"" + settings[fieldIndex].fieldName + "\""  + "value=\"" + opt + "\"" + "id=\"" + settings[fieldIndex].fieldPrompts[opt] + "\">" + settings[fieldIndex].fieldPrompts[opt] + "</input><br>");
          }
          client.print("<script>" + settings[fieldIndex].fieldName + "=document.getElementById('" + settings[fieldIndex].fieldName + "').value</script>");
          client.print(F("<br>"));
          break;
        }
      case 2: // text
        {
          client.print(settings[fieldIndex].fieldPrompt + "<br>");
          client.print("<input id=\"" + settings[fieldIndex].fieldName + "\"" + "value=\"" + settings[fieldIndex].textDefault + "\"><br>");
          break;
        }
    }
  }
  client.print(F("<br>"));
  client.print(F("<button type=\"button\" onclick=\"SendText()\">Enter</button>"));
  client.println(F("</body>"));
}

void sendHTMLFooter() {
  client.println(F("<script>"));
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) {
    String fieldName = settings[fieldIndex].fieldName;
    client.println("var " + fieldName + " = document.querySelector('#" + fieldName + "');");
  }
  client.println(F("function SendText() {"));
  client.println("var nocache=\"&nocache=\" + Math.random() * 1000000;");
  client.println(F("var request = new XMLHttpRequest();"));
  client.print(F("var netText = \"&txt="));

  bool first = true;
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) {
    String fieldName = settings[fieldIndex].fieldName;
    if (first) client.print(F("?"));
    else client.print(F("\"&?"));
    first = false;
    client.print(fieldName + "=\"" + "+" + fieldName + ".value" + "+");
  }

  client.println(F("\"&,&end=end\";"));
  client.println(F("request.open(\"GET\",\"ajax_inputs\" +  netText + nocache, true);"));
  client.println(F("request.send(null);"));
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) { // clear fields
    String fieldName = settings[fieldIndex].fieldName;
    client.println(fieldName + ".value" + "='';");
  }
  client.println(F("}</script>"));
  client.println(F("</html>"));
  client.println();
}

void getWiFi() {
  if (xnetwork == "" or xpassword == "") {
    Serial.println("Invalid WiFi credentials");
    Serial.println("STOP!!!!!!!!!!");
    while (true);
  }

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    WiFi.begin(xnetwork, xpassword);
    delay(10000);
  }
  Serial.println("WiFi connection successful");
  printWiFiStatus();
  needWiFi = false;
  delay(1000);
}

void printWiFiStatus() {
#ifdef DEBUG
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
#endif
}

void printAPStatus() {
#ifdef DEBUG
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("To connect, open a browser to http://");
  Serial.println(ip);
#endif
}
