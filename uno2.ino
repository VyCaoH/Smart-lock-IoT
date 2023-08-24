// Include required libraries
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by unix time_t as ten ascii digits
#define TIME_HEADER  255   // Header tag for serial time sync message


// Create instances
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(10, 9); // MFRC522 mfrc522(SS_PIN, RST_PIN)
Servo sg90;

// Initialize Pins for led's, servo and buzzer
// Blue LED is connected to 5V

constexpr uint8_t servoPin = 5;
constexpr uint8_t buzzerPin = 8;
int buzzerState = LOW;

char initial_password[4] = {'1', '2', '3', '4'};  // Variable to store initial password
String tagUID = "64 A1 5E 24";  //
char password[4];   // Variable to store users password
boolean RFIDMode = true; // boolean to change modes
char key_pressed = 0; // Variable to store incoming keys
uint8_t i = 0;  // Variable used for counter
int wrongAttemptsRFID = 0; // Số lần sai
int wrongAttemptsPass = 0; // Số lần sai

// defining how many rows and columns our keypad have
const byte rows = 4;
const byte columns = 3;

// Keypad pin map
char hexaKeys[rows][columns] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// Initializing pins for keypad
byte row_pins[rows] = {A0, A1, A2, A3};
byte column_pins[columns] = {4, 3, 2};

// Create instance for keypad
Keypad keypad_key = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);


SoftwareSerial mySerial(0, 1); // RX, TX
int pir = 6;
int pirState = LOW;       //Bắt đầu không có báo động
int val = 0;

void setup() {
  // Arduino Pin configuration
  pinMode(buzzerPin, OUTPUT);


  sg90.attach(servoPin);  //Declare pin 8 for servo
  sg90.write(0); // Set initial position at 90 degrees

  lcd.init();   // LCD screen
  lcd.backlight();
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522

  lcd.clear(); // Clear LCD screen

  mySerial.begin(9600);
  Serial.begin(9600);

  pinMode(pir, INPUT);

}
void unlockbuzz()
{
  digitalWrite(buzzerPin, HIGH);
  delay(80);
  digitalWrite(buzzerPin, LOW);
  delay(80);
  digitalWrite(buzzerPin, HIGH);
  delay(80);
  digitalWrite(buzzerPin, LOW);
  delay(200);
  digitalWrite(buzzerPin, HIGH);
  delay(80);
  digitalWrite(buzzerPin, LOW);
  delay(80);
  digitalWrite(buzzerPin, HIGH);
  delay(80);
  digitalWrite(buzzerPin, LOW);
  delay(80);
}

