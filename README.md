# Enviromental-Monitoring-System 

Sistem Monitoring lingkungan menggunakan ESP32 untuk mengukur dan memonitor air quality index dan suhu secara real time, Data tersebut nantinya akan digunakan untuk memberikan warning jika kualitas udara atau suhu berada pada kondisi yang buruk. Sistem akan mengimplementasikan MQTT dan Blynk untuk integrasi dan visualiassi, dan menggunakan LCD untuk mengdisplaynya. Proyek ini akan menggunakan ESP32, Sensor AQI untuk memantau kualitas udara, sensor suhu untuk memantau temperatur, lcd untuk mendisplay nilai yang didapat dari sensor, dan aktuator  yang akan aktif ketika terdapat kondisi yang tidak baik baik saja

**Anggota Kelompok 17:**
+ FABSESYA MUHAMMAD PI  - 2206829433
+ IRFAN YUSUF KHAERULLAH  - 2206813290
+ SHARIF FATIH ASAD MASYHUR  - 2206063014
+ RAJA YONANDRO RUSLITO  - 2206059553

## Introduction to the problem and the solution

Kondisi lingkungan yang buruk, seperti kualitas udara dan suhu ekstrem, dapat mempengaruhi kesehatan manusia dan ekosistem. Namun, pemantauan lingkungan real-time masih terbatas. Diperlukan sistem berbasis IoT yang dapat memberikan data lingkungan secara real-time, terintegrasi dengan platform visualisasi, dan memberikan peringatan dini untuk membantu mitigasi risiko. Solusi ini akan mempercepat pengambilan keputusan dan melindungi kesehatan serta keselamatan, dengan menggabungkan teknologi IoT, MQTT, dan platform seperti Blynk.

## Hardware design and implementation details

Desain hardware kami mencakup ESP32, sensor suhu dan kelembapan DHT11, sensor gas MQ135, sensor indeks kualitas udara (AQI), layar LCD, motor servo, indikator LED, dan kabel jumper. Koneksi antar komponen menggunakan berbagai protokol komunikasi untuk integrasi yang efisien. Berikut adalah perangkat yang kami gunakan dalam proyek ini:

- ESP32

