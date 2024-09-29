#include <Servo.h>              // ไลบรารีสำหรับควบคุม Servo Motor เพื่อใช้ในการเปิด-ปิดประตู
#include <Keypad_I2C.h>         // ไลบรารีสำหรับการเชื่อมต่อและอ่านข้อมูลจาก Keypad ผ่าน I2C
#include <Wire.h>               // ไลบรารีสำหรับการสื่อสาร I2C ซึ่งใช้ในการเชื่อมต่อระหว่างไมโครคอนโทรลเลอร์กับอุปกรณ์อื่นๆ
#include <Keypad.h>             // ไลบรารีสำหรับการจัดการ Keypad เพื่อรับอินพุตจากผู้ใช้งาน
#include <LiquidCrystal_I2C.h>  // ไลบรารีสำหรับควบคุมจอแสดงผล LCD ผ่านการสื่อสารแบบ I2C
#include <SPI.h>                // ไลบรารีสำหรับการสื่อสารแบบ SPI ซึ่งใช้เชื่อมต่อกับโมดูล RFID
#include <MFRC522.h>            // ไลบรารีสำหรับควบคุมการอ่านข้อมูลจากบัตร RFID โดยใช้โมดูล MFRC522
#include <EEPROM.h>             // ไลบรารีสำหรับการอ่านและเขียนข้อมูลใน EEPROM (หน่วยความจำถาวร) เพื่อเก็บข้อมูลบัตร RFID ที่ลงทะเบียนแล้ว
#include <SoftwareSerial.h>     // ไลบรารีสำหรับการสื่อสารแบบอนุกรมเสมือนผ่านพอร์ต RX/TX เพื่อเชื่อมต่อกับอุปกรณ์อื่นๆ (เช่น โมดูลภายนอก)

// ข้อมูลการเชื่อมต่อ Keypad I2C
#define I2CADDR 0x20  // กำหนดที่อยู่ I2C ของ Keypad (0x20 คือที่อยู่ I2C ของอุปกรณ์นี้)
const byte ROWS = 4;  // กำหนดจำนวนแถวของ Keypad (Keypad มี 4 แถว)
const byte COLS = 4;  // กำหนดจำนวนคอลัมน์ของ Keypad (Keypad มี 4 คอลัมน์)

char hexaKeys[ROWS][COLS] = {
  // สร้างแมปของปุ่มกดแต่ละปุ่มใน Keypad
  { '1', '2', '3', 'A' },  // แถวที่ 1 มีปุ่ม 1, 2, 3, A
  { '4', '5', '6', 'B' },  // แถวที่ 2 มีปุ่ม 4, 5, 6, B
  { '7', '8', '9', 'C' },  // แถวที่ 3 มีปุ่ม 7, 8, 9, C
  { '*', '0', '#', 'D' }   // แถวที่ 4 มีปุ่ม *, 0, #, D
};

byte rowPins[ROWS] = { 7, 6, 5, 4 };  // กำหนดขาเชื่อมต่อของแถว (Row) ของ Keypad เข้ากับพิน 7, 6, 5, 4
byte colPins[COLS] = { 3, 2, 1, 0 };  // กำหนดขาเชื่อมต่อของคอลัมน์ (Column) ของ Keypad เข้ากับพิน 3, 2, 1, 0

Keypad_I2C keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);
// สร้างออบเจ็กต์ keypad สำหรับ Keypad I2C โดยใช้การแมปปุ่ม hexaKeys และการเชื่อมต่อผ่านพินที่กำหนดไว้
// พร้อมทั้งระบุจำนวนแถวและคอลัมน์ของ Keypad และที่อยู่ I2C (I2CADDR)

// ข้อมูลการเชื่อมต่อ RFID
#define SS_PIN 10             // กำหนดขา SS ของโมดูล RFID ให้เชื่อมต่อกับพิน 10
#define RST_PIN 5             // กำหนดขา Reset ของโมดูล RFID ให้เชื่อมต่อกับพิน 5
const int EEPROM_SIZE = 512;  // กำหนดขนาดของ EEPROM ที่ใช้เก็บข้อมูล UID การ์ด (512 ไบต์)
const int CARD_SIZE = 8;      // กำหนดขนาดของ UID การ์ด (8 ไบต์) ที่เก็บใน EEPROM

SoftwareSerial mySerial(8, 9);  // ใช้ SoftwareSerial สำหรับการสื่อสารกับอุปกรณ์อื่นๆ ผ่านขา Rx (พิน 8) และ Tx (พิน 9)

Servo myservo;                       // สร้างออบเจ็กต์ servo สำหรับควบคุมการหมุนของมอเตอร์เพื่อเปิด-ปิดประตู
LiquidCrystal_I2C lcd(0x27, 16, 2);  // สร้างออบเจ็กต์ lcd สำหรับควบคุมจอ LCD ขนาด 16x2 ที่มี address I2C เป็น 0x27
MFRC522 rfid(SS_PIN, RST_PIN);       // สร้างออบเจ็กต์ rfid สำหรับควบคุมโมดูล RFID โดยใช้ขา SS และ RST ที่กำหนด

String enteredCode = "";                  // สร้างตัวแปรเก็บรหัสที่ผู้ใช้ป้อนจาก Keypad
unsigned long lastActionTime = 0;         // ตัวแปรเก็บเวลาล่าสุดที่มีการกระทำ (ใช้ในการควบคุมการปิดประตูอัตโนมัติ)
bool motorMoving = false;                 // สถานะการเคลื่อนที่ของมอเตอร์ (ประตูเปิดหรือปิด)
bool emergencyDoorOpen = false;           // สถานะการเปิดประตูฉุกเฉิน
bool messageShown = false;                // ตัวแปรระบุว่าข้อความฉุกเฉินแสดงแล้วหรือยัง
double angle = 0;                         // ตัวแปรเก็บมุมหมุนของ Servo Motor
bool registrationMode = false;            // สถานะการลงทะเบียนการ์ดใหม่
bool deleteMode = false;                  // สถานะการลบการ์ดจากระบบ
unsigned long lastKeypadPressTime = 0;    // เก็บเวลาที่กดปุ่ม Keypad ครั้งล่าสุด
const unsigned long debounceDelay = 200;  // หน่วงเวลา 200 มิลลิวินาที เพื่อป้องกันการกดปุ่มซ้ำ (ป้องกันการรบกวนจากการกดปุ่มหลายครั้ง)


// ขาเชื่อมต่อ Ultrasonic Sensor
const int trigPin = A0;  // กำหนดขา Trig ของเซนเซอร์ Ultrasonic เชื่อมต่อกับพิน A0
const int echoPin = A1;  // กำหนดขา Echo ของเซนเซอร์ Ultrasonic เชื่อมต่อกับพิน A1