void loop() {
  //motion detect
  val = digitalRead(pir);  // đọc giá trị ngõ vào
  if (val == HIGH)
  {            // kiểm tra ngõ vào có ở mức CAO không
    delay(150);
    if (pirState == LOW) 
    {
      Serial.println("Motion_detect");
      pirState = HIGH;
    }
  } 
  else 
  {
    delay(150);
    if (pirState == HIGH) 
    {
      Serial.println("Motion_not_detect");
      pirState = LOW;
    }
  }

  //read message from esp
  if (mySerial.available())
  {
    String recvMessage = mySerial.readStringUntil('\n');
    recvMessage.trim();
    int noti = recvMessage.toInt();
    //door open
    if (noti == 11)
    {
      sg90.write(180); // Door Opened
      Serial.println("Opened!");
      delay(5000);
      sg90.write(0); // Door Closed
      Serial.println("Closed");
    }
    //door close
    else if (noti == 10)
    {
      sg90.write(0); // Door Closed
      Serial.println("Closed");
    }
    //buzzer on
    else if (noti == 21)
    {
      buzzerState = HIGH; // Buzzer On
      Serial.println("ON");
    }
    //buzzer off
    else if (noti == 20)
    {
      buzzerState = LOW; // Buzzer Off
      Serial.println("OFF");
    }
  }

  // System will first look for mode
  if (RFIDMode == true) {
    lcd.setCursor(0, 0);
    lcd.print("   Door Lock");
    lcd.setCursor(0, 1);
    lcd.print(" Scan Your Tag ");

    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    //Reading from the card
    String tag = "";
    for (byte j = 0; j < mfrc522.uid.size; j++)
    {
      tag.concat(String(mfrc522.uid.uidByte[j] < 0x10 ? " 0" : " "));
      tag.concat(String(mfrc522.uid.uidByte[j], HEX));
    } 

    tag.toUpperCase();

    //Checking the card
    if (tag.substring(1) == tagUID)
    {
      // If UID of tag is matched.
      lcd.clear();
      lcd.print("Tag Matched");
      if (buzzerState == HIGH) unlockbuzz();
      else delay(1000);
      lcd.clear();
      lcd.print("Enter Password:");
      lcd.setCursor(0, 1);
      RFIDMode = false; // Make RFID mode false
    }

    else
    {
      // If UID of tag is not matched.
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wrong Tag Shown");
      wrongAttemptsRFID++;
      lcd.setCursor(0, 1);
      lcd.print("Access Denied");
      if (buzzerState == HIGH)
      {
      digitalWrite(buzzerPin, HIGH);
      delay(3000);
      digitalWrite(buzzerPin, LOW);
      }
      else {
        delay(3000);
      }
      lcd.clear();
    }
    if (wrongAttemptsRFID >= 3) {
    wrongAttemptsRFID = 0; // Đặt lại số lần sai sau khi gửi thông báo
    Serial.println("Failed 3 times");
    }
  }

  // If RFID mode is false, it will look for keys from keypad
  if (RFIDMode == false) {
    key_pressed = keypad_key.getKey(); // Storing keys
    if (key_pressed)
    {
      if (buzzerState == HIGH)
      {
      password[i++] = key_pressed; // Storing in password variable
      lcd.print("*");
      digitalWrite(buzzerPin, HIGH);
      delay(50);
      digitalWrite(buzzerPin, LOW);
      }
      else 
      {
        password[i++] = key_pressed; // Storing in password variable
        delay(50);
        lcd.print("*");
      }
    }
    if (i == 4) // If 4 keys are completed
    {
      delay(200);
      if (!(strncmp(password, initial_password, 4))) // If password is matched
      {
        lcd.clear();
        lcd.print("PASS ACCEPTED");
        delay(1000);
        if (buzzerState == HIGH) unlockbuzz();
        sg90.write(180); // Door Opened
        Serial.println("Opened!");
        lcd.clear();
        lcd.print("DOOR OPENED");
        delay(3000);
        i = 0;
        for (int count = 5; count > 0; count--) {
        lcd.clear();
        lcd.print("CLOSE DOOR IN...");
        lcd.setCursor(0, 1);
        lcd.print(count);
        if (buzzerState == HIGH)
        {
        digitalWrite(buzzerPin, HIGH);
        delay(80);
        digitalWrite(buzzerPin, LOW);
        delay(1000);
        }
        else {
          delay(1000);
        }
        }

      // Close the door and reset back to RFID mode
        sg90.write(0);
        Serial.println("Closed");
        RFIDMode = true;
        lcd.clear();
      }
      else    // If password is not matched
      {
        lcd.clear();
        lcd.print("Wrong Password");
        delay(1500);
        wrongAttemptsPass++;
        if (buzzerState == HIGH)
        {
        digitalWrite(buzzerPin, HIGH);
        delay(3000);
        digitalWrite(buzzerPin, LOW);
        }
        else {
           delay(3000);
        }
        lcd.clear();
        i = 0;
        RFIDMode = true;  // Make RFID mode true
      }
    }
    if (wrongAttemptsPass >= 3) {
    Serial.print("Failed 3 times\n");
    wrongAttemptsPass = 0; // Đặt lại số lần sai sau khi gửi thông báo
    }
  }
  

}