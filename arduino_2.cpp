#include <Adafruit_Fingerprint.h>  // ไลบรารีสำหรับเซ็นเซอร์ลายนิ้วมือ
#include <SoftwareSerial.h>        // ไลบรารีสำหรับการสื่อสารผ่านพอร์ตอนุกรมเสมือน
#include <LiquidCrystal_I2C.h>     // ไลบรารีสำหรับการควบคุมจอ LCD ผ่าน I2C
#include <Wire.h>                  // ไลบรารีสำหรับการสื่อสารผ่านบัส I2C
#include <TimeLib.h>               // ไลบรารีสำหรับจัดการเวลา
#include <DS1307RTC.h>             // ไลบรารีสำหรับสื่อสารกับโมดูล RTC DS1307
#include <DHT.h>                   // ไลบรารีสำหรับอ่านค่าอุณหภูมิและความชื้นจากเซ็นเซอร์ DHT
#include <avr/sleep.h>             // ไลบรารีสำหรับการจัดการโหมดประหยัดพลังงาน
#include <avr/interrupt.h>         // ไลบรารีสำหรับการจัดการการขัดจังหวะ (Interrupts)

#define DHTPIN 2                   // กำหนดขาเชื่อมต่อกับเซ็นเซอร์ DHT ที่ขา 2
#define DHTTYPE DHT11              // กำหนดประเภทเซ็นเซอร์ DHT เป็น DHT11
#define MQ2AnalogPin A3            // กำหนดขาอนาล็อก A3 สำหรับเซ็นเซอร์ MQ2
#define FlameDigitalPin 3          // กำหนดขา 3 สำหรับเซ็นเซอร์ตรวจจับไฟ
#define BuzzerPin 7                // กำหนดขา 7 สำหรับบัซเซอร์

SoftwareSerial mySerial(10, 11);   // กำหนดพอร์ตอนุกรมเสมือน (Rx, Tx) สำหรับเซ็นเซอร์ลายนิ้วมือ
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);  // อินสแตนซ์เซ็นเซอร์ลายนิ้วมือ

SoftwareSerial commSerial(8, 9);   // กำหนดพอร์ตอนุกรมเสมือนสำหรับการสื่อสารกับบอร์ดอื่น (ขา 8 และ 9)

LiquidCrystal_I2C lcd(0x27, 16, 2); // อินสแตนซ์จอ LCD ผ่าน I2C ที่อยู่ 0x27 ขนาด 16x2
tmElements_t tm;                    // โครงสร้างข้อมูลสำหรับเก็บเวลาจาก RTC
DHT dht(DHTPIN, DHTTYPE);           // อินสแตนซ์เซ็นเซอร์ DHT11

unsigned long previousMillis = 0;         // ตัวแปรสำหรับเก็บเวลาปัจจุบัน (เพื่อใช้ในการจับเวลา)
const long interval = 1000;               // กำหนดช่วงเวลาห่างกัน 1 วินาที
unsigned long previousMillisSensors = 0;  // ตัวแปรสำหรับจับเวลาอ่านเซ็นเซอร์
const long intervalSensors = 3000;        // ตั้งค่าการอ่านเซ็นเซอร์ทุก 3 วินาที
bool blinkState = true;                   // ตัวแปรสำหรับสถานะการกระพริบของ LCD
bool isDoorOpened = false;                // ตัวแปรสถานะประตู (เปิด/ปิด)
const int mq2Threshold = 100;             // ค่าที่ใช้สำหรับตั้งเกณฑ์การแจ้งเตือนจากเซ็นเซอร์ MQ2
unsigned long previousMillisPrint = 0;    // ตัวแปรสำหรับจับเวลาการพิมพ์ข้อความ
const long intervalPrint = 3000;          // ตั้งเวลาห่างกันทุก 3 วินาที
int durationDHT = 3;                      // ตั้งค่าระยะเวลาในการแสดงผล DHT บนหน้าจอ LCD
int delayTimer = 0;                       // ตัวแปรสำหรับการจับเวลาเพื่อหน่วงการทำงาน
int flameDigitalValue;                    // ตัวแปรเก็บค่าการอ่านจากเซ็นเซอร์ตรวจจับไฟ

