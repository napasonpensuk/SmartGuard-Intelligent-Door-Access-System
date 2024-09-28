import cv2
import os
from datetime import datetime
import time
import requests  # สำหรับส่งข้อมูลเข้า LINE Notify
import pygame  # สำหรับเล่นเสียงแจ้งเตือน

# ตั้งค่า LINE Notify
LINE_TOKEN = 'ZUZJ0LbdfgtP7Bts6SjXHgjcqowbcHPnP4BXuU5gWl4'  # ใส่ LINE Notify Token ของคุณ
LINE_NOTIFY_URL = 'https://notify-api.line.me/api/notify'

# ฟังก์ชันการส่งข้อความพร้อมภาพไปยัง LINE
def send_line_message_with_image(message, image_path):
    # สร้าง headers โดยใส่ Authorization เป็น Bearer token ที่ได้จาก LINE API
    headers = {
        'Authorization': f'Bearer {LINE_TOKEN}'
    }

    # สร้างข้อมูลส่วนของข้อความที่จะส่งไปใน request
    data = {
        'message': message
    }

    try:
        # พยายามเปิดไฟล์ภาพที่อยู่ที่ path ที่ส่งเข้ามา
        with open(image_path, 'rb') as image_file:
            # กำหนดไฟล์ภาพที่จะถูกส่งไปใน request
            files = {'imageFile': image_file}

            # ทำการส่ง POST request ไปยัง LINE Notify URL พร้อมทั้ง headers, ข้อความ และไฟล์ภาพ
            response = requests.post(LINE_NOTIFY_URL, headers=headers, data=data, files=files)

        # ถ้าส่งสำเร็จ (status code 200)
        if response.status_code == 200:
            print(f"Message sent with image: {message}")  # แสดงข้อความว่า "ส่งข้อความพร้อมรูปภาพสำเร็จ"
            return True  # คืนค่า True เพื่อบ่งบอกว่าสำเร็จ
        else:
            # ถ้าไม่สำเร็จ แสดงสถานะของการตอบกลับ (response) และสาเหตุว่าล้มเหลว
            print(f"Failed to send message. Status code: {response.status_code}, Response: {response.text}")
            return False  # คืนค่า False ถ้าส่งไม่สำเร็จ
    
    # กรณีที่ไฟล์ภาพไม่พบใน path ที่ระบุ
    except FileNotFoundError:
        print(f"Image file not found: {image_path}")  # แสดงข้อความว่าไม่พบไฟล์ภาพ
    # กรณีเกิดข้อผิดพลาดอื่น ๆ
    except Exception as e:
        print(f"An error occurred: {e}")  # แสดงข้อผิดพลาดที่เกิดขึ้น

# ฟังก์ชันการเขียนข้อความพร้อมพื้นหลัง
def draw_text_with_background(img, text, pos, font, font_scale, font_thickness, text_color, bg_color):
    text_size, _ = cv2.getTextSize(text, font, font_scale, font_thickness)
    x, y = pos
    bg_x1, bg_y1 = x, y - text_size[1] - 10
    bg_x2, bg_y2 = x + text_size[0] + 10, y + 10
    # วาดพื้นหลังสี
    cv2.rectangle(img, (bg_x1, bg_y1), (bg_x2, bg_y2), bg_color, -1)
    # วาดข้อความบนพื้นหลัง
    cv2.putText(img, text, (x + 5, y - 5), font, font_scale, text_color, font_thickness, cv2.LINE_AA)

# ฟังก์ชันการลดขนาดภาพ
def resize_image(image, max_size=(800, 600)):
    h, w = image.shape[:2]
    scale = min(max_size[0] / w, max_size[1] / h)
    if scale < 1:  # ถ้าใหญ่กว่ากำหนดให้ลดขนาด
        image = cv2.resize(image, (int(w * scale), int(h * scale)), interpolation=cv2.INTER_AREA)
    return image

# ฟังก์ชันการเล่นเสียงแจ้งเตือน
def play_notification_sound():
    pygame.mixer.init()
    pygame.mixer.music.load("/home/odroid/Desktop/Facedetect.mp3")  # ใส่ชื่อไฟล์เสียงแจ้งเตือนที่ต้องการ
    pygame.mixer.music.play()

# เปิดกล้องเว็บแคม
cap = cv2.VideoCapture(0)
face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

save_folder = '/home/odroid/Desktop/saved_faces'
if not os.path.exists(save_folder):
    os.makedirs(save_folder)

last_sent_time = time.time()  # บันทึกเวลาปัจจุบันตอนเริ่มโปรแกรม

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # ทำให้ภาพเป็น Mirror (สลับซ้าย-ขวา)
    frame = cv2.flip(frame, 1)

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    detected_faces = face_cascade.detectMultiScale(gray, 1.3, 5)

    if len(detected_faces) > 0:  # ถ้าพบใบหน้าในเฟรม
        current_time = time.time()

        if current_time - last_sent_time >= 5:  # ตรวจสอบเวลาผ่านไปแล้ว 5 วินาที
            for (x, y, w, h) in detected_faces:
                # วาดกรอบสีเหลืองรอบใบหน้า
                cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 255), 2)

            # ประทับวันที่และเวลาในภาพ
            timestamp_text = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            draw_text_with_background(frame, timestamp_text, (10, frame.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX, 1, 2, (255, 255, 255), (0, 0, 0))

            # บันทึกทั้งเฟรม
            face_img = resize_image(frame)  # ลดขนาดภาพก่อนบันทึก
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = os.path.join(save_folder, f"face_{timestamp}.jpg")
            cv2.imwrite(filename, face_img)
            print(f"Saved face image: {filename}")

            # สร้างข้อความที่ต้องการส่งไป LINE พร้อมกับเวลาปัจจุบัน
            num_faces = len(detected_faces)
            if num_faces > 1:
                message = (
                    f"🚨 ตรวจพบความเคลื่อนไหว! \n\n"
                    f"📅 วันที่: {datetime.now().strftime('%d/%m/%Y')} \n"
                    f"🕒 เวลา: {datetime.now().strftime('%H:%M:%S')} น. \n\n"
                    f"ระบบตรวจจับพบ {num_faces} บุคคลที่หน้าบ้าน 🎥"
                )
            else:
                message = (
                    f"🚨 ตรวจพบความเคลื่อนไหว! \n\n"
                    f"📅 วันที่: {datetime.now().strftime('%d/%m/%Y')} \n"
                    f"🕒 เวลา: {datetime.now().strftime('%H:%M:%S')} น. \n\n"
                    f"ระบบตรวจจับพบ 1 บุคคลที่หน้าบ้าน 🎥"
                )

            if send_line_message_with_image(message, filename):
                last_sent_time = current_time  # อัปเดตเวลาที่บันทึกภาพล่าสุด
                play_notification_sound()  # เล่นเสียงแจ้งเตือน

    # แสดงภาพ
    cv2.imshow('Face Detection', frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()