// กำหนด pin ใหม่
const int buzzerPin = 6;  // ย้ายการเชื่อมต่อ Buzzer ไปยังพิน 6
const int servoPin = 7;   // ย้ายการเชื่อมต่อ Servo Motor ไปยังพิน 7
const int switchPin = 4;  // ย้ายการเชื่อมต่อสวิตช์ไปยังพิน 4
const int ledPin = 3;     // กำหนดให้ LED เชื่อมต่อที่พิน 3

void setup() {
  Wire.begin();          // เริ่มต้นการสื่อสารผ่าน I2C สำหรับอุปกรณ์ที่เชื่อมต่อด้วย I2C (เช่น Keypad, LCD)
  keypad.begin();        // เริ่มต้นการทำงานของ Keypad I2C
  Serial.begin(9600);    // เริ่มต้นการสื่อสารอนุกรม (Serial) ด้วยบอดเรต 9600 สำหรับการดีบัก
  mySerial.begin(9600);  // เริ่มต้นการสื่อสารอนุกรมเสมือน (SoftwareSerial) ด้วยบอดเรต 9600 เพื่อสื่อสารกับอุปกรณ์ภายนอก (เช่น โค้ดหรือบอร์ดอื่น)

  lcd.init();       // เริ่มต้นการทำงานของจอ LCD
  lcd.backlight();  // เปิดไฟหลังจอ LCD

  myservo.attach(servoPin);  // เชื่อมต่อ Servo Motor กับพินที่กำหนดไว้ (servoPin)
  myservo.write(0);          // ตั้งค่า Servo Motor ให้อยู่ที่มุม 0 องศา (ประตูปิด)

  SPI.begin();      // เริ่มต้นการทำงานของ SPI เพื่อใช้ในการสื่อสารกับโมดูล RFID
  rfid.PCD_Init();  // เริ่มต้นการทำงานของโมดูล RFID (MFRC522)

  pinMode(buzzerPin, OUTPUT);        // กำหนดให้ขา buzzerPin ทำหน้าที่ส่งออก (OUTPUT) สำหรับการควบคุมเสียงเตือน
  pinMode(switchPin, INPUT_PULLUP);  // กำหนดให้ขา switchPin เป็นอินพุตพร้อมเปิดตัวต้านทานภายใน (INPUT_PULLUP) เพื่อใช้กับสวิตช์
  pinMode(ledPin, OUTPUT);           // กำหนดให้ขา ledPin ทำหน้าที่ส่งออก (OUTPUT) สำหรับควบคุม LED

  // ตั้งค่า Ultrasonic Sensor
  pinMode(trigPin, OUTPUT);  // กำหนดขา trigPin ของเซนเซอร์ Ultrasonic เป็นขาส่งออก (OUTPUT) สำหรับส่งสัญญาณเสียง
  pinMode(echoPin, INPUT);   // กำหนดขา echoPin ของเซนเซอร์ Ultrasonic เป็นขารับ (INPUT) สำหรับรับสัญญาณสะท้อนกลับ

  // ตั้งค่า LED เริ่มต้นเป็น "ปิด"
  digitalWrite(ledPin, LOW);  // ปิด LED โดยเริ่มต้นให้ขา ledPin เป็น LOW

  // แสดงสถานะระบบพร้อมทำงาน
  displaySystemReady();  // เรียกฟังก์ชันแสดงผลสถานะ "พร้อมทำงาน" บนจอ LCD
}