// ฟังก์ชันสำหรับตั้งค่าการทำงานของ Timer
void setupTimer();

// ฟังก์ชันสำหรับแสดงการแจ้งเตือนบนจอ LCD พร้อมเสียงเตือน
void showAlert(const char* line1, const char* line2);

// ฟังก์ชันสำหรับเข้าสู่โหมดประหยัดพลังงาน (Sleep Mode)
void enterSleepMode();

byte degreeSymbol[8] = { // สร้างสัญลักษณ์สำหรับ "องศา" เพื่อใช้แสดงผลบนจอ LCD
  0b00111,  // แถวที่ 1
  0b00101,  // แถวที่ 2
  0b00111,  // แถวที่ 3
  0b00000,  // แถวที่ 4
  0b00000,  // แถวที่ 5
  0b00000,  // แถวที่ 6
  0b00000,  // แถวที่ 7
  0b00000   // แถวที่ 8
};

byte dropSymbol[8] = { // สร้างสัญลักษณ์ "หยดน้ำ" เพื่อใช้แสดงผลบนจอ LCD
  0b00100,  // แถวที่ 1
  0b00100,  // แถวที่ 2
  0b00100,  // แถวที่ 3
  0b01010,  // แถวที่ 4
  0b10001,  // แถวที่ 5
  0b10001,  // แถวที่ 6
  0b10001,  // แถวที่ 7
  0b01110   // แถวที่ 8
};

byte flameSymbol[8] = { // สร้างสัญลักษณ์ "ไฟ" เพื่อใช้แสดงผลบนจอ LCD
  0b00100,  // แถวที่ 1
  0b01110,  // แถวที่ 2
  0b01110,  // แถวที่ 3
  0b11011,  // แถวที่ 4
  0b10101,  // แถวที่ 5
  0b10101,  // แถวที่ 6
  0b10001,  // แถวที่ 7
  0b01010   // แถวที่ 8
};

ISR(TIMER1_COMPA_vect) { // อินเทอร์รัพท์เมื่อ Timer 1 ตรงกับค่าที่กำหนด
  // ปลุกจากโหมด Sleep Mode ทุกครั้งที่ Timer ทำงาน
  sleep_disable();  // ปิดการทำงานของ Sleep Mode
  TCNT1 = 0;        // รีเซ็ตค่า Timer
  delayTimer++;     // เพิ่มค่าตัวแปรจับเวลา delay
}

// ฟังก์ชันการขัดจังหวะสำหรับ INT0 (ขัดจังหวะเมื่อตรวจจับเหตุการณ์บางอย่าง)
ISR(INT0_vect) {
  // ใส่โค้ดสำหรับจัดการงานที่ต้องการ เช่น ตรวจสอบสัญญาณจากเซ็นเซอร์
}

// ISR สำหรับ INT1 เมื่อเซ็นเซอร์ไฟตรวจพบไฟ จะปลุกให้ CPU ตื่น
ISR(INT1_vect) {
  // ปลุก CPU เมื่อเซ็นเซอร์ตรวจพบไฟ
  flameDigitalValue = digitalRead(FlameDigitalPin);  // อ่านค่าจากเซ็นเซอร์ไฟอีกครั้งเพื่อตรวจสอบสถานะปัจจุบัน
}

