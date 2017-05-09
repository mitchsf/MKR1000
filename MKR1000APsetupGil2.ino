#include <SPI.h>
#include <WiFi101.h>
//#include <HashMap.h>

#define DEBUG

// Constants
const String FIELD_USERNAME = "Username";
const String FIELD_PASSWORD = "Password";
const String ALL_FIELDS[] = {FIELD_USERNAME, FIELD_PASSWORD};
byte numFields = sizeof(ALL_FIELDS) / sizeof(ALL_FIELDS[0]);

//const byte HASH_SIZE = sizeof(ALL_FIELDS);
//HashType<String, String> hashRawArray[HASH_SIZE];
//HashMap<String, String> fieldMap = HashMap<String, String>(hashRawArray, HASH_SIZE);

char apssid[] = "ZEV";
int status = WL_IDLE_STATUS;
WiFiServer server(80);
String HTTP_req;
boolean readingNetwork = false;
boolean readingPassword = false;
String password = "";
String network = "";
boolean needCredentials = true;
boolean needWiFi = false;
int netIndex;
int passIndex;
String xnetwork;
String xpassword;
//String w[10];

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
        if (c == '&' && currentLine.substring(0, 3) == "GET") {
          Serial.println();
          Serial.println("testxx");
          Serial.println(currentLine.substring(0, 3));
          currentLine.replace("%20", "");

          for (int fieldIndex = 0; fieldIndex < numFields; fieldIndex++) {
            String fieldName = ALL_FIELDS[fieldIndex];
            byte startPosition = currentLine.indexOf("?" + fieldName + "=");
            if (startPosition != -1) {
              byte endPosition = currentLine.indexOf("&", startPosition + 1);
              String fieldValue = currentLine.substring(startPosition, endPosition);
    //          fieldMap[fieldIndex](fieldName, currentLine.indexOf("?" + fieldName + "="));
              Serial.println(fieldValue);
            }
          }
       //   if (currentLine.indexOf("?Username=") != -1) netIndex = currentLine.indexOf("?Username=");
        //    if (currentLine.indexOf("?Password=") != -1)passIndex = currentLine.indexOf("?Password=");
         //     xnetwork = currentLine.substring(netIndex + 9, passIndex);
         //     xpassword = currentLine.substring(passIndex + 10, currentLine.indexOf(","));

              client.stop();
              WiFi.end();
              readingPassword = false;
              needCredentials = false;
              needWiFi = true;
              Serial.println();
              Serial.println(xnetwork);
              Serial.println(xpassword);
            }
      }
    }
    delay(1);
    client.stop();
    Serial.println("client disconnected");
    Serial.println();
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
  client.println("<title>Arduino Setup</title>");
  client.println("</head>");
}

void sendHTMLBody() {

  client.println("<body>");
  client.println("<h2>WiFi Networks</h2>");

  client.println("<h2>WiFi Credentials</h2>");
  for (int fieldIndex = 0; fieldIndex < numFields; fieldIndex++) {
    String fieldName = ALL_FIELDS[fieldIndex];
    client.print(fieldName + ": ");
    client.print("<input id=\"" + fieldName + "\"><br>");
  }
  client.print("<br>");
  client.print("<button type=\"button\" onclick=\"SendText()\">Enter</button>");
  client.println("</body>");
  //*************************

  //***************************
}

void sendHTMLFooter() {
  client.println("<script>");
  for (int fieldIndex = 0; fieldIndex < numFields; fieldIndex++) {
    String fieldName = ALL_FIELDS[fieldIndex];
    client.println("var " + fieldName + " = document.querySelector('#" + fieldName + "');");
  }
  client.println("function SendText() {");
  client.println("var nocache=\"&nocache=\" + Math.random() * 1000000;");
  client.println("var request = new XMLHttpRequest();");
  client.print("var netText = \"&txt=");

  bool first = true;
  for (int fieldIndex = 0; fieldIndex < numFields; fieldIndex++) {
    String fieldName = ALL_FIELDS[fieldIndex];
    if (first) client.print("?");
    else client.print("\"&");
    first = false;
    client.print(fieldName + "=\"" + "+" + fieldName + ".value" + "+");
  }
  client.println("\"&end=end\";");

  client.println("request.open(\"GET\",\"ajax_inputs\" +  netText + nocache, true);");
  client.println("request.send(null)");
  client.println("Username.value=''");
  client.println("Password.value=''}");
  client.println("</script>");
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
    Serial.println(xnetwork);
    WiFi.begin(xnetwork, xpassword);
    delay(10000);
  }
  Serial.println("WiFi connection successful");
  printWiFiStatus();
  needWiFi = false;
  delay(1000);
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

void printAPStatus() {
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
}