void loop() {
  char key = keypad.getKey();                // อ่านค่าจาก Keypad ว่ามีการกดปุ่มใดๆ หรือไม่
  unsigned long currentMillis = millis();    // เก็บเวลาปัจจุบัน (หน่วยเป็นมิลลิวินาที) เพื่อตรวจสอบการหน่วงเวลา
  int switchValue = digitalRead(switchPin);  // อ่านค่าจากสวิตช์เพื่อดูว่ามีการกดสวิตช์หรือไม่

  // ตรวจสอบการกดปุ่ม Keypad
  if (key) {      // ถ้าพบว่ามีการกดปุ่มใดๆ บน Keypad
    lcd.clear();  // ล้างจอ LCD เพื่อเตรียมแสดงข้อความใหม่

    if (registrationMode || deleteMode) {           // ตรวจสอบว่าอยู่ในโหมดลงทะเบียนหรือโหมดลบการ์ดหรือไม่
      if (key == 'D' && registrationMode) {         // ถ้ากดปุ่ม D และอยู่ในโหมดลงทะเบียน (ต้องการยกเลิกโหมด)
        Serial.println("Registration cancelled.");  // แสดงข้อความยกเลิกโหมดลงทะเบียนใน Serial Monitor
        lcd.clear();                                // ล้างจอ LCD
        lcd.setCursor(0, 0);                        // ตั้งค่าตำแหน่งเคอร์เซอร์ไปที่แถวที่ 0 คอลัมน์ที่ 0
        lcd.print("Registration");                  // แสดงข้อความ "Registration"
        lcd.setCursor(0, 1);                        // ตั้งค่าตำแหน่งเคอร์เซอร์ไปที่แถวที่ 1 คอลัมน์ที่ 0
        lcd.print("Cancelled!");                    // แสดงข้อความ "Cancelled!"
        tone(buzzerPin, 1000, 200);                 // ส่งเสียงเตือนที่ buzzer ที่ความถี่ 1000 Hz เป็นเวลา 200 มิลลิวินาที
        digitalWrite(ledPin, HIGH);                 // เปิด LED เพื่อแสดงสถานะ
        delay(300);                                 // รอ 300 มิลลิวินาที
        digitalWrite(ledPin, LOW);                  // ปิด LED
        delay(300);                                 // รอ 300 มิลลิวินาที
        tone(buzzerPin, 1000, 200);                 // ส่งเสียงเตือนครั้งที่ 2
        digitalWrite(ledPin, HIGH);                 // เปิด LED อีกครั้ง
        delay(300);                                 // รอ 300 มิลลิวินาที
        digitalWrite(ledPin, LOW);                  // ปิด LED
        delay(1000);                                // รอ 1000 มิลลิวินาที
        resetDisplayAfterDelay();                   // เรียกฟังก์ชัน resetDisplayAfterDelay() เพื่อแสดงข้อความพร้อมทำงานใหม่บนจอ LCD
        registrationMode = false;                   // ยกเลิกโหมดลงทะเบียน (ตั้งค่า registrationMode ให้เป็น false)
        return;                                     // จบการทำงานของฟังก์ชัน loop ในรอบนี้และรอรอบถัดไป
      }

      if (key == 'C') {                          // กดปุ่ม C เพื่อยกเลิกการทำงานไม่ว่ากำลังอยู่ในโหมดไหน
        Serial.println("Operation cancelled.");  // แสดงข้อความใน Serial Monitor ว่าการทำงานถูกยกเลิก
        lcd.clear();                             // ล้างหน้าจอ LCD
        lcd.setCursor(0, 0);                     // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
        lcd.print("Operation");                  // แสดงข้อความ "Operation" บนจอ LCD
        lcd.setCursor(0, 1);                     // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
        lcd.print("Cancelled!");                 // แสดงข้อความ "Cancelled!" บนจอ LCD
        tone(buzzerPin, 1000, 200);              // ส่งเสียงเตือนที่ความถี่ 1000 Hz เป็นเวลา 200 มิลลิวินาทีครั้งแรก
        digitalWrite(ledPin, HIGH);              // เปิด LED เพื่อแสดงสถานะ
        delay(300);                              // รอ 300 มิลลิวินาที
        digitalWrite(ledPin, LOW);               // ปิด LED
        delay(300);                              // รอ 300 มิลลิวินาที
        tone(buzzerPin, 1000, 200);              // ส่งเสียงเตือนครั้งที่สอง
        digitalWrite(ledPin, HIGH);              // เปิด LED อีกครั้ง
        delay(300);                              // รอ 300 มิลลิวินาที
        digitalWrite(ledPin, LOW);               // ปิด LED
        delay(1000);                             // รอ 1000 มิลลิวินาทีเพื่อให้เสียงและแสงแจ้งเตือนเสร็จสิ้น
        resetDisplayAfterDelay();                // เรียกฟังก์ชันรีเซ็ตจอ LCD หลังจากหน่วงเวลา
        registrationMode = false;                // ยกเลิกโหมดลงทะเบียน (ตั้งค่าให้เป็น false)
        deleteMode = false;                      // ยกเลิกโหมดลบการ์ด (ตั้งค่าให้เป็น false)
        return;                                  // ยุติการทำงานในฟังก์ชัน loop รอบนี้
      }

      // โค้ดสำหรับการกรอกรหัสในโหมดลงทะเบียนหรือลบการ์ด
      if (key == '#') {                                     // ถ้าผู้ใช้กดปุ่ม '#'
        if (enteredCode == "1234") {                        // ตรวจสอบว่ารหัสผ่านที่กรอกถูกต้องหรือไม่
          tone(buzzerPin, 1000, 200);                       // ส่งเสียงเตือนที่ Buzzer
          if (registrationMode) {                           // ถ้าอยู่ในโหมดลงทะเบียน
            Serial.println("Enter new card to register.");  // แสดงข้อความใน Serial Monitor
            lcd.setCursor(0, 0);                            // ตั้งเคอร์เซอร์ไปที่แถวที่ 0 คอลัมน์ที่ 0
            lcd.print("Scan new card");                     // แสดงข้อความบนจอ LCD
            lcd.setCursor(0, 1);                            // ตั้งเคอร์เซอร์ไปที่แถวที่ 1 คอลัมน์ที่ 0
            lcd.print("to register...");                    // แสดงข้อความเพิ่มเติมบนจอ LCD
            registerCard();                                 // เรียกฟังก์ชันลงทะเบียนการ์ดใหม่
          } else if (deleteMode) {                          // ถ้าอยู่ในโหมดลบการ์ด
            Serial.println("Enter card to delete.");        // แสดงข้อความใน Serial Monitor
            lcd.setCursor(0, 0);                            // ตั้งเคอร์เซอร์ไปที่แถวที่ 0 คอลัมน์ที่ 0
            lcd.print("Scan card to");                      // แสดงข้อความบนจอ LCD
            lcd.setCursor(0, 1);                            // ตั้งเคอร์เซอร์ไปที่แถวที่ 1 คอลัมน์ที่ 0
            lcd.print("delete...");                         // แสดงข้อความเพิ่มเติมบนจอ LCD
            deleteCard();                                   // เรียกฟังก์ชันเพื่อลบการ์ด
          }
          registrationMode = false;           // ยกเลิกโหมดลงทะเบียน
          deleteMode = false;                 // ยกเลิกโหมดลบการ์ด
        } else {                              // ถ้ารหัสผ่านไม่ถูกต้อง
          Serial.println("Incorrect code.");  // แสดงข้อความรหัสผิดใน Serial Monitor
          lcd.setCursor(0, 0);                // ตั้งเคอร์เซอร์ไปที่แถวที่ 0 คอลัมน์ที่ 0
          lcd.print("Incorrect code");        // แสดงข้อความบนจอ LCD
          lcd.setCursor(0, 1);                // ตั้งเคอร์เซอร์ไปที่แถวที่ 1 คอลัมน์ที่ 0
          lcd.print("Try again.");            // แสดงข้อความเพิ่มเติมบนจอ LCD
          tone(buzzerPin, 1000, 200);         // ส่งเสียงเตือนครั้งที่ 1
          digitalWrite(ledPin, HIGH);         // เปิด LED
          delay(300);                         // รอ 300 มิลลิวินาที
          digitalWrite(ledPin, LOW);          // ปิด LED
          delay(300);                         // รอ 300 มิลลิวินาที
          tone(buzzerPin, 1000, 200);         // ส่งเสียงเตือนครั้งที่ 2
          digitalWrite(ledPin, HIGH);         // เปิด LED อีกครั้ง
          delay(300);                         // รอ 300 มิลลิวินาที
          digitalWrite(ledPin, LOW);          // ปิด LED
          delay(1000);                        // รอ 1000 มิลลิวินาที
          resetDisplayAfterDelay();           // รีเซ็ตหน้าจอ LCD
          registrationMode = false;           // ยกเลิกโหมดลงทะเบียน
          deleteMode = false;                 // ยกเลิกโหมดลบการ์ด
        }
        enteredCode = "";                                // รีเซ็ตค่า enteredCode เป็นค่าว่าง
      } else if (key == '*') {                           // ถ้าผู้ใช้กดปุ่ม '*'
        tone(buzzerPin, 1000, 200);                      // ส่งเสียงเตือนที่ Buzzer
        if (enteredCode.length() > 0) {                  // ถ้ามีการกรอกรหัสอยู่
          enteredCode.remove(enteredCode.length() - 1);  // ลบตัวอักษรสุดท้ายออกจาก enteredCode
          showEnteredCode();                             // แสดงรหัสที่กรอกไว้บนจอ LCD
        }
      } else {               // ถ้าผู้ใช้กดปุ่มอื่นๆ
        enteredCode += key;  // เพิ่มตัวอักษรที่กดลงใน enteredCode
        showEnteredCode();   // แสดงรหัสที่กรอกไว้บนจอ LCD
      }
    } else {                                                     // โหมดปกติ
      if (key == 'D') {                                          // ถ้าผู้ใช้กดปุ่ม 'D' เพื่อเข้าสู่โหมดลงทะเบียนการ์ดใหม่
        if (registrationMode) {                                  // ตรวจสอบว่าอยู่ในโหมดลงทะเบียนอยู่แล้วหรือไม่
          Serial.println("Registration cancelled.");             // แสดงข้อความยกเลิกการลงทะเบียนใน Serial Monitor
          lcd.clear();                                           // ล้างจอ LCD
          lcd.setCursor(0, 0);                                   // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
          lcd.print("Registration");                             // แสดงข้อความ "Registration" บนจอ LCD
          lcd.setCursor(0, 1);                                   // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
          lcd.print("Cancelled!");                               // แสดงข้อความ "Cancelled!" บนจอ LCD
          tone(buzzerPin, 1000, 200);                            // ส่งเสียงเตือนครั้งที่ 1
          digitalWrite(ledPin, HIGH);                            // เปิด LED เพื่อแสดงสถานะ
          delay(300);                                            // รอ 300 มิลลิวินาที
          digitalWrite(ledPin, LOW);                             // ปิด LED
          delay(300);                                            // รอ 300 มิลลิวินาที
          tone(buzzerPin, 1000, 200);                            // ส่งเสียงเตือนครั้งที่ 2
          digitalWrite(ledPin, HIGH);                            // เปิด LED อีกครั้ง
          delay(300);                                            // รอ 300 มิลลิวินาที
          digitalWrite(ledPin, LOW);                             // ปิด LED
          delay(1000);                                           // รอ 1000 มิลลิวินาที
          resetDisplayAfterDelay();                              // เรียกฟังก์ชันรีเซ็ตจอ LCD หลังจากหน่วงเวลา
          registrationMode = false;                              // ยกเลิกโหมดลงทะเบียน
        } else {                                                 // ถ้าไม่อยู่ในโหมดลงทะเบียน
          tone(buzzerPin, 1000, 200);                            // ส่งเสียงเตือนครั้งที่ 1
          Serial.println("Entering card registration mode...");  // แสดงข้อความเข้าสู่โหมดลงทะเบียนใน Serial Monitor
          lcd.setCursor(0, 0);                                   // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
          lcd.print("Enter admin code");                         // แสดงข้อความ "Enter admin code" บนจอ LCD
          lcd.setCursor(0, 1);                                   // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
          lcd.print("for registration");                         // แสดงข้อความ "for registration" บนจอ LCD
          registrationMode = true;                               // ตั้งค่าให้เข้าสู่โหมดลงทะเบียน
          enteredCode = "";                                      // รีเซ็ตรหัสที่ป้อน
        }
      } else if (key == 'C') {  // ถ้าผู้ใช้กดปุ่ม 'C' เพื่อเข้าสู่โหมดลบการ์ด
        if (deleteMode) {  // ถ้าอยู่ในโหมดลบการ์ดอยู่แล้ว
          Serial.println("Delete mode cancelled.");  // แสดงข้อความยกเลิกโหมดลบใน Serial Monitor
          lcd.clear();  // ล้างหน้าจอ LCD
          lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
          lcd.print("Delete mode");  // แสดงข้อความ "Delete mode" บนจอ LCD
          lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
          lcd.print("Cancelled!");  // แสดงข้อความ "Cancelled!" บนจอ LCD
          tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
          digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะ
          delay(300);  // รอ 300 มิลลิวินาที
          digitalWrite(ledPin, LOW);   // ปิด LED
          delay(300);  // รอ 300 มิลลิวินาที
          tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
          digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
          delay(300);  // รอ 300 มิลลิวินาที
          digitalWrite(ledPin, LOW);   // ปิด LED
          delay(1000);  // รอ 1000 มิลลิวินาที
          resetDisplayAfterDelay();  // รีเซ็ตหน้าจอ LCD เพื่อแสดงสถานะพร้อมทำงาน
          deleteMode = false; // ยกเลิกโหมดลบการ์ด
        } else {  // ถ้าไม่อยู่ในโหมดลบการ์ด
          tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
          Serial.println("Entering card delete mode...");  // แสดงข้อความเข้าสู่โหมดลบการ์ดใน Serial Monitor
          lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
          lcd.print("Enter admin code");  // แสดงข้อความ "Enter admin code" บนจอ LCD
          lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
          lcd.print("to delete card");  // แสดงข้อความ "to delete card" บนจอ LCD
          deleteMode = true;  // ตั้งค่าให้เข้าสู่โหมดลบการ์ด
          enteredCode = "";  // รีเซ็ตรหัสที่ป้อน
        }
      } else if (key == '#') {  // ถ้าผู้ใช้กดปุ่ม '#'
        if (enteredCode == "1234") {  // ตรวจสอบว่ารหัสที่กรอกถูกต้องหรือไม่
          Serial.println("Password correct! Welcome...");  // แสดงข้อความยินดีใน Serial Monitor
          lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
          lcd.print("Password correct!");  // แสดงข้อความ "Password correct!" บนจอ LCD
          lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
          lcd.print("Welcome...");  // แสดงข้อความ "Welcome..." บนจอ LCD
          activateMotor();  // เรียกฟังก์ชันเพื่อเปิดประตู
        } else {  // ถ้ารหัสไม่ถูกต้อง
          Serial.println("Incorrect code.");  // แสดงข้อความรหัสผิดใน Serial Monitor
          lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
          lcd.print("Incorrect code");  // แสดงข้อความ "Incorrect code" บนจอ LCD
          // ส่งเสียงเตือนเมื่อใส่รหัสผิด
          tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
          digitalWrite(ledPin, HIGH);  // เปิด LED
          delay(300);  // รอ 300 มิลลิวินาที
          digitalWrite(ledPin, LOW);   // ปิด LED
          delay(300);  // รอ 300 มิลลิวินาที
          tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
          digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
          delay(300);  // รอ 300 มิลลิวินาที
          digitalWrite(ledPin, LOW);   // ปิด LED
          delay(2000);  // รอ 2000 มิลลิวินาที
          resetDisplayAfterDelay();  // รีเซ็ตหน้าจอ LCD หลังจากหน่วงเวลา
        }
        enteredCode = "";  // รีเซ็ตรหัสที่ป้อนเป็นค่าว่าง
      } else if (key == '*') {  // ถ้าผู้ใช้กดปุ่ม '*'
        tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือน
        if (enteredCode.length() > 0) {  // ถ้ามีการกรอกรหัสอยู่
          enteredCode.remove(enteredCode.length() - 1);  // ลบตัวอักษรสุดท้ายออกจาก enteredCode
          showEnteredCode();  // แสดงรหัสที่กรอกไว้บนจอ LCD
        }
      } else {  // ถ้าผู้ใช้กดปุ่มอื่นๆ
        enteredCode += key;  // เพิ่มตัวอักษรที่กดลงใน enteredCode
        showEnteredCode();  // แสดงรหัสที่กรอกไว้บนจอ LCD
      }
    }
  }

  // ตรวจสอบการกดปุ่มสวิตช์
  if (switchValue == LOW && !motorMoving) {
    Serial.println("Switch pressed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Switch pressed!");
    activateMotor();
  }

  // ตรวจสอบสถานะของมอเตอร์
  if (motorMoving) {  // ถ้าหากมอเตอร์กำลังทำงาน (ประตูเปิดอยู่)
    // ตรวจจับระยะทาง Ultrasonic
    long duration, distance;  // ประกาศตัวแปรสำหรับเก็บเวลาที่ใช้ในการส่งและรับสัญญาณ
    digitalWrite(trigPin, LOW);  // ตั้งขา Trig ให้เป็น LOW
    delayMicroseconds(2);  // รอ 2 ไมโครวินาที
    digitalWrite(trigPin, HIGH);  // ตั้งขา Trig ให้เป็น HIGH
    delayMicroseconds(10);  // รอ 10 ไมโครวินาที
    digitalWrite(trigPin, LOW);  // ตั้งขา Trig กลับไปเป็น LOW
    duration = pulseIn(echoPin, HIGH);  // อ่านเวลาที่สัญญาณ Echo กลับมาเป็น HIGH
    distance = duration * 0.034 / 2;  // คำนวณระยะทางในเซนติเมตร (ความเร็วเสียงประมาณ 0.034 เซนติเมตรต่อไมโครวินาที)

    Serial.print("Distance: ");  // แสดงข้อความระยะทางที่วัดได้
    Serial.print(distance);  // แสดงค่าระยะทาง
    Serial.println(" cm");  // แสดงหน่วยระยะทาง

    if (distance > 10) {  // ถ้าระยะทางมากกว่า 10 เซนติเมตร (ปรับค่าได้ตามต้องการ)
      Serial.println("No person detected.");  // แสดงข้อความว่าไม่มีคนอยู่ใกล้
      if (millis() - lastActionTime >= 2000) {  // ตรวจสอบเวลาที่ผ่านไปตั้งแต่มีการกระทำล่าสุด (รอ 2 วินาที)
        angle = 0;  // ตั้งค่าให้ Servo หมุนไปที่มุม 0 องศา (ปิดประตู)
        myservo.write(angle);  // ส่งคำสั่งให้ Servo หมุนไปยังมุมที่กำหนด
        Serial.println("Door Close!");  // แสดงข้อความ "Door Close!" ใน Serial Monitor
        lcd.clear();  // ล้างหน้าจอ LCD
        lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
        lcd.print("Door Close!");  // แสดงข้อความ "Door Close!" บนจอ LCD
        tone(buzzerPin, 1000, 500);  // ส่งเสียงเตือนที่ Buzzer
        motorMoving = false;  // เปลี่ยนสถานะมอเตอร์ไม่ทำงาน (ประตูปิด)

        // รอ 2 วินาที จากนั้นปิด LED และแสดงข้อความพร้อมทำงาน
        delay(500);  // รอ 500 มิลลิวินาที
        digitalWrite(ledPin, LOW);  // ปิด LED หลังจากเวลาผ่านไป 2 วินาที
        displaySystemReady();  // แสดงสถานะระบบพร้อมทำงาน
      }
    } else {  // ถ้ามีคนอยู่ในระยะใกล้
      Serial.println("Person detected.");  // แสดงข้อความว่าพบคน
      lastActionTime = millis();  // รีเซ็ตเวลารอถ้ามีคนอยู่ใกล้ประตู
    }
  }

  // ตรวจสอบการ์ด RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {  // ถ้ามีการ์ดใหม่เข้ามาและอ่านข้อมูลการ์ดได้
    lcd.clear();  // ล้างหน้าจอ LCD
    lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
    lcd.print("Scanning");  // แสดงข้อความ "Scanning" บนจอ LCD
    Serial.print("NUID tag is :");  // แสดงข้อความเริ่มต้นใน Serial Monitor
    String ID = "";  // ประกาศตัวแปรเพื่อเก็บ ID ของการ์ด
    for (byte i = 0; i < rfid.uid.size; i++) {  // วนลูปเพื่ออ่าน UID ของการ์ด
      lcd.print(".");  // แสดงจุดบนจอ LCD เพื่อแสดงการทำงาน
      ID.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));  // เพิ่มศูนย์หน้าถ้าค่าต่ำกว่า 16 (0x10)
      ID.concat(String(rfid.uid.uidByte[i], HEX));  // แปลงค่า UID เป็นเลขฐาน 16 และเพิ่มเข้าไปใน ID
      delay(300);  // หน่วงเวลา 300 มิลลิวินาทีเพื่อให้การแสดงผลชัดเจน
    }
    ID.toUpperCase();  // เปลี่ยน ID ให้เป็นตัวพิมพ์ใหญ่

    // ลบช่องว่างทั้งหมดออกเพื่อให้แน่ใจว่าการเปรียบเทียบเป็นไปอย่างถูกต้อง
    ID.replace(" ", "");  // ลบช่องว่างใน ID
    Serial.println(ID);  // แสดง ID ของการ์ดใน Serial Monitor

    if (isCardRegistered(ID)) {  // ตรวจสอบว่าการ์ด RFID ได้รับอนุญาตแล้วหรือไม่
      activateMotor();  // ถ้าการ์ดถูกลงทะเบียน ให้เรียกฟังก์ชัน activateMotor() เพื่อเปิดประตู
    } else {  // ถ้าการ์ดไม่ถูกลงทะเบียน
      lcd.clear();  // ล้างหน้าจอ LCD
      lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
      lcd.print("Wrong card!");  // แสดงข้อความ "Wrong card!" บนจอ LCD
      tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
      digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะ
      delay(300);  // รอ 300 มิลลิวินาที
      digitalWrite(ledPin, LOW);  // ปิด LED
      delay(300);  // รอ 300 มิลลิวินาที
      tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
      digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
      delay(300);  // รอ 300 มิลลิวินาที
      digitalWrite(ledPin, LOW);  // ปิด LED
      delay(1500);  // รอ 1500 มิลลิวินาที
      resetDisplayAfterDelay();  // รีเซ็ตหน้าจอ LCD เพื่อแสดงสถานะพร้อมทำงาน
    }
  }
  if (mySerial.available()) {  // ตรวจสอบว่ามีข้อมูลในพอร์ตอนุกรมเสมือนหรือไม่
    String message = mySerial.readString();  // อ่านข้อมูลที่ส่งเข้ามาเป็นข้อความ
    if (message.indexOf("OPEN_DOOR") >= 0 && !motorMoving) {  // ถ้าข้อความมีคำว่า "OPEN_DOOR" และมอเตอร์ไม่กำลังทำงาน
      activateMotor();  // เรียกฟังก์ชันเปิดประตู
    }
    if (message.indexOf("EMERGENCY_OPEN") >= 0 && !motorMoving) {  // ถ้าข้อความมีคำว่า "EMERGENCY_OPEN" และมอเตอร์ไม่กำลังทำงาน
      openDoorEmergency();       // เรียกฟังก์ชันเปิดประตูในโหมดฉุกเฉิน
      emergencyDoorOpen = true;  // ตั้งสถานะว่าอยู่ในโหมดฉุกเฉิน
    }
  }

  if (emergencyDoorOpen && !messageShown) {  // ถ้าอยู่ในโหมดฉุกเฉินและยังไม่แสดงข้อความ
    // แสดงข้อความ Emergency door open บน LCD เพียงครั้งเดียว
    lcd.clear();  // ล้างหน้าจอ LCD
    lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
    lcd.print("Emergency door");  // แสดงข้อความ "Emergency door" บนจอ LCD
    lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
    lcd.print("open!");  // แสดงข้อความ "open!" บนจอ LCD
    digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะฉุกเฉิน
    messageShown = true;         // ระบุว่าข้อความแสดงแล้ว
  }
  if (emergencyDoorOpen) {  // ถ้ายังอยู่ในโหมดฉุกเฉิน
    // ตรวจสอบการกดปุ่ม 'A' เพื่อปิดประตู
    if (key == 'A') {  // ถ้าผู้ใช้กดปุ่ม 'A'
      lcd.clear();  // ล้างหน้าจอ LCD
      lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
      lcd.print("Closing door...");  // แสดงข้อความ "Closing door..."
      delay(2000);  // รอ 2 วินาที
      closeDoor();  // เรียกฟังก์ชันปิดประตู
      digitalWrite(ledPin, LOW);  // ปิด LED

      // รีเซ็ตสถานะ
      emergencyDoorOpen = false;  // ยกเลิกสถานะโหมดฉุกเฉิน
      messageShown = false;             // พร้อมแสดงข้อความฉุกเฉินในรอบถัดไป
      mySerial.println("DOOR_CLOSED");  // ส่งสัญญาณไปยังบอร์ด 2 ว่าประตูปิดแล้ว
    }
  }
  delay(100);  // หน่วงเวลา 100 มิลลิวินาทีเพื่อไม่ให้ระบบทำงานเร็วเกินไป
}