void setup() {
  Serial.begin(9600);              // เริ่มต้นการสื่อสารผ่านพอร์ตอนุกรมหลักที่ความเร็ว 9600 bps
  mySerial.begin(57600);           // เริ่มต้นการสื่อสารกับเซ็นเซอร์ลายนิ้วมือที่ความเร็ว 57600 bps
  commSerial.begin(9600);          // เริ่มต้นการสื่อสารกับบอร์ดอื่นที่ความเร็ว 9600 bps
  Wire.begin();                    // เริ่มต้นการทำงานของ I2C
  lcd.init();                      // เริ่มต้นการทำงานของจอ LCD
  lcd.backlight();                 // เปิดไฟหลังจอ LCD
  lcd.home();                      // ตั้งตำแหน่งเคอร์เซอร์ของ LCD ไว้ที่ตำแหน่งเริ่มต้น
  dht.begin();                     // เริ่มต้นการทำงานของเซ็นเซอร์ DHT11

  pinMode(FlameDigitalPin, INPUT);  // ตั้งขาของเซ็นเซอร์ตรวจจับไฟเป็นอินพุต
  pinMode(BuzzerPin, OUTPUT);       // ตั้งขาสำหรับบัซเซอร์เป็นเอาท์พุต

  lcd.createChar(0, degreeSymbol);  // สร้างสัญลักษณ์ "องศา" บน LCD ที่ตำแหน่ง 0
  lcd.createChar(1, dropSymbol);    // สร้างสัญลักษณ์ "หยดน้ำ" บน LCD ที่ตำแหน่ง 1
  lcd.createChar(2, flameSymbol);   // สร้างสัญลักษณ์ "ไฟ" บน LCD ที่ตำแหน่ง 2

  // เริ่มต้นเซ็นเซอร์ลายนิ้วมือ
  finger.begin(57600);              // กำหนดการสื่อสารกับเซ็นเซอร์ลายนิ้วมือที่ 57600 bps
  if (finger.verifyPassword()) {    // ตรวจสอบการเชื่อมต่อกับเซ็นเซอร์ลายนิ้วมือ
    Serial.println("Fingerprint sensor detected.");  // แสดงข้อความเมื่อพบเซ็นเซอร์ลายนิ้วมือ
  } else {
    Serial.println("Did not find fingerprint sensor :(");  // แสดงข้อความเมื่อไม่พบเซ็นเซอร์ลายนิ้วมือ
    while (1) { delay(1); }        // หยุดโปรแกรมหากไม่พบเซ็นเซอร์ลายนิ้วมือ
  }

  // ตั้งค่าการขัดจังหวะ INT0 (PD2) สำหรับจัดการเหตุการณ์อื่น ๆ ที่ไม่ใช่ DHT11
  EICRA |= (1 << ISC01);           // เลือกการขัดจังหวะที่ขอบสัญญาณตก (falling edge) สำหรับ INT0
  EIMSK |= (1 << INT0);            // เปิดใช้งานการขัดจังหวะ INT0
  setupTimer();                    // เรียกใช้ฟังก์ชันตั้งค่า Timer

  // เปิดใช้งานการขัดจังหวะ INT1 สำหรับเซ็นเซอร์ตรวจจับไฟ (PD3)
  EICRA |= (1 << ISC11);           // เลือกการขัดจังหวะที่ขอบสัญญาณตก (falling edge) สำหรับ INT1
  EIMSK |= (1 << INT1);            // เปิดใช้งานการขัดจังหวะ INT1

  // เปิดใช้งานการขัดจังหวะทั้งหมด
  sei();                           // เปิดใช้งานการขัดจังหวะ (Interrupts)
}

