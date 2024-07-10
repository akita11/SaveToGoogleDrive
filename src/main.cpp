#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <pgmspace.h>

// ref: https://www.geosense.co.jp/blog_m5upload/

String mGasServer, mGasPath;

String gasUrl = "https://script.google.com/macros/s/.../exec";
const char *WIFI_SSID = "WIFI_SSID";
const char *WIFI_PWD = "WIFI_PWD";

#define FILENAME "hoge.txt"
#define FOLDER "hoge"

const char PROGMEM b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

int base64_enc_len(int plainLen) {
	int n = plainLen;
	return (n + 2 - ((n + 2) % 3)) / 3 * 4;
}

inline void a3_to_a4(unsigned char * a4, unsigned char * a3) {
	a4[0] = (a3[0] & 0xfc) >> 2;
	a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
	a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
	a4[3] = (a3[2] & 0x3f);
}

int base64_encode(char *output, char *input, int inputLen) {
	int i = 0, j = 0;
	int encLen = 0;
	unsigned char a3[3];
	unsigned char a4[4];

	while(inputLen--) {
		a3[i++] = *(input++);
		if(i == 3) {
			a3_to_a4(a4, a3);

			for(i = 0; i < 4; i++) {
				output[encLen++] = pgm_read_byte(&b64_alphabet[a4[i]]);
			}

			i = 0;
		}
	}

	if(i) {
		for(j = i; j < 3; j++) {
			a3[j] = '\0';
		}

		a3_to_a4(a4, a3);

		for(j = 0; j < i + 1; j++) {
			output[encLen++] = pgm_read_byte(&b64_alphabet[a4[j]]);
		}

		while((i++ < 3)) {
			output[encLen++] = '=';
		}
	}
	output[encLen] = '\0';
	return encLen;
}

String urlencode(String str)
{
	//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
}

void setup() {
	M5.begin();

	WiFi.disconnect();
	delay(500);
	WiFi.mode(WIFI_STA); //init wifi mode
  WiFi.begin(WIFI_SSID, WIFI_PWD);
 	while (WiFi.status() != WL_CONNECTED){
   	printf(".\n");
		delay(500);
	}
	printf("connected\n");

	int p1 = gasUrl.indexOf( "://" );
//	if ( p1 < 0 ) return -1;
	p1 += 3;
	int p2 = gasUrl.indexOf( "/", p1 ); 
	mGasServer = gasUrl.substring( p1, p2 );
	mGasPath = gasUrl.substring( p2 );

	printf("mGasServer=%s\n", mGasServer.c_str());
	printf("mGasPath=%s\n", mGasPath.c_str());
	//---------------------------------------------
	WiFiClientSecure client;
	client.setInsecure();
	if (!client.connect(mGasServer.c_str(), 443)){
		printf("connection failed\n");
	}

	char output[base64_enc_len(3)];
	String data = "filename=" + String( FILENAME ) +
                  String( "&mimetype=text/plain" ) +
                  String( "&folder=" ) + String( FOLDER ) +
                  String( "&data=" );

	printf("data = %s\n", data.c_str());

	data += "hogehoge\n";

	client.println("POST " + mGasPath + " HTTP/1.1");
	client.println("Host: " + mGasServer);
  client.println("Content-Length: " + String(data.length()));
	client.println("Connection: close");
	client.println("Content-Type: application/x-www-form-urlencoded");
	client.println();
	client.println(data);

/*
	client.println("POST " + mGasPath + " HTTP/1.1");
	client.println("Host: " + mGasServer);
	client.println("Transfer-Encoding: Chunked");
	client.println("Content-Type: application/x-www-form-urlencoded");
	client.println();
	client.println( String( data.length(), HEX ) );
	client.println(data);
	//---------------------------------------------
	String dataFile = "";
	int chunkSize = 3000;
	while(1){
		char mBuff[64];
		strcpy(mBuff, "hogehoge\n");
		int lenBuff = strlen(mBuff);

		char* p = mBuff;
		int n = lenBuff / 3;
		int m = lenBuff % 3;
    if ( m ) n++;
   	for (int i=0; i < n; i++) {
			if ( i == n - 1 && m ) base64_encode(output, p, m);
			else base64_encode(output, p, 3);
			dataFile += urlencode(String(output));
			p += 3;
		}
		printf("dataFile = %s / %s\n", dataFile.c_str(), String( dataFile.length(), HEX ));
		client.println( String( dataFile.length(), HEX ) );
		printf("done\n");
		client.println(dataFile);
		printf("done\n");
		dataFile = "";
	}
	if ( dataFile.length() > 0 ){
		printf("dataFile(last) = %s\n", dataFile.c_str());
		client.println( String( dataFile.length(), HEX ) );
		client.println(dataFile);
		dataFile = "";
	}
	client.println( "0" );
	client.println();
*/
	//---------------------------------------------
	Serial.println("Waiting for response.");
	int msecTimeout = 30 * 1000;
	unsigned long msecStart = millis();
	while (!client.available()) {
		printf(".\n");
		delay(100);
		if ( ( millis() - msecStart ) > msecTimeout ) {
			printf("No response.\n");
			break;
		}
	}

	String location = "";
	while(client.available()) {
		String line = client.readStringUntil('\r');
		Serial.println(line);
		if ( line.indexOf( "HTTP" ) >= 0 ){
			int p1 = line.indexOf( " ", 4 );
			int p2 = line.indexOf( " ", p1 + 1 );
			String resCode = line.substring( p1 + 1, p2 );
			printf("resCode=%d\n", resCode);
			if ( resCode != "302" ) break;
		}
		int p = line.indexOf( "Location:" );
		if ( p >= 0 ) {
			location = line.substring( p + 9 );
			location.trim();
			break;
		}
	}
	while(client.available()) {
		String line = client.readStringUntil('\r');
		printf("%s\n", line);
	}
	if ( location != "" ){
		client.print(String("GET ") + location + " HTTP/1.1\r\n" +
						"Host: " + String(mGasServer) + "\r\n" + 
						"User-Agent: M5F9P\r\n" + 
						"Connection: close\r\n\r\n");

		msecStart = millis();
		while (!client.available()) {
			printf(".\n");
			delay(100);
			if ( ( millis() - msecStart ) > msecTimeout ) {
				printf("No response.\n");
				break;
			}
		}
		while(client.available()) {
			String line = client.readStringUntil('\r');
			Serial.println(line);
			if ( line.indexOf( "HTTP" ) >= 0 ){
				int p1 = line.indexOf( " ", 4 );
				int p2 = line.indexOf( " ", p1 + 1 );
				String resCode = line.substring( p1 + 1, p2 );
				printf("resCode=%d\n", resCode);
			}
		}
	}
	client.stop();

	printf("done\n");
}

void loop() {
}