void activateMotor() {  // ฟังก์ชันสำหรับเปิดประตูด้วยมอเตอร์
  angle = 90;  // ตั้งมุมของ Servo Motor ไปที่ 90 องศา (เปิดประตู)
  myservo.write(angle);  // หมุน Servo Motor ไปยังมุมที่กำหนด
  Serial.println("Door Open!");  // แสดงข้อความ "Door Open!" บน Serial Monitor
  lcd.clear();  // ล้างหน้าจอ LCD
  lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
  lcd.print("Door Open!");  // แสดงข้อความ "Door Open!" บนจอ LCD
  tone(buzzerPin, 1000, 500);  // ส่งเสียงเตือนที่ Buzzer ด้วยความถี่ 1000 Hz เป็นเวลา 500 มิลลิวินาที
  digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะว่ามอเตอร์กำลังทำงาน (ประตูเปิด)
  lastActionTime = millis();  // บันทึกเวลาปัจจุบันเพื่อติดตามเวลาที่เปิดประตู
  motorMoving = true;  // ตั้งสถานะว่า Servo Motor กำลังทำงาน (ประตูเปิด)
}

void openDoorEmergency() {  // ฟังก์ชันสำหรับเปิดประตูในกรณีฉุกเฉิน
  myservo.write(90);  // หมุน Servo Motor ไปที่มุม 90 องศา (เปิดประตู)
  while (motorMoving) {  // วนลูปขณะที่มอเตอร์กำลังทำงาน (ประตูเปิดอยู่)
    tone(buzzerPin, 1000);  // ส่งเสียงเตือนที่ Buzzer ด้วยความถี่ 1000 Hz อย่างต่อเนื่อง
    delay(100);  // รอ 100 มิลลิวินาที
    noTone(buzzerPin);  // หยุดเสียงเตือนชั่วคราว
    delay(100);  // รออีก 100 มิลลิวินาที
  }
}