void updateDisplay() {
  // อัปเดตเวลา วันที่ และข้อมูลอุณหภูมิจาก RTC (Real-Time Clock)
  if (RTC.read(tm)) {  // อ่านข้อมูลเวลาจากโมดูล RTC
    lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวบนสุดของจอ LCD
    if (tm.Hour < 10) lcd.print(0);  // ถ้าเวลาชั่วโมงมีค่าน้อยกว่า 10 ให้แสดงเลข 0 ข้างหน้า
    lcd.print(tm.Hour);  // แสดงผลชั่วโมง
    if (blinkState) {  // ถ้า blinkState เป็น true ให้แสดงเครื่องหมาย ":" มิฉะนั้นจะแสดงเป็นช่องว่าง
      lcd.print(":");
    } else {
      lcd.print(" ");
    }
    if (tm.Minute < 10) lcd.print(0);  // ถ้าเวลานาทีมีค่าน้อยกว่า 10 ให้แสดงเลข 0 ข้างหน้า
    lcd.print(tm.Minute);  // แสดงผลนาที
    lcd.print(" ");  // เพิ่มช่องว่างระหว่างเวลาและวันที่
    if (tm.Day < 10) lcd.print(0);  // ถ้าวันมีค่าน้อยกว่า 10 ให้แสดงเลข 0 ข้างหน้า
    lcd.print(tm.Day);  // แสดงผลวันที่
    lcd.print("/");  // แสดงเครื่องหมาย "/"
    if (tm.Month < 10) lcd.print(0);  // ถ้าเดือนมีค่าน้อยกว่า 10 ให้แสดงเลข 0 ข้างหน้า
    lcd.print(tm.Month);  // แสดงผลเดือน
    lcd.print("/");  // แสดงเครื่องหมาย "/"
    lcd.print(tmYearToCalendar(tm.Year));  // แสดงผลปี
  }

  // อัปเดตข้อมูลอุณหภูมิและความชื้นจากเซ็นเซอร์ DHT11
  float humidity = dht.readHumidity();  // อ่านค่าความชื้น
  float temperature = dht.readTemperature();  // อ่านค่าอุณหภูมิ
  if (!isnan(humidity) && !isnan(temperature)) {  // ตรวจสอบว่าค่าที่อ่านได้ไม่เป็น NaN (ค่าที่ไม่ใช่ตัวเลข)
    lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวล่างสุดของจอ LCD
    lcd.print("Temp:");  // แสดงข้อความ "Temp:"
    lcd.print(temperature);  // แสดงค่าอุณหภูมิ
    lcd.print("C ");  // แสดงเครื่องหมาย "C" สำหรับองศาเซลเซียส
    lcd.print("Hum:");  // แสดงข้อความ "Hum:"
    lcd.print(humidity);  // แสดงค่าความชื้น
    lcd.print("%");  // แสดงเครื่องหมาย "%"
  }
}

void checkFingerprint() {
  int p = finger.getImage();  // รับภาพลายนิ้วมือจากเซ็นเซอร์
  if (p == FINGERPRINT_OK) {  // ถ้ารับภาพลายนิ้วมือได้สำเร็จ
    Serial.println("Fingerprint detected.");  // แสดงข้อความว่าพบลายนิ้วมือ
    // ประมวลผลเพิ่มเติมหากจำเป็น
  }
}

void showAlert(const char* line1, const char* line2) {
  lcd.clear();  // ล้างหน้าจอ LCD
  lcd.setCursor(0, 0);  // ตั้งเคอร์เซอร์ไว้ที่แถวบนสุด
  lcd.print(line1);  // แสดงข้อความบรรทัดแรกบนจอ LCD
  lcd.setCursor(0, 1);  // ตั้งเคอร์เซอร์ไว้ที่แถวล่างสุด
  lcd.print(line2);  // แสดงข้อความบรรทัดที่สองบนจอ LCD
  tone(BuzzerPin, 1000);  // ส่งเสียงจากบัซเซอร์ที่ความถี่ 1000 Hz
  delay(300);  // รอ 300 มิลลิวินาที
  noTone(BuzzerPin);  // ปิดเสียงบัซเซอร์
}

// ฟังก์ชันสำหรับตั้งค่า Sleep Mode (โหมดพักเครื่องเพื่อประหยัดพลังงาน)
void enterSleepMode() {
  set_sleep_mode(SLEEP_MODE_IDLE);  // ตั้งค่าโหมด Idle เพื่อให้ I2C และ Serial ยังคงทำงาน
  sleep_enable();  // เปิดการทำงานของ Sleep Mode
  sleep_cpu();     // เข้าสู่โหมดพักเครื่อง
  sleep_disable(); // ปิดโหมดพักเครื่องหลังจากตื่นขึ้นมาทำงาน
}

