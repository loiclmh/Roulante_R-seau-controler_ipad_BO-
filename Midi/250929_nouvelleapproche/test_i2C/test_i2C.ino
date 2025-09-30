#include <Wire.h>

// Modifie ici selon ton câblage
#define SDA_PIN 4
#define SCL_PIN 5

void setup() {
	Serial.begin(115200);
	Wire.setSDA(SDA_PIN);
	Wire.setSCL(SCL_PIN);
	Wire.begin();
	delay(1000);
	Serial.println("Scanner I2C démarré...");
}

void loop() {
	byte count = 0;
	Serial.println("Scan...");
	for (byte addr = 1; addr < 127; addr++) {
		Wire.beginTransmission(addr);
		if (Wire.endTransmission() == 0) {
			Serial.print("Trouvé à l'adresse 0x");
			Serial.println(addr, HEX);
			count++;
		}
	}
	if (count == 0) Serial.println("Aucun périphérique I2C trouvé.");
	else Serial.print(count), Serial.println(" périphérique(s) trouvé(s).");
	delay(2000);
}