void closeDoor() {  // ฟังก์ชันสำหรับปิดประตู
  myservo.write(0);  // หมุน Servo Motor ไปที่มุม 0 องศา (ปิดประตู)
  lcd.clear();  // ล้างหน้าจอ LCD
  lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
  lcd.print("Door closed!");  // แสดงข้อความ "Door closed!" บนจอ LCD
  tone(buzzerPin, 1000, 500);  // ส่งเสียงเตือนที่ Buzzer ด้วยความถี่ 1000 Hz เป็นเวลา 500 มิลลิวินาที
  delay(2000);  // รอ 2 วินาที
  displaySystemReady();  // เรียกฟังก์ชันแสดงข้อความว่าระบบพร้อมทำงานบนจอ LCD
}

void showEnteredCode() {  // ฟังก์ชันสำหรับแสดงรหัสผ่านที่ผู้ใช้กรอก
  Serial.print("Password: ");  // แสดงข้อความ "Password: " ใน Serial Monitor
  lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0 บนจอ LCD
  lcd.print("Password: ");  // แสดงข้อความ "Password: " บนจอ LCD
  for (int i = 0; i < enteredCode.length(); i++) {  // วนลูปตามความยาวของรหัสที่กรอก
    Serial.print('*');  // แสดงเครื่องหมาย '*' ใน Serial Monitor แทนการแสดงรหัสจริง
    lcd.print('*');  // แสดงเครื่องหมาย '*' บนจอ LCD แทนการแสดงรหัสจริง
  }
  Serial.println();  // ขึ้นบรรทัดใหม่ใน Serial Monitor
}

