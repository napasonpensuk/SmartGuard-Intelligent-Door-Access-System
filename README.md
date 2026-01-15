# SmartGuard: Intelligent Door Access System

## วิชา 240-319 Embedded System Developer Module

## จัดทำโดย

นางสาวนภสร ป็นสุข 6510110225

นายภัคคภูมิ จินดาเดช 6510110340

นายตุลธร จันทระ 6510110632

Section 01

## เสนอ

รศ.ดร. ทวีศักดิ์ เรืองพีระกุล

รศ.ดร. ปัญญยศ ไชยกาฬ

ผศ.ดร. วชรินทร์ แก้วอภิชัย

คณะวิศวกรรมศาสตร์  สาขาวิชาวิศวกรรมคอมพิวเตอร์

มหาวิทยาลัยสงขลานครินทร์วิทยาเขตหาดใหญ่

==============================================================================================================

## Tools

- Arduino UNO R3
- RFID MFRC-522 ตรวจจับคีย์การ์ด
- Keypad 4*4 ป้อน input Password
- LCD I2C display แสดงจำนวนหลักของรหัสและคำอธิบาย
- Ultrasonic Sensor HCSR04 ตรวจจับร่างกายให้เปิดประตูค้างไว้
- Servo Motor แกนหมุนของประตู
- KY-026 Flame Sensor ตรวจจับเปลวไฟ
- MQ-2 Gas Sensor ตรวจจับควัน
- DHT11 Temperature and Humidity Sensor ตรวจวัดอุณหภูมิและความชื้น
- Buzzer ส่งเสียงเตือนเมื่อเกิดเหตุฉุกเฉิน เมื่อเปิดประตู และเมื่อดำเนินการต่างๆ
- Fingerprint Sensor สแกนลายนิ้วมือ (Bio) เพื่อเปิดประตู
- LED ติดสว่างเมื่อเปิดประตูสำเร็จและจะดับเมื่อปิดประตูและกระพริบเมื่ออินพุตผิดพลาด
- ใช้ EEPROM เก็บข้อมูลคีย์การ์ด
- Odroid & webcam บันทึกภาพเมื่อตรวจเจอใบหน้าและส่งเข้าทาง LINE พร้อมประทับเวลา และระบุจำนวนคน
- ใช้ PCF8574 ขยาย I/O ของบอร์ด
- เมื่อ Sensor แต่ละตัวตรวจพบเหตุการณ์ผิดปกติจะสั่งเปิดประตูอัตโนมัติ
- ใช้ RTC DS1307 แสดงเวลา วันเดือนปีบน LCD
- Sleep Mode เมื่อไม่มีการตรวจจับเปลวไฟ

==============================================================================================================

## Video Preview

https://drive.google.com/file/d/1tQIbv8PFuV6UCVJl5ukV_tj5qc5oH9BB/view?fbclid=IwY2xjawFmlzhleHRuA2FlbQIxMAABHejbRDkIX-wvmYnT2QhPVE21tDd2WGN0YZ_jAD3sMbJsS3aEsoV9DyILIg_aem_lRbSFC1ngE4KnezXk_UKJA

==============================================================================================================

## Presentation Flie

https://drive.google.com/file/d/1dvtefZqTciarLKhhGUjkPI2ET8FvZHIA/view?usp=drive_link

==============================================================================================================


## SmartGuard: Intelligent Door Access System

## Course 240-319 Embedded System Developer Module

## Prepared by:

Ms. Napason Pensuk 6510110225

Mr. Phakkapoom Jindadech 6510110340

Mr. Tulthorn Chantra 6510110632

Section 01

## Submitted to:

Assoc. Prof. Dr. Taweesak Ruangpeerakul

Assoc. Prof. Dr. Panyayot Chaiyakarn

Asst. Prof. Dr. Wacharin Kaewapichai

Faculty of Engineering, Department of Computer Engineering

Prince of Songkla University, Hat Yai Campus

=================================================================================================================

## Tools

- Arduino UNO R3
- RFID MFRC-522 keycard detector
- 4x4 keypad for password input
- LCD I2C display showing password digits and description
- Ultrasonic Sensor HCSR04 for body detection and door opening control
- Servo Motor for door rotation
- KY-026 Flame Sensor for flame detection
- MQ-2 Gas Sensor for smoke detection
- DHT11 Temperature and Humidity Sensor for temperature and humidity measurement
- Buzzer for emergency alarm (e.g., door opening) And when performing various operations:
- Fingerprint Sensor scans fingerprints (Bio) to open the door.
- LED lights up when the door is successfully opened and turns off when the door is closed, and blinks when there is an incorrect input.
- Uses EEPROM to store key card data.
- Odroid & webcam record images when a face is detected and send them via LINE with a timestamp and the number of people.
- Uses PCF8574 to expand the board's I/O.
- When each sensor detects an abnormal event, it automatically opens the door.
- Uses RTC DS1307 to display the time, date, and time on the LCD.
- Sleep Mode when no flames are detected.

============================================================================================================

## Video Preview

https://drive.google.com/file/d/1tQIbv8PFuV6UCVJl5ukV_tj5qc5oH9BB/view?fbclid=IwY2xjawFmlzhleHRuA2FlbQIxMAABHejbRDkIX-wvmYnT2QhPVE21tDd2WGN0YZ_jAD3sMbJsS3a EsoV9DyILIg_aem_lRbSFC1ngE4KnezXk_UKJA

================================================================================================

## Presentation Flie

https://drive.google.com/file/d/1dvtefZqTciarLKhhGUjkPI2ET8FvZHIA/view?usp=drive_link

================================================================================================

## Report Project Flie

https://drive.google.com/file/d/1fS12gOmGymEOauKYsH2hISeXrR0UCLyZ/view?usp=drive_link


================================================================================================


