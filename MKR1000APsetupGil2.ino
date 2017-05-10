#include <SPI.h>
#include <WiFi101.h>

#define DEBUG

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
  String textDefault;
  int numDefault;
  int valid; // range as 1-20, or each valid number as 1,4,7,9 etc.
  String heading; // displays before field
  String fieldPrompts[10];
};

const int numberFields = 5;
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

  settings[0].fieldPrompt = "Weather Station Setup"; // first element is used for title

  settings[1].type = 2;
  settings[1].fieldPrompt = "User ID";
  settings[1].fieldName = "userid";
  settings[1].textDefault = "";
  settings[1].heading = "Credentials"; // first element is used for title


  settings[2].type = 2;
  settings[2].fieldPrompt = "Password";
  settings[2].fieldName = "password";
  settings[2].textDefault = "yes";

  settings[3].type = 0;
  settings[3].fieldPrompt = "Select Auto";
  settings[3].fieldName = "auto";
  settings[3].textDefault = "";
  settings[3].fieldPrompts[0] = "Tesla";
  settings[3].fieldPrompts[1] = "BMW";
  settings[3].fieldPrompts[2] = "Mercedes";
  settings[3].fieldPrompts[3] = "Alfa Romeo";

  settings[4].type = 1;
  settings[4].fieldPrompt = "Pick One";
  settings[4].fieldName = "auto";
  settings[4].textDefault = "";
  settings[4].fieldPrompts[0] = "Tesla";
  settings[4].fieldPrompts[1] = "BMW";
  settings[4].fieldPrompts[2] = "Mercedes";
  settings[4].fieldPrompts[3] = "Alfa Romeo";




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
  client.println("<html>");
  client.println("<head>");
  client.println("<style type=\"text/css\"> body {font-family: sans-serif; margin:50px; padding:20px; line-height: 250% } </style>");
  client.println("<title>" + settings[0].fieldPrompt + "</title>");
  client.println("</head>");
}

void sendHTMLBody() {
  client.println("<body>");
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) { // note that 0 is used for title
    String fieldName = settings[fieldIndex].fieldName;
    if (settings[fieldIndex].heading != "") client.println("<h2>" + settings[fieldIndex].heading + "</h2>");
    switch (settings[fieldIndex].type) {
      case 0: // combo
        {
          client.print(settings[fieldIndex].fieldPrompt + " ");
          client.println("<select>");
          for (int opt = 0; opt < 11; opt++) {
            if (settings[fieldIndex].fieldPrompts[opt] == "") break;
            client.println("<option value=\"" + String(opt) + "\">" + settings[fieldIndex].fieldPrompts[opt] + "</option>");
          }
          client.println("</select>");
          client.print("<br>");
          break;
        }
      case 1: // radio
        {
          client.print(settings[fieldIndex].fieldPrompt + " ");

          for (int opt = 0; opt < 11; opt++) {
            if (settings[fieldIndex].fieldPrompts[opt] == "") break;
            client.print("<input type='radio'" + String("name='source'")  + "value='" + String(opt) + "'>" + settings[fieldIndex].fieldPrompts[opt] + "<br>");
          }
          client.print("<br>");
          break;
        }

      case 2: // text
        {
          client.print(settings[fieldIndex].fieldPrompt + " ");
          client.print("<input id=\"" + settings[fieldIndex].fieldName + "\"" + "value=\"" + settings[fieldIndex].textDefault + "\"><br>");
          break;
        }
    }
  }
  client.print("<br>");
  client.print("<button type=\"button\" onclick=\"SendText()\">Enter</button>");
  client.println("</body>");
}

/*  radio button
  client.print("Data Source: ");
  client.print("<br>");
  client.print("<input type='radio' name=source value='2'> Anemometer<br>");
  client.print("<input type='radio' name=source value='3'> Weather Underground<br>");

********* combobox
  <select>
  <option value="volvo">Volvo</option>
  <option value="saab">Saab</option>
  <option value="opel">Opel</option>
  <option value="audi">Audi</option>
  </select>

*/

void sendHTMLFooter() {
  client.println("<script>");
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) {
    String fieldName = settings[fieldIndex].fieldName;
    client.println("var " + fieldName + " = document.querySelector('#" + fieldName + "');");
  }
  client.println("function SendText() {");
  client.println("var nocache=\"&nocache=\" + Math.random() * 1000000;");
  client.println("var request = new XMLHttpRequest();");
  client.print("var netText = \"&txt=");

  bool first = true;
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) {
    String fieldName = settings[fieldIndex].fieldName;
    if (first) client.print("?");
    else client.print("\"&?");
    first = false;
    client.print(fieldName + "=\"" + "+" + fieldName + ".value" + "+");
  }

  client.println("\"&,&end=end\";");
  client.println("request.open(\"GET\",\"ajax_inputs\" +  netText + nocache, true);");
  client.println("request.send(null);");
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) { // clear fields
    String fieldName = settings[fieldIndex].fieldName;
    client.println(fieldName + ".value" + "='';");
  }

  client.println("}</script>");
  client.println("</html>");
  client.println();
}

void getWiFi() {
  if (xnetwork == "" or xpassword == "") {
    Serial.println("Invalid WiFi credentials");
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