void registerCard() {  // ฟังก์ชันสำหรับลงทะเบียนการ์ดใหม่
  String newUID = "";  // ประกาศตัวแปรสำหรับเก็บ UID ของการ์ดใหม่
  lcd.clear();  // ล้างหน้าจอ LCD
  lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
  lcd.print("Scan new card");  // แสดงข้อความ "Scan new card" บนจอ LCD

  // รอการสแกนการ์ดใหม่
  while (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {  // ถ้ายังไม่มีการสแกนการ์ดใหม่
    char key = keypad.getKey();  // ตรวจสอบการกดปุ่มจาก Keypad
    if (key == 'D') {  // ถ้าผู้ใช้กดปุ่ม 'D' เพื่อยกเลิก
      Serial.println("Registration cancelled.");  // แสดงข้อความ "Registration cancelled." ใน Serial Monitor
      lcd.clear();  // ล้างหน้าจอ LCD
      lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
      lcd.print("Registration");  // แสดงข้อความ "Registration" บนจอ LCD
      lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
      lcd.print("Cancelled!");  // แสดงข้อความ "Cancelled!" บนจอ LCD
      tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
      digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะ
      delay(300);  // รอ 300 มิลลิวินาที
      digitalWrite(ledPin, LOW);  // ปิด LED
      delay(300);  // รอ 300 มิลลิวินาที
      tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
      digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
      delay(300);  // รอ 300 มิลลิวินาที
      digitalWrite(ledPin, LOW);  // ปิด LED
      delay(1000);  // รอ 1000 มิลลิวินาที
      resetDisplayAfterDelay();  // รีเซ็ตหน้าจอ LCD ให้แสดงสถานะพร้อมทำงาน
      return;  // ยกเลิกการลงทะเบียนและออกจากฟังก์ชัน
    }
    delay(100);  // หน่วงเวลาสำหรับการตรวจสอบครั้งต่อไป
  }

  // อ่าน UID ของการ์ดใหม่
  for (byte i = 0; i < rfid.uid.size; i++) {  // วนลูปตามขนาดของ UID การ์ด
    newUID.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));  // ถ้าค่าต่ำกว่า 16 (0x10) ให้เพิ่ม "0" ข้างหน้า
    newUID.concat(String(rfid.uid.uidByte[i], HEX));  // แปลงค่าแต่ละไบต์ของ UID เป็นเลขฐาน 16 และรวมเข้าไปใน newUID
  }
  newUID.toUpperCase();  // แปลงตัวอักษรใน newUID เป็นตัวพิมพ์ใหญ่
  newUID.replace(" ", "");  // ลบช่องว่างทั้งหมดออกจาก newUID

  if (storeCardInEEPROM(newUID)) {  // ตรวจสอบว่าการ์ดใหม่ถูกบันทึกใน EEPROM สำเร็จหรือไม่
    Serial.print("New UID Registered: ");  // แสดงข้อความใน Serial Monitor
    Serial.println(newUID);  // แสดง UID ของการ์ดที่ลงทะเบียนสำเร็จ
    lcd.clear();  // ล้างหน้าจอ LCD
    lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
    lcd.print("Card Registered!");  // แสดงข้อความ "Card Registered!" บนจอ LCD
    tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
    digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะ
    delay(300);  // รอ 300 มิลลิวินาที
    digitalWrite(ledPin, LOW);  // ปิด LED
    delay(300);  // รอ 300 มิลลิวินาที
    tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
    digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
    delay(300);  // รอ 300 มิลลิวินาที
    digitalWrite(ledPin, LOW);  // ปิด LED
  } else {  // ถ้าการบันทึกการ์ดใน EEPROM ล้มเหลว
    lcd.clear();  // ล้างหน้าจอ LCD
    lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
    lcd.print("Failed to Register");  // แสดงข้อความ "Failed to Register" บนจอ LCD
    lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
    lcd.print("Card Full");  // แสดงข้อความ "Card Full" บนจอ LCD
    tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
    digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะ
    delay(300);  // รอ 300 มิลลิวินาที
    digitalWrite(ledPin, LOW);  // ปิด LED
    delay(300);  // รอ 300 มิลลิวินาที
    tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
    digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
    delay(300);  // รอ 300 มิลลิวินาที
    digitalWrite(ledPin, LOW);  // ปิด LED
  }
  delay(2000);  // รอ 2 วินาที
  displaySystemReady();  // แสดงสถานะว่าระบบพร้อมทำงาน
}


