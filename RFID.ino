#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// --- PIN DEFINITIONS ---
#define SS_PIN  5  
#define RST_PIN 4  
#define BUZZER_PIN 15 

// --- OBJECTS ---
MFRC522 mfrc522(SS_PIN, RST_PIN);   
LiquidCrystal_I2C lcd(0x27, 16, 2); 

void setup() {
  Serial.begin(115200); 
  SPI.begin();          
  mfrc522.PCD_Init();   
  
  // Initialize I2C for LCD (SDA=21, SCL=22)
  Wire.begin(21, 22); 
  
  pinMode(BUZZER_PIN, OUTPUT);
  
  // LCD Setup
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Scan Your Card");

  // Set timeout for serial reading to make it snappier
  Serial.setTimeout(2000); 
}

void loop() {
  // 1. Check for new card
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // 2. Read UID
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  String cardID = content.substring(1); // Remove leading space

  // 3. Send ID to Python
  // (Python is listening for this!)
  Serial.println(cardID); 
  
  // 4. Wait for Python to Reply
  lcd.clear();
  lcd.print("Checking...");
  
  unsigned long startTime = millis();
  
  // Wait up to 5 seconds for Python to respond
  while (Serial.available() == 0) {
    if (millis() - startTime > 5000) { 
       lcd.clear(); 
       lcd.print("PC Error"); 
       delay(1000); 
       lcd.clear(); 
       lcd.print("Scan Your Card"); 
       return; 
    }
  }

  // 5. Read the Name sent back by Python
  String name = Serial.readStringUntil('\n');
  name.trim(); // Remove any extra whitespace

  // 6. Display Logic
  if (name == "DENIED") {
    lcd.clear(); 
    lcd.print("Unknown User");
    // Long Error Beep
    digitalWrite(BUZZER_PIN, HIGH); delay(800); digitalWrite(BUZZER_PIN, LOW);
  } 
  else if (name == "CLOUD ERROR") {
    lcd.clear();
    lcd.print("WiFi Error");
    delay(1000);
  }
  else {
    lcd.clear();
    lcd.print("Welcome:");
    lcd.setCursor(0, 1);
    lcd.print(name);
    // Success Beeps (Two short beeps)
    digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
  }

  // 7. Reset for next user
  delay(500); 
  lcd.clear();
  lcd.print("Scan Your Card");
  
  // Stop reading the card so it doesn't trigger again immediately
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}