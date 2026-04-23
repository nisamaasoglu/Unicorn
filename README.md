# 🦄 Unicorn - Otonom Arama & Kurtarma Robotu

![ESP32](https://img.shields.io/badge/Mikrodenetleyici-ESP32-red)
![Framework](https://img.shields.io/badge/Framework-Arduino_C++-blue)
![Platform](https://img.shields.io/badge/Geliştirme_Ortamı-PlatformIO-orange)
![Durum](https://img.shields.io/badge/Durum-Geliştirme_Aşamasında-brightgreen)

Unicorn, afet (özellikle deprem) anlarında otonom olarak hayatta kalma ve arama-kurtarma protokollerini devreye sokan, 4 bacaklı (Quadruped) akıllı bir robotik yoldaştır. 

Sistem; internetin, GPS'in ve baz istasyonlarının çöktüğü senaryolarda bile sahibinin yerini bulmak, hayati durumunu analiz etmek ve kilometrelerce öteye radyo frekansları üzerinden acil durum sinyali göndermek üzere tasarlanmıştır.

---

## 🧠 Donanım Anatomisi ve Sensör Ağı

Unicorn, biyolojik canlıların reflekslerini taklit eden gelişmiş bir sensör ağına sahiptir:

* **Beyin (Ana Kontrolcü):** `ESP32` - Çift çekirdekli, yüksek hızlı işlem kapasitesi.
* **İç Kulak (Deprem/Sarsıntı Algısı):** `MPU6050` (6 Eksenli İvmeölçer ve Jiroskop) - Yerçekimi ivmesini referans alarak sismik hareketleri ölçer.
* **Yarasa Gözler (Otopilot):** `HC-SR04` (Ultrasonik Mesafe Sensörü) - Hedefe giderken çarpışmaları önler ve rotayı ayarlar.
* **Av Köpeği Burnu (Kapalı Alan Takip):** `ESP32 Dahili BLE` (Bluetooth Low Energy) - Sahibinin telefon MAC adresini tarayarak sinyal gücü (RSSI) üzerinden mesafe tahmini yapar.
* **Hassas Kulaklar (Hayati Belirti Dinleme):** `MAX9814` (AGC'li Mikrofon) - Otomatik kazanç kontrolü sayesinde ortam gürültüsünü filtreler ve enkaz altındaki inleme/fısıltı seslerini algılar.
* **Ses Telleri (Uzak Mesafe İletişim):** `LoRa SX1278` (433 MHz) - Altyapısız, düşük frekanslı, kilometrelerce menzile sahip RF haberleşmesi sağlar.
* **Kriz Refleksleri:** `RGB LED (Gözler)`, `Aktif Buzzer (Siren)` ve `SOS Lazer Modülü` - Toz duman içinde görsel ve işitsel işaret fişeği görevi görür.
* **Kas Sistemi:** `6 Adet Servo Motor` - 4 bacak, 1 tarayıcı kafa ve 1 duygu ifade eden kuyruk mekanizması.

---

## ⚙️ Yazılım Mimarisi: Durum Makinesi (State Machine)

Robotun davranışları 3 ana "Durum" (State) üzerinden yönetilir:

### 1. DURUM_NORMAL (Bekleme ve İzleme)
* Robot uyku halindedir, gözleri yeşil yanar.
* `MPU6050` sürekli olarak ortamdaki titreşimleri dinler.
* `BLE Scanner` periyodik olarak ortamı koklar ve sahibinin telefonunun "Son Bilinen Konumunu (Sinyal Gücünü)" hafızasına yazar.

### 2. DURUM_DEPREM (Kriz Refleksi ve Otopilot)
* İvmeölçer eşik değerini aştığında tetiklenir.
* Gözler kırmızıya döner, **Siren** ve **SOS Lazeri** aktifleşir.
* Robot ayağa kalkar, `HC-SR04` ile engelleri tarayarak son bilinen konuma doğru (sahibine) otonom yürüyüşe geçer.

### 3. DURUM_HAYATTA_KALMA (İletişim ve Analiz)
* Hedefe ulaştığında sistemi dinlemeye alır.
* `MAX9814` mikrofonu ortamdaki sesleri ölçer.
* Sahibinden ses gelip gelmediğine göre bir durum kodu (Örn: `01` - Yaralı/Ses Var veya `00` - Tepkisiz) oluşturur.
* Bu veri paketi, `LoRa` üzerinden kilometrelerce ötedeki komuta merkezine fırlatılır.

---

## 🛠️ Kurulum ve Geliştirme (Geliştiriciler İçin)

Bu proje **VS Code** ve **PlatformIO** ortamında geliştirilmiştir. Standart Arduino IDE yerine PlatformIO kullanılması, gelişmiş kütüphane yönetimi ve bellek optimizasyonu sağlar.

### Gereksinimler
1. [VS Code](https://code.visualstudio.com/) kurun.
2. VS Code eklentilerinden **PlatformIO IDE**'yi yükleyin.
3. Repoyu bilgisayarınıza klonlayın:
   ```bash
   git clone [https://github.com/KULLANICI_ADIN/Unicorn_Otonom_Sistem.git](https://github.com/KULLANICI_ADIN/Unicorn_Otonom_Sistem.git)