void deleteCard() {  // ฟังก์ชันสำหรับลบการ์ดที่ลงทะเบียนในระบบ
  String cardToDelete = "";  // ประกาศตัวแปรสำหรับเก็บ UID ของการ์ดที่จะลบ
  lcd.clear();  // ล้างหน้าจอ LCD
  lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
  lcd.print("Scan to delete");  // แสดงข้อความ "Scan to delete" บนจอ LCD

  // รอการสแกนการ์ดเพื่อทำการลบ
  while (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {  // ตรวจสอบว่ามีการ์ดใหม่ถูกสแกนหรือไม่
    char key = keypad.getKey();  // ตรวจสอบการกดปุ่มจาก Keypad
    if (key == 'C') {  // ถ้าผู้ใช้กดปุ่ม 'C' เพื่อยกเลิกการลบ
      Serial.println("Delete operation cancelled.");  // แสดงข้อความ "Delete operation cancelled." ใน Serial Monitor
      lcd.clear();  // ล้างหน้าจอ LCD
      lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
      lcd.print("Delete");  // แสดงข้อความ "Delete" บนจอ LCD
      lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
      lcd.print("Cancelled!");  // แสดงข้อความ "Cancelled!" บนจอ LCD
      tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
      digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะ
      delay(300);  // รอ 300 มิลลิวินาที
      digitalWrite(ledPin, LOW);  // ปิด LED
      delay(300);  // รอ 300 มิลลิวินาที
      tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
      digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
      delay(300);  // รอ 300 มิลลิวินาที
      digitalWrite(ledPin, LOW);  // ปิด LED
      delay(1000);  // รอ 1000 มิลลิวินาที
      resetDisplayAfterDelay();  // รีเซ็ตหน้าจอ LCD ให้แสดงสถานะพร้อมทำงาน
      return;  // ยกเลิกการลบการ์ดและออกจากฟังก์ชัน
    }
    delay(100);  // หน่วงเวลาสำหรับการตรวจสอบครั้งถัดไป
  }

  // อ่าน UID ของการ์ดที่จะลบ
  for (byte i = 0; i < rfid.uid.size; i++) {  // วนลูปตามขนาดของ UID การ์ด
    cardToDelete.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));  // ถ้าค่า UID ต่ำกว่า 16 (0x10) ให้เพิ่ม "0" ข้างหน้า
    cardToDelete.concat(String(rfid.uid.uidByte[i], HEX));  // แปลงค่าแต่ละไบต์ของ UID เป็นเลขฐาน 16 และรวมเข้าไปใน cardToDelete
  }
  cardToDelete.toUpperCase();  // แปลงตัวอักษรใน cardToDelete เป็นตัวพิมพ์ใหญ่
  cardToDelete.replace(" ", "");  // ลบช่องว่างทั้งหมดออกจาก cardToDelete

  if (removeCardFromEEPROM(cardToDelete)) {  // ตรวจสอบว่าการลบการ์ดจาก EEPROM สำเร็จหรือไม่
    Serial.println("Card Deleted");  // แสดงข้อความ "Card Deleted" ใน Serial Monitor
    lcd.clear();  // ล้างหน้าจอ LCD
    lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
    lcd.print("Card Deleted!");  // แสดงข้อความ "Card Deleted!" บนจอ LCD
    tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
    digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะ
    delay(300);  // รอ 300 มิลลิวินาที
    digitalWrite(ledPin, LOW);  // ปิด LED
    delay(300);  // รอ 300 มิลลิวินาที
    tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
    digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
    delay(300);  // รอ 300 มิลลิวินาที
    digitalWrite(ledPin, LOW);  // ปิด LED
  } else {  // ถ้าการ์ดไม่ถูกพบใน EEPROM
    Serial.println("Card not found");  // แสดงข้อความ "Card not found" ใน Serial Monitor
    lcd.clear();  // ล้างหน้าจอ LCD
    lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
    lcd.print("Card not found!");  // แสดงข้อความ "Card not found!" บนจอ LCD
    tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 1
    digitalWrite(ledPin, HIGH);  // เปิด LED เพื่อแสดงสถานะ
    delay(300);  // รอ 300 มิลลิวินาที
    digitalWrite(ledPin, LOW);  // ปิด LED
    delay(300);  // รอ 300 มิลลิวินาที
    tone(buzzerPin, 1000, 200);  // ส่งเสียงเตือนครั้งที่ 2
    digitalWrite(ledPin, HIGH);  // เปิด LED อีกครั้ง
    delay(300);  // รอ 300 มิลลิวินาที
    digitalWrite(ledPin, LOW);  // ปิด LED
  }
  delay(2000);  // รอ 2 วินาที
  displaySystemReady();  // แสดงสถานะว่าระบบพร้อมทำงาน
}