![picture 0](https://i.imgur.com/xJ4aq6v.png)  



- Sensor DHT11

![picture 1](https://i.imgur.com/azbwrfJ.png)  



- Sensor MQ135 

![picture 2](https://i.imgur.com/v2TjDRQ.png)  



- LCD Display (I2C)

![picture 3](https://i.imgur.com/vgo93RD.png)  


- Motor Servo

![picture 4](https://i.imgur.com/kctNlfE.png)  



- LED Indicators

![picture 5](https://i.imgur.com/Xb8NEcS.png)  



- Jumper Cables

![picture 7](https://i.imgur.com/CT9NbIF.png)  


- Protoboard

![picture 8](https://i.imgur.com/e9lBlX7.png)  


- USB Power Supply

![picture 9](https://i.imgur.com/I9qOpdy.png)  



Sensor DHT11 terhubung ke ESP32 untuk transmisi data, sementara sensor MQ135 dan AQI yang bersifat analog terhubung ke pin ADC di ESP32 untuk pemantauan kualitas udara. Kedua sensor ini mengukur tingkat polutan dan berkontribusi dalam perhitungan Air Quality Index (AQI). Layar LCD menggunakan antarmuka komunikasi I2C, dengan pin SDA dan SCL terhubung ke GPIO pinout pada ESP32 yang bertugas mengontrol atau membaca status pin tertentu yang menangani sinyal digital masuk dan keluar.

ESP32 berfungsi sebagai unit pemrosesan pusat yang mengumpulkan dan memproses data dari sensor. Sistem ini dirancang untuk memantau kualitas udara dan suhu secara real-time. Jika AQI atau suhu melebihi ambang batas yang telah ditentukan, ESP32 memicu peringatan visual dan suara.

Motor servo terhubung ke pin PWM GPIO di ESP32 dan dapat digunakan untuk mengubah sinyal kontrol menjadi rotasi atau kecepatan sudut poros motor. Indikator LED terhubung ke pin output digital untuk memberikan umpan balik visual tentang status kualitas udara secara real-time.

Terakhir, sistem menggunakan protokol MQTT untuk mengirim data real-time ke cloud, memungkinkan visualisasi dan kontrol melalui aplikasi Blynk. ESP32 diberi daya melalui koneksi USB, sementara semua komponen terintegrasi pada protoboard untuk organisasi dan koneksi bersama.

## Software implementation details

Pada Software Implementation proyek IoT kami, digunakan satu kode .ino yang include program yang perlu untuk proyeknya. Pada bagian ini, akan dicoba jelaskan bagian yang penting pada program.

```c
#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include "freertos/semphr.h"
#include "time.h"
#include <DHT.h>  
#include <ESP32Servo.h>
```
Ini merupakan library yang digunakan pada program. DHT.h itu untuk membaca sensor DHT11 (suhu dan kelembapan), LiquidCrystal untuk mengontrol layar LCD, dan Blynk serta WiFi digunakan untuk integrasi platform Blynk. ESP32Servo untuk mengontrol servo motor. 

```c
void sendEnvDataToBlynk()
{
  if(xSemaphoreTake(xMutex, portMAX_DELAY)) {
    printLocalTime();
    Serial.println("\tMutex taken");

    // Read data from DHT22 sensor
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    gasLevel = analogRead(MQ135_PIN);
    gasLevel = map(gasLevel, 0, 4095, 0, 500); 

    // Check if the sensor readings are valid
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Clamp AQI and temperature to valid ranges
    gasLevel = max(0.0f, min(gasLevel, 100.0f));  // Same for temperature
    temperature = max(-10.0f, min(temperature, 50.0f));  // Same for temperature
    humidity = max(0.0f, min(humidity, 100.0f));  // Clamp humidity

    printLocalTime();
    Serial.printf("\tGas: %.2f\tTemperature: %.2f°C\tHumidity: %.2f%%\n", gasLevel, temperature, humidity);

    saveEnvDataToEEPROM();

    // Write to Blynk
    Blynk.virtualWrite(V0, gasLevel);
    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, humidity);

    // Write to LCD Display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gas: ");
    lcd.print(gasLevel, 2);
    lcd.print(" ppm");

    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(temperature, 2);
    lcd.print(" C");

    lcd.setCursor(0, 2);
    lcd.print("Humidity: ");
    lcd.print(humidity, 2);
    lcd.print(" %");

    
    // Not Good Condition

    if (gasLevel > 50) {
      Blynk.logEvent("gas_alert", "Gas Leakage Detected");
      digitalWrite(ledPin, HIGH);  /
      printLocalTime();
      Serial.println("\tALERT! AIR QUALITY IS POOR - Servo activated");
    } 
    else {
      digitalWrite(ledPin, LOW);  /
      printLocalTime();
      Serial.println("\tAir quality is GOOD - Servo deactivated");
    }

    if (temperature > 40) {
      Blynk.logEvent("temperature_alert", "Overheat Detected");
      myservo.write(90); 
      printLocalTime();
      Serial.println("\tALERT! Temperature is too HIGH!");
      delay(500); 
      myservo.write(0);
      delay(500);
    } 
    else {
      myservo.write(0); 
      printLocalTime();
      Serial.println("\tTemperature is within normal range.");
    }

    

    xSemaphoreGive(xMutex);
    printLocalTime();
    Serial.println("\tMutex given");
    Serial.println("---------------------------------");
  }
}
```
Fungsi diatas untuk membaca data dari sensor dan mengirimnya ke Blynk serta menampilkan di LCD. Ini juga mengaktifkan LED atau servo berdasarkan kondisinya. Informasi juga ditampilkan di LCD, serta disimpan di EEPROM untuk keperluan persistensi. Fungsi ini juga mengatur kondisi peringatan, seperti menyalakan LED jika kualitas udara buruk atau menggerakkan servo jika suhu terlalu tinggi.

Pada loop itu menjalankan Blynk, MQTT, dan timer secara terus-menerus. MQTT digunakan untuk mengirim pesan ke broker atau menerima perintah dari aplikasi lain. Jika koneksi MQTT terputus, perangkat akan mencoba untuk terhubung kembali. Hal ini menjadikan sistem mampu memantau kondisi lingkungan, memberikan notifikasi real-time, dan bereaksi terhadap kondisi tertentu secara otomatis.



## Test results and performance evaluation

Pada tahap pengujian proyek IoT kami, dilakukan beberapa uji untuk memastikan bahwa program dan sirkuit beroperasi sesuai dengan kriteria dan harapan yang telah ditentukan. Pada aplikasi Blynk 2.0, ESP32 mencoba untuk terhubung ke jaringan WiFi menggunakan SSID dan password yang diberikan. Kemudian, layar LCD dan servo menyala dan menampilkan nilai Suhu, Kelembapan, Gas, dan IAQ (Indoor Air Quality).

LED dan servo akan aktif dalam kondisi tertentu. Di aplikasi Blynk, nilai Suhu, Kelembapan, dan Gas akan ditampilkan, beserta informasi mengenai kondisi kualitas udara berdasarkan nilai IAQ yang diperoleh.

Hasil dari ketiga pengujian yang dilakukan pada rangkaian asli menggunakan platform Blynk sesuai dengan harapan dan kriteria yang telah ditetapkan. Selama pengujian ini, semua fungsi dan respons sistem dievaluasi untuk memastikan bahwa sirkuit beroperasi secara optimal dan terintegrasi dengan Blynk secara real-time. Selain itu, sistem juga mencakup indikator LED yang menyala sesuai dengan kondisi IAQ dan buzzer yang memberikan peringatan saat kualitas udara dalam ruangan memburuk.
![image](https://github.com/user-attachments/assets/45dadb4e-90fb-4e8a-a313-f535517cdbd5)
![image](https://github.com/user-attachments/assets/b7de47a8-e003-4a2d-a076-07f38cb51cf4)

## Conclusion and future work

Sistem pemantauan kualitas udara ini mengintegrasikan MQTT, sensor DHT11, MQ135, mikrokontroler ESP32, dan platform Blynk untuk menyediakan solusi komprehensif dalam memantau faktor lingkungan. Perangkat lunak dirancang untuk kinerja optimal dengan struktur modular, penanganan kesalahan, dan integrasi Blynk yang mulus, memungkinkan pengguna untuk melihat data, menerima peringatan, dan mempersonalisasi pemantauan mereka. Integrasi perangkat keras dan perangkat lunak yang sukses memastikan pengukuran yang akurat, dan pengujian serta kalibrasi yang ekstensif memvalidasi keandalan dan konsistensi sistem seiring waktu.

Untuk pekerjaan kami ke depan, kami ingin menemukan cara agar ESP32 memiliki sumber daya daya sendiri untuk berfungsi. Dengan cara ini, ESP32 tidak perlu terhubung ke PC atau laptop kami untuk menjalankan tugasnya dan dapat dibawa ke mana saja oleh pengguna jika diperlukan.
