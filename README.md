## 📖 Project Overview

This project is an **IoT-based monitoring and alert system** using an **ESP32** microcontroller.
It integrates **five load cells (via HX711 amplifiers)** and a **DS18B20 temperature sensor** to measure weight and temperature in real-time. The data is processed, displayed locally via Serial Monitor, and sent to cloud services for monitoring and notifications.

### 🔹 Key Features

* **Weight Measurement**:
  Reads weight from 5 independent load cells, each with its own calibration factor and offset, ensuring accurate measurement.

* **Temperature Monitoring**:
  Uses a DS18B20 digital temperature sensor to monitor environmental temperature.

* **Cloud Integration (ThingsBoard)**:
  Periodically (every 5 seconds) sends weight and temperature data to the **ThingsBoard demo dashboard** for visualization and further analytics.

* **Alert System via Telegram**:

  * Sends current weight and temperature values when the **button is pressed**.
  * Automatically sends an **ALARM message** if any load cell’s weight exceeds a predefined threshold.

* **Wi-Fi Connectivity**:
  ESP32 connects to Wi-Fi to enable cloud communication and Telegram messaging.

### 🔹 Typical Workflow

1. On startup, the ESP32 initializes the button, load cells, temperature sensor, and Wi-Fi connection.
2. Each load cell is calibrated with its unique calibration factor and offset.
3. The system continuously reads weights and temperature.
4. Data is:

   * Printed to the Serial Monitor for local debugging.
   * Sent to **ThingsBoard** every 5 seconds.
   * Sent to **Telegram** when the button is pressed.
   * Sent to **Telegram** as an **alarm** if any weight exceeds the threshold.

### 🔹 Applications

* Smart weighing systems
* Environmental/industrial monitoring
* Automated alert systems for IoT devices
* Educational IoT/embedded system projects

---

# ภาพรวมการทำงาน

ระบบเป็น ESP32 ที่อ่านค่าน้ำหนักจาก **Loadcell 5 ตัวผ่าน HX711**, อ่านอุณหภูมิจาก **DS18B20**, จากนั้น

* ส่งข้อมูลขึ้น **ThingsBoard (Dashboard)** ทุก ๆ 5 วินาที
* ส่งข้อความไป **Telegram** เมื่อกดปุ่ม
* และส่ง **Alarm** ไป Telegram ถ้าน้ำหนักตัวใดตัวหนึ่งเกิน **Threshold**

ทุกอย่างทำงานอยู่ในลูปหลัก (loop) ตลอดเวลา

---

# ไล่ทีละบล็อกตาม PlantUML

## 1) Setup()

ขั้นตอนเตรียมระบบก่อนเข้าลูป:

* เปิดสื่อสาร Serial (สำหรับดูค่าหน้างานและดีบั๊ก)
* ตั้งค่าและเริ่มต้นโมดูล/เซ็นเซอร์ต่าง ๆ: HX711 (5 ตัว), DS18B20
* เชื่อมต่อ Wi-Fi
* เชื่อมต่อหรือเตรียมข้อมูลสำหรับ Dashboard (ThingsBoard)

### การแมปกับโค้ด

* `Serial.begin(115200);` → เปิด Serial Monitor
* ปุ่ม: `pinMode(BUTTON_PIN, INPUT_PULLUP);` (ปุ่มถูกกดจะอ่านค่าเป็น **LOW**)
* HX711:

  ```cpp
  scaleX.begin(LOADCELL_DOUTx, LOADCELL_SCKx);
  scaleX.set_scale(CALIBRATION_FACTORx);
  scaleX.set_offset(OFFSETx);
  ```

  ทำครบทั้ง 5 ตัว (`scale1` ถึง `scale5`)
* DS18B20: `sensors.begin();`
* Wi-Fi: วนรอ `WL_CONNECTED` แล้วพิมพ์ “Connected to WiFi”

> หมายเหตุสำคัญ: บล็อก **“Set Calibration and Offset for 5 Loadcells”** สะท้อนกับคอนสแตนต์ในโค้ด
>
> * `CALIBRATION_FACTOR1..5`
> * `OFFSET1..5`
>   ที่ถูกตั้งผ่าน `set_scale()` และ `set_offset()` ของ HX711
>   การคาลิเบรทแต่ละตัว **ต้องทำแยกกัน** เพราะโหลดเซลล์/บอร์ด/สภาพการติดตั้งต่างกัน

---

## 2) Read weights from 5 Loadcells

ในลูปหลัก ระบบจะอ่านค่าน้ำหนักจากโหลดเซลล์ทั้ง 5 ตัว (ผ่าน HX711) ทีละตัว แล้วได้ค่าเป็นกิโลกรัม

### การแมปกับโค้ด

```cpp
float w1 = scale1.is_ready() ? get_units_kg(scale1, CALIBRATION_FACTOR1, OFFSET1) : 0;
// … w2..w5 ลักษณะเดียวกัน
```

* ใช้ `is_ready()` เช็กก่อนอ่าน เพื่อลดโอกาสอ่านค่าว่าง/ไม่พร้อม
* ฟังก์ชัน `get_units_kg(...)` คือที่แปลงค่าดิบเป็นกิโลกรัม โดยอ้างอิง factor/offset (ในโค้ดคุณเรียกใช้แล้ว)

---

## 3) Read temperature from DS18B20

อ่านอุณหภูมิ (องศาเซลเซียส) เพื่อนำไปแสดงและส่งออก

### การแมปกับโค้ด

```cpp
sensors.requestTemperatures();
float tempC = sensors.getTempCByIndex(0);
```

---

## 4) Print to Serial for monitoring & Debug

พิมพ์ค่าทั้งหมดออก Serial เพื่อดูเรียลไทม์และช่วยดีบั๊ก

### การแมปกับโค้ด

```cpp
Serial.printf("Weights: %.2f, %.2f, %.2f, %.2f, %.2f | Temp: %.2f °C\n",
              w1, w2, w3, w4, w5, tempC);
```

---

## 5) (Decision) 5 seconds passed? → Yes/No

ถ้าครบทุก **5 วินาที** ให้ส่งเทเลเมทรีขึ้น ThingsBoard

### การแมปกับโค้ด

```cpp
const long intervalTB = 5000; // 5 seconds
if (millis() - lastSendTB >= intervalTB) {
  lastSendTB = millis();
  sendToThingsBoard(w1, w2, w3, w4, w5, tempC);
}
```

* ใช้ตัวจับเวลา `millis()` คู่กับ `lastSendTB` เพื่อเว้นช่วงส่งทุก 5 วินาที
* ค่านี้ตรงกับคำว่า **“5 seconds passed?”** ในแผนภาพ

---

## 6) (Decision) Button pressed? → Yes/No

ถ้าปุ่มถูกกด ให้ส่ง “น้ำหนักทั้ง 5 + อุณหภูมิ” ไป Telegram แล้วดีเลย์ 1 วินาทีเพื่อตัดเด้ง (debounce)

### การแมปกับโค้ด

```cpp
if (digitalRead(BUTTON_PIN) == LOW) { // ปุ่มต่อแบบ INPUT_PULLUP → กด = LOW
  sendToTelegram(w1, w2, w3, w4, w5, tempC);
  delay(1000); // debounce
}
```

* `INPUT_PULLUP` ทำให้สถานะปุ่มเป็น **ปกติ HIGH**, เมื่อกดจะ **LOW**
* `delay(1000)` คือ **Delay 1s (debounce)** ตามแผนภาพ

---

## 7) (Decision) Any weight >= Threshold? → Yes/No

ถ้าค่าน้ำหนัก **ตัวใดตัวหนึ่ง** เกิน Threshold ให้ส่ง **ALARM** ไป Telegram แล้วดีเลย์ 1 วินาที

### การแมปกับโค้ด

```cpp
float value_theshold = 1.50; // kg
if (w1 >= value_theshold || w2 >= value_theshold || w3 >= value_theshold || w4 >= value_theshold) {
  sendToTelegram(w1, w2, w3, w4, w5, tempC);
  Serial.println("ALARM!");
  delay(1000);
}
```

> **ข้อสังเกตสำคัญ:** ในโค้ดจริงคุณเช็ก **แค่ w1..w4** ยัง **ไม่เช็ก w5**
> แต่ใน PlantUML เขียนว่า *“Any weight”* (สื่อถึงทั้ง 5 ตัว)
> ถ้าต้องการให้ตรงกัน ควรแก้เป็น
>
> ```cpp
> if (w1 >= th || w2 >= th || w3 >= th || w4 >= th || w5 >= th) { ... }
> ```

---

## 8) Loop (ทำงานวนไปเรื่อย ๆ)

ทำขั้นตอนอ่านค่า → ตรวจเงื่อนไข → ส่งข้อมูล วนซ้ำตลอดเวลา

---

# ภาพรวมการเชื่อมต่อ/บริการภายนอก

* **Wi-Fi:** ใช้ `WiFi.begin(SSID, PASSWORD)` และรอจนเชื่อมต่อสำเร็จ
* **ThingsBoard:** ใช้ token อุปกรณ์ (ในโค้ดกำหนด `tb_host`, `tb_token`) แล้วส่งเทเลเมทรีทุก 5 วินาทีผ่านฟังก์ชัน `sendToThingsBoard(...)`
  *(แผนภาพใช้ข้อความ “Connect to Dashboard (ThingsBoard Demo)” เพื่อสื่อว่าใช้แดชบอร์ดสาธิต/ทดสอบ)*
* **Telegram:** ใช้ `BOT_TOKEN` และ `CHAT_ID` เพื่อส่งข้อความเมื่อปุ่มถูกกดหรือเมื่อเกิด Alarm ผ่าน `sendToTelegram(...)`

---

# คำแนะนำเสริมให้ระบบนิ่งขึ้น 

* **ตรวจ w5 ในเงื่อนไข Alarm** (อย่างที่ชี้ไว้ด้านบน) เพื่อให้สอดคล้องกับคำว่า “Any weight”
* **ปุ่มกดกับ INPUT\_PULLUP:** ถ้าเจอการเด้งบ่อย อาจเพิ่มซอฟต์แวร์ดีบาวซ์แบบอ่านซ้ำหลายครั้ง/เฉลี่ยเวลา แทน `delay(1000)` เพื่อลดการหน่วงลูป
* **Wi-Fi Reconnect:** ถ้า Wi-Fi หลุด ควรมีลอจิก reconnect (ตอนนี้เชื่อมต่อใน `setup()` ครั้งเดียว)
* **ความปลอดภัยของ Token:** ควรหลีกเลี่ยงการฮาร์ดโค้ด BOT\_TOKEN/CHAT\_ID/tb\_token ในซอร์สที่จะเผยแพร่
* **HX711 is\_ready():** ดีแล้วที่เช็กก่อนอ่าน แต่ถ้าพบอ่านค่าเป็นศูนย์บ่อย อาจเพิ่ม retry/เฉลี่ยค่า
* **หน่วย Threshold:** `value_theshold = 1.50` คือกิโลกรัม—ตรวจให้แน่ใจว่าสเกล/ปัจจัยคาลิเบรตสอดคล้องกับหน่วยนี้

---