bool isCardRegistered(String card) {  // ฟังก์ชันสำหรับตรวจสอบว่าการ์ดที่กำหนดถูกลงทะเบียนใน EEPROM หรือไม่
  for (int i = 0; i < EEPROM_SIZE; i += CARD_SIZE) {  // วนลูปตรวจสอบทุกการ์ดที่ถูกเก็บใน EEPROM ทีละบล็อกตามขนาดของการ์ด (CARD_SIZE)
    String storedCard = "";  // สร้างตัวแปรเก็บการ์ดที่อ่านได้จาก EEPROM
    for (int j = 0; j < CARD_SIZE; j++) {  // วนลูปอ่านข้อมูลของการ์ดแต่ละไบต์จาก EEPROM
      storedCard += char(EEPROM.read(i + j));  // อ่านค่าแต่ละไบต์และเพิ่มเข้าไปในตัวแปร storedCard
    }
    if (storedCard == card) {  // ถ้าการ์ดที่อ่านได้ตรงกับการ์ดที่กำหนด
      return true;  // คืนค่า true แสดงว่าการ์ดนี้ถูกลงทะเบียนแล้ว
    }
  }
  return false;  // ถ้าไม่พบการ์ดที่ตรงกัน คืนค่า false แสดงว่าการ์ดนี้ยังไม่ถูกลงทะเบียน
}

bool storeCardInEEPROM(String card) {  // ฟังก์ชันสำหรับบันทึกการ์ดใหม่ลงใน EEPROM
  if (!isCardRegistered(card)) {  // ตรวจสอบว่าการ์ดนี้ยังไม่ได้ลงทะเบียนในระบบ
    for (int i = 0; i < EEPROM_SIZE; i += CARD_SIZE) {  // วนลูปผ่าน EEPROM ทีละบล็อกตามขนาดของการ์ด (CARD_SIZE)
      if (EEPROM.read(i) == 255) {  // ตรวจสอบว่าตำแหน่งใน EEPROM ว่าง (ค่า 255 หมายถึงว่าง)
        for (int j = 0; j < CARD_SIZE; j++) {  // วนลูปตามขนาดของการ์ดเพื่อบันทึกลงใน EEPROM
          EEPROM.write(i + j, card[j]);  // เขียนข้อมูลการ์ดทีละตัวอักษรลงใน EEPROM
        }
        return true;  // บันทึกการ์ดสำเร็จ
      }
    }
  }
  return false;  // ถ้าการ์ดถูกลงทะเบียนแล้ว หรือไม่มีพื้นที่ว่างใน EEPROM จะคืนค่า false
}

bool removeCardFromEEPROM(String card) {  // ฟังก์ชันสำหรับลบการ์ดจาก EEPROM
  for (int i = 0; i < EEPROM_SIZE; i += CARD_SIZE) {  // วนลูปผ่าน EEPROM ทีละบล็อกตามขนาดของการ์ด (CARD_SIZE)
    String storedCard = "";  // สร้างตัวแปรเก็บการ์ดที่อ่านได้จาก EEPROM
    for (int j = 0; j < CARD_SIZE; j++) {  // วนลูปตามขนาดของการ์ด
      storedCard += char(EEPROM.read(i + j));  // อ่านข้อมูลการ์ดจาก EEPROM และเพิ่มลงในตัวแปร storedCard
    }
    if (storedCard == card) {  // ถ้าการ์ดที่อ่านได้ตรงกับการ์ดที่ต้องการลบ
      for (int j = 0; j < CARD_SIZE; j++) {  // วนลูปตามขนาดของการ์ด
        EEPROM.write(i + j, 255);  // เขียนค่า 255 ลงใน EEPROM เพื่อลบการ์ดออก
      }
      return true;  // การลบสำเร็จ
    }
  }
  return false;  // ถ้าหากไม่พบการ์ดที่ต้องการลบใน EEPROM จะคืนค่า false
}

void displaySystemReady() {  // ฟังก์ชันแสดงข้อความว่าระบบพร้อมทำงาน
  lcd.clear();  // ล้างหน้าจอ LCD
  lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 0 คอลัมน์ที่ 0
  lcd.print("Scan Fingerprint");  // แสดงข้อความ "Scan Fingerprint" บนจอ LCD
  lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวที่ 1 คอลัมน์ที่ 0
  lcd.print("Or put your card");  // แสดงข้อความ "Or put your card" บนจอ LCD
}

void resetDisplayAfterDelay() {  // ฟังก์ชันรีเซ็ตหน้าจอหลังจากหน่วงเวลา
  delay(1000);  // หน่วงเวลา 1000 มิลลิวินาที (1 วินาที)
  displaySystemReady();  // เรียกฟังก์ชันแสดงสถานะพร้อมทำงานอีกครั้ง
}