void loop() {
  unsigned long currentMillis = millis();  // อ่านเวลาปัจจุบันเป็นมิลลิวินาที

  if (flameDigitalValue == LOW) {  // ถ้าไม่ตรวจพบไฟ (ค่า flameDigitalValue เป็น LOW)
    enterSleepMode();  // เข้าสู่โหมดพักเครื่อง (Sleep Mode)
  } else {
    // ถ้าตรวจพบไฟ
    Serial.println("Flame detected, system active.");  // แสดงข้อความใน Serial Monitor ว่าตรวจพบไฟและระบบกำลังทำงาน
  }

  // ถ้าค่าเวลาปัจจุบันห่างจากค่าที่จับเวลาไว้เท่ากับช่วงเวลาที่กำหนด (interval)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // รีเซ็ตค่า previousMillis เป็นค่าเวลาปัจจุบัน
    blinkState = !blinkState;  // สลับสถานะ blinkState (กระพริบไฟหรือข้อความ)
  }

  // อ่านค่าจากเซ็นเซอร์ DHT11 ทุก 2 วินาที
  if (delayTimer >= 1) {  // ถ้า delayTimer มีค่าเท่ากับหรือมากกว่า 1
    delayTimer = 0;  // รีเซ็ตค่า delayTimer
    float humidity = dht.readHumidity();  // อ่านค่าความชื้นจากเซ็นเซอร์ DHT11
    float temperature = dht.readTemperature();  // อ่านค่าอุณหภูมิจากเซ็นเซอร์ DHT11
    char buf[6];  // ตัวแปรบัฟเฟอร์สำหรับแปลงค่าอุณหภูมิและความชื้นเป็นข้อความ

    if (isnan(humidity) || isnan(temperature)) {  // ตรวจสอบว่าค่าความชื้นหรืออุณหภูมิที่อ่านได้ไม่เป็น NaN (ค่าที่ไม่ใช่ตัวเลข)
      Serial.println("Failed to read from DHT sensor!");  // แสดงข้อความใน Serial Monitor ว่าการอ่านค่าจากเซ็นเซอร์ล้มเหลว
    } else {
      lcd.setCursor(0, 1);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวล่างสุดของจอ LCD
      lcd.write(2);  // แสดงสัญลักษณ์ "ไฟ"
      lcd.print("=");  // แสดงเครื่องหมาย "="
      lcd.print(dtostrf(temperature, 4, 1, buf));  // แสดงค่าอุณหภูมิที่แปลงเป็นข้อความแล้ว
      lcd.write(0);  // แสดงสัญลักษณ์ "องศา"
      lcd.print("C ");  // แสดงหน่วยเป็น "C"
      lcd.write(1);  // แสดงสัญลักษณ์ "หยดน้ำ"
      lcd.print("=");  // แสดงเครื่องหมาย "="
      lcd.print(dtostrf(humidity, 4, 1, buf));  // แสดงค่าความชื้นที่แปลงเป็นข้อความแล้ว
      lcd.print("%");  // แสดงเครื่องหมาย "%"

      // แสดงค่าอุณหภูมิและความชื้นใน Serial Monitor
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" °C, Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");
    }
  }

  // อ่านค่าจากเซ็นเซอร์ MQ2 และเซ็นเซอร์ตรวจจับไฟ (Flame Sensor)
  int mq2AnalogValue = analogRead(MQ2AnalogPin);  // อ่านค่าควันจากเซ็นเซอร์ MQ2 (เป็นค่าอนาล็อก)
  int flameDigitalValue = digitalRead(FlameDigitalPin);  // อ่านค่าจากเซ็นเซอร์ตรวจจับไฟ (เป็นค่าดิจิตอล)

  // แสดงค่าที่อ่านได้จากเซ็นเซอร์ MQ2 และ Flame Sensor ใน Serial Monitor ทุก 3 วินาที
  if (currentMillis - previousMillisPrint >= intervalPrint) {  // ถ้าเวลาผ่านไปเกิน 3 วินาที
    previousMillisPrint = currentMillis;  // อัปเดตค่า previousMillisPrint

    Serial.print("MQ2 Gas Sensor Value: ");  // แสดงข้อความบอกว่าเป็นค่าจากเซ็นเซอร์ MQ2
    Serial.println(mq2AnalogValue);  // แสดงค่าที่อ่านได้จากเซ็นเซอร์ MQ2
    Serial.print("Flame Sensor Value: ");  // แสดงข้อความบอกว่าเป็นค่าจากเซ็นเซอร์ตรวจจับไฟ
    Serial.println(flameDigitalValue);  // แสดงค่าที่อ่านได้จากเซ็นเซอร์ตรวจจับไฟ
  }

  // แสดงเวลาและวันที่บนจอ LCD
  if (RTC.read(tm)) {  // ถ้าสามารถอ่านข้อมูลจาก RTC ได้
    lcd.setCursor(0, 0);  // ตั้งตำแหน่งเคอร์เซอร์ที่แถวบนสุดของจอ LCD
    if (tm.Hour < 10) lcd.print(0);  // ถ้าชั่วโมงน้อยกว่า 10 ให้แสดงเลข 0 ข้างหน้า
    lcd.print(tm.Hour);  // แสดงค่าชั่วโมง
    if (blinkState) {  // ถ้า blinkState เป็นจริง
      lcd.print(":");  // แสดงเครื่องหมาย :
    } else {
      lcd.print(" ");  // ถ้า blinkState เป็นเท็จ แสดงช่องว่าง
    }
    if (tm.Minute < 10) lcd.print(0);  // ถ้านาทีน้อยกว่า 10 ให้แสดงเลข 0 ข้างหน้า
    lcd.print(tm.Minute);  // แสดงค่านาที
    lcd.print(" ");  // เว้นช่องว่าง
    if (tm.Day < 10) lcd.print(0);  // ถ้าวันน้อยกว่า 10 ให้แสดงเลข 0 ข้างหน้า
    lcd.print(tm.Day);  // แสดงวันที่
    lcd.print("/");  // แสดงเครื่องหมาย /
    if (tm.Month < 10) lcd.print(0);  // ถ้าเดือนน้อยกว่า 10 ให้แสดงเลข 0 ข้างหน้า
    lcd.print(tm.Month);  // แสดงเดือน
    lcd.print("/");  // แสดงเครื่องหมาย /
    lcd.print(tmYearToCalendar(tm.Year));  // แสดงปีที่แปลงจากค่า tm.Year
  }

  // ตรวจสอบว่ามีข้อมูลเข้ามาผ่านการสื่อสารจากพอร์ตอนุกรมหรือไม่
  if (commSerial.available()) {
    String message = commSerial.readString();  // อ่านข้อมูลที่เข้ามาเป็นสตริง
    if (message.indexOf("DOOR_CLOSED") >= 0) {  // ถ้าข้อความมีคำว่า "DOOR_CLOSED"
      isDoorOpened = false;  // ตั้งค่าสถานะประตูเป็นปิด
      Serial.println("Door closed, fire and gas sensor reset.");  // แสดงข้อความใน Serial Monitor ว่าประตูปิดแล้วและรีเซ็ตเซ็นเซอร์ไฟและก๊าซ
    }
  }

  // ตรวจจับไฟ (Flame) และควัน (MQ2) เพื่อเปิดประตูฉุกเฉิน
  if (flameDigitalValue == 1 && !isDoorOpened) {  // ถ้าตรวจพบไฟและประตูยังไม่ถูกเปิด
    Serial.println("Flame detected, starting alert.");  // แสดงข้อความใน Serial Monitor ว่าตรวจพบไฟ
    commSerial.println("EMERGENCY_OPEN");  // ส่งสัญญาณผ่านพอร์ตอนุกรมเพื่อเปิดประตูฉุกเฉิน
    isDoorOpened = true;  // ตั้งค่าสถานะว่าประตูถูกเปิดแล้ว

    while (flameDigitalValue == 1) {  // วนลูปจนกว่าจะไม่ตรวจพบไฟ
      showAlert("Flame Detect!", "Emergency Exit!");  // แสดงการแจ้งเตือนผ่านจอ LCD และบัซเซอร์
      flameDigitalValue = digitalRead(FlameDigitalPin);  // อ่านค่าเซ็นเซอร์ตรวจจับไฟซ้ำเพื่ออัปเดตสถานะปัจจุบัน
      delay(100);  // หน่วงเวลา 100 มิลลิวินาที
    }

    Serial.println("Fire cleared, stopping alert.");  // แสดงข้อความใน Serial Monitor ว่าไฟดับแล้ว
    isDoorOpened = false;  // รีเซ็ตสถานะประตูหลังจากที่ไฟดับ
    flameDigitalValue = LOW;  // รีเซ็ตค่าการตรวจจับไฟ
    lcd.clear();  // ล้างหน้าจอ LCD เมื่อไฟดับ
  }

  // ตรวจจับค่าก๊าซจากเซ็นเซอร์ MQ2 เพื่อเปิดประตูฉุกเฉิน
  if (mq2AnalogValue > mq2Threshold && !isDoorOpened) {  // ถ้าค่าก๊าซเกินเกณฑ์ที่กำหนดและประตูยังไม่ถูกเปิด
    Serial.println("Gas Alert! Emergency Exit!");  // แสดงข้อความใน Serial Monitor ว่าตรวจพบควันก๊าซ
    commSerial.println("EMERGENCY_OPEN");  // ส่งสัญญาณผ่านพอร์ตอนุกรมเพื่อเปิดประตูฉุกเฉิน
    isDoorOpened = true;  // ตั้งค่าสถานะว่าประตูถูกเปิดแล้ว

    while (mq2AnalogValue > mq2Threshold) {  // วนลูปจนกว่าค่าก๊าซจะน้อยกว่าเกณฑ์ที่ตั้งไว้
      showAlert("Gas Alert!", "Emergency Exit!");  // แสดงการแจ้งเตือนผ่านจอ LCD และบัซเซอร์
      mq2AnalogValue = analogRead(MQ2AnalogPin);  // อ่านค่าเซ็นเซอร์ซ้ำเพื่ออัปเดตสถานะปัจจุบัน
      delay(500);  // หน่วงเวลา 500 มิลลิวินาทีเพื่อไม่ให้ลูปรันเร็วเกินไป
    }

    Serial.println("Gas cleared, resetting door status.");  // แสดงข้อความใน Serial Monitor ว่าควันก๊าซหายไปแล้ว
    isDoorOpened = false;  // รีเซ็ตสถานะประตูหลังจากที่ควันก๊าซหายไป
  }

  // ตรวจจับลายนิ้วมือ
  int p = finger.getImage();  // รับภาพลายนิ้วมือจากเซ็นเซอร์
  if (p == FINGERPRINT_OK) {  // ถ้าการรับภาพลายนิ้วมือสำเร็จ
      p = finger.image2Tz();  // แปลงภาพลายนิ้วมือให้เป็น Template
      if (p == FINGERPRINT_OK) {  // ถ้าแปลงภาพลายนิ้วมือเป็น Template สำเร็จ
          p = finger.fingerFastSearch();  // ค้นหาลายนิ้วมือในฐานข้อมูลอย่างรวดเร็ว
          if (p == FINGERPRINT_OK) {  // ถ้าพบลายนิ้วมือที่ตรงกันในฐานข้อมูล
              Serial.print("Found fingerprint with ID: ");  // แสดงข้อความใน Serial Monitor
              Serial.println(finger.fingerID);  // แสดงหมายเลข ID ของลายนิ้วมือที่พบ
              commSerial.println("OPEN_DOOR");  // ส่งสัญญาณเปิดประตูผ่านพอร์ตอนุกรม
              isDoorOpened = true;  // ตั้งค่าสถานะประตูเป็นเปิด
              lcd.clear();  // ล้างหน้าจอ LCD
              lcd.setCursor(0, 0);  // ตั้งเคอร์เซอร์ที่แถวบนสุด
              lcd.print("Fingerprint ID:");  // แสดงข้อความ "Fingerprint ID:"
              lcd.setCursor(0, 1);  // ตั้งเคอร์เซอร์ที่แถวล่างสุด
              lcd.print(finger.fingerID);  // แสดงหมายเลข ID ของลายนิ้วมือ
              lcd.print(" Welcome!");  // แสดงข้อความ "Welcome!"
              tone(BuzzerPin, 1000);  // ส่งเสียงบัซเซอร์ที่ความถี่ 1000 Hz
              delay(500);  // หน่วงเวลา 500 มิลลิวินาที
              noTone(BuzzerPin);  // ปิดเสียงบัซเซอร์
              delay(2000);  // หน่วงเวลา 2 วินาที
              lcd.clear();  // ล้างหน้าจอ LCD

              // แก้ไข: รีเซ็ตสถานะประตูหลังจากแสดงข้อความเสร็จ
              isDoorOpened = false;  
          } else {
              // เมื่อไม่พบลายนิ้วมือที่บันทึกไว้
              lcd.clear();  // ล้างหน้าจอ LCD
              lcd.setCursor(0, 0);  // ตั้งเคอร์เซอร์ที่แถวบนสุด
              lcd.print("Fingerprint");  // แสดงข้อความ "Fingerprint"
              lcd.setCursor(0, 1);  // ตั้งเคอร์เซอร์ที่แถวล่างสุด
              lcd.print("Not Found!");  // แสดงข้อความ "Not Found!"
              Serial.print("Fingerprint Not Found!"); //แสดงข้อความใน Serial Monitor
              // ส่งเสียงบัซเซอร์เมื่อไม่พบลายนิ้วมือ
              tone(BuzzerPin, 1000);  // ส่งเสียงบัซเซอร์ที่ความถี่ 1000 Hz
              delay(300);  // หน่วงเวลา 300 มิลลิวินาที
              noTone(BuzzerPin);  // ปิดเสียงบัซเซอร์
              delay(100);  // หน่วงเวลา 100 มิลลิวินาที
              tone(BuzzerPin, 1000);  // ส่งเสียงบัซเซอร์อีกครั้งที่ความถี่ 1000 Hz
              delay(300);  // หน่วงเวลา 300 มิลลิวินาที
              noTone(BuzzerPin);  // ปิดเสียงบัซเซอร์

              // แสดงข้อความเป็นเวลา 2 วินาที
              delay(2000);  // หน่วงเวลา 2 วินาที
              lcd.clear();  // ล้างหน้าจอ LCD
          }
      }
  }
}

void setupTimer()
{
  cli();
  // ตั้งค่า Timer1 ในโหมด CTC (Clear Timer on Compare Match)
  TCCR1A = 0;  // ปิดการตั้งค่าโมดูล Output Compare
  TCCR1B = 0;  // ตั้งค่า Timer1
  TCCR1B |= (1 << WGM12);  // เปิดใช้งานโหมด CTC
  TCCR1B |= (1 << CS12) | (1 << CS10);  // ตั้ง Prescaler เป็น 1024
  // ตั้งค่าค่าที่ต้องจับคู่เพื่อให้เกิดการขัดจังหวะทุก 1 วินาที
  OCR1A = 46874;  // คำนวณจาก: (16MHz / (1024 * 1Hz)) - 1
  // 46874 == 3 second
  TCNT1 = 0;

  // เปิดการขัดจังหวะที่เกิดจาก Timer1
  TIMSK1 |= (1 << OCIE1A);

  // เปิดการขัดจังหวะทั้งหมด
  sei();
}