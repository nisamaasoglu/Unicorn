#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <LoRa.h>
#include <ESP32Servo.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define LAZER_PIN 4 //

#define LORA_SS_PIN 5
#define LORA_RST_PIN 14
#define LORA_DIO0_PIN 26

#define LED_YESIL_PIN 16   
#define LED_KIRMIZI_PIN 2  
#define BUZZER_PIN 17      
#define MIC_PIN 34 //

#define TRIG_PIN 32 // Ses dalgasını ateşleyen pin
#define ECHO_PIN 33 // Yansıyan sesi duyan pin

#define BACAK_SOL_ON_PIN 13
#define BACAK_SAG_ON_PIN 12
#define BACAK_SOL_ARKA_PIN 27
#define BACAK_SAG_ARKA_PIN 33
#define KAFA_PIN 32
#define KUYRUK_PIN 25

Adafruit_MPU6050 mpu; // Sensör nesnemizi yarattık
// Robotun içinde bulunabileceği 3 ana durumu tanımlıyoruz
enum RobotDurumu {
  DURUM_NORMAL,       // Güneşlenme, konum takibi
  DURUM_DEPREM,       // Deprem! Hedefe git, alarm çal
  DURUM_HAYATTA_KALMA // LoRa iletisimi, durum tespiti
};

RobotDurumu mevcutDurum = DURUM_NORMAL; // Robot uyandığında normal moddadır

// --- Eşik Değerleri ve Zamanlayıcılar ---
const float DEPREM_ESIGI = 3.5; 
unsigned long sonKonumGuncelleme = 0;
const unsigned long GUNCELLEME_SURESI = 600000; // 10 dakika (milisaniye cinsinden)

// ==========================================
// --- FONKSIYON PROTOTIPLERI (C++ KURALI) ---
void normalModCalistir();
void depremModCalistir();
void hayattaKalmaModCalistir();
float ivmeOlcerOku();
void konumGuncelle();
String sesDinle();
String kodBelirle(String ses);
void loraGonder(String durumKodu);

void ayagaKalk();
void ileriYuru();

// --- Bluetooth (Koku Alma) Ayarları ---
// Pazartesi buraya senin telefonunun GERÇEK Bluetooth adresini yazacağız
String hedefTelefonMAC = "aa:bb:cc:dd:ee:ff"; 
int sonSinyalGucu = -100; // Sinyal gücü (Sıfıra ne kadar yakınsa sen o kadar yakındasın demektir)

int mesafeOlc();

// Servo nesnelerimizi oluşturuyoruz
Servo bacakSolOn;
Servo bacakSagOn;
Servo bacakSolArka;
Servo bacakSagArka;
Servo kafa;
Servo kuyruk;





void setup() {
  Serial.begin(115200);
  Serial.println("Unicorn Sistemi Baslatiliyor...");
  
  // MPU6050'yi Başlat
  Serial.println("Sismik algilayici (MPU6050) araniyor...");
  if (!mpu.begin()) {
    Serial.println("HATA: MPU6050 bulunamadi! Kablolari kontrol et.");
    while (1) {
      delay(10); // Sensör yoksa sistemi durdur, sonsuz döngü
    }
  }
  Serial.println("MPU6050 Hazir. Titresimler dinleniyor...");
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // Hassasiyet ayarı
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   // Ufak gürültüleri filtrele

  // --- LoRa Modülünü Başlat ---
  Serial.println("LoRa Haberlesme Agi kuruluyor...");
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
  
  // Türkiye/Avrupa için LoRa frekansı genelde 433 MHz'dir (433E6)
  if (!LoRa.begin(433E6)) {
    Serial.println("HATA: LoRa baslatilamadi! Baglantilari kontrol et.");
    while (1); // Hata varsa sistemi durdur
  }
  Serial.println("LoRa Agi Aktif! Acil durum sinyalleri icin hazir.");

  // --- HC-SR04 Kurulumu ---
  pinMode(TRIG_PIN, OUTPUT); // Ses gönderecek
  pinMode(ECHO_PIN, INPUT);  // Ses dinleyecek
  Serial.println("Otopilot (HC-SR04) Aktif.");

  // --- Servo Motor (Kas Sistemi) Kurulumu ---
  Serial.println("Kas sistemi baslatiliyor...");
  // ESP32Servo kütüphanesi için zamanlayıcı (timer) ayarı
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Servoları pinlere bağla (Min ve Max PWM sinyal aralıklarıyla)
  bacakSolOn.attach(BACAK_SOL_ON_PIN, 500, 2400);
  bacakSagOn.attach(BACAK_SAG_ON_PIN, 500, 2400);
  bacakSolArka.attach(BACAK_SOL_ARKA_PIN, 500, 2400);
  bacakSagArka.attach(BACAK_SAG_ARKA_PIN, 500, 2400);
  kafa.attach(KAFA_PIN, 500, 2400);
  kuyruk.attach(KUYRUK_PIN, 500, 2400);

  // Başlangıç Pozisyonu: Unicorn ayağa kalksın ve kafayı karşıya baksın (90 derece)
  ayagaKalk();

  // --- Lazer ve Radar Kurulumu ---
  pinMode(LAZER_PIN, OUTPUT);
  digitalWrite(LAZER_PIN, LOW); // Normalde lazer kapalı
  
  BLEDevice::init("Unicorn_Radar"); // Bluetooth çipini uyandır
  Serial.println("Bluetooth Radari Aktif.");
}

void loop() {
  // Robot sürekli olarak "Ben şu an hangi durumdayım?" diye kontrol eder
  switch (mevcutDurum) {
    case DURUM_NORMAL:
      normalModCalistir();
      break;
    
    case DURUM_DEPREM:
      depremModCalistir();
      break;

    case DURUM_HAYATTA_KALMA:
      hayattaKalmaModCalistir();
      break;
  }
}

// ==========================================
// --- DURUM FONKSİYONLARI (Mantık Akışı) ---
// ==========================================

void normalModCalistir() {
  // 1. Deprem var mı dinle? (MPU6050 okuması)
  float sarsintiSiddeti = ivmeOlcerOku(); 
  if (sarsintiSiddeti >= DEPREM_ESIGI) {
    mevcutDurum = DURUM_DEPREM; // Eşik aşıldı! Modu hemen değiştir!
    return; // Normal moddan çık
  }

  // 2. Telefon konumunu güncelle (Sadece 10 dakikada bir)
  if (millis() - sonKonumGuncelleme >= GUNCELLEME_SURESI) {
    konumGuncelle();
    sonKonumGuncelleme = millis();
  }
}

void depremModCalistir() {
  Serial.println("DEPREM ALGILANDI! Arama Kurtarma Basliyor...");
  digitalWrite(LED_YESIL_PIN, LOW);   
  digitalWrite(LED_KIRMIZI_PIN, HIGH); 
  digitalWrite(BUZZER_PIN, HIGH); 
  
  digitalWrite(LAZER_PIN, HIGH);
  // --- YENİ EKLENEN: Otopilot İlerleyişi ---
  int onumdekiEngel = mesafeOlc(); // Bak bakalım önde ne var?
  Serial.print("Hedefe Gidiliyor. Onumdeki Engel Mesafesi: ");
  Serial.print(onumdekiEngel);
  Serial.println(" cm");

  if (onumdekiEngel < 15) {
      // Engel 15 cm'den yakınsa DUR veya ETRAFINDAN DOLAN
      Serial.println("DİKKAT: ENGEL! Motorlar durduruluyor, yon degistiriliyor...");
      // motorlariDurdur();
      // sagaDon();
  } else {
      // Yol açıksa sana doğru koşmaya devam et
      Serial.println("Yol acik, ilerliyorum...");
      // ileriGit();
  }
  
  delay(5000); 
  digitalWrite(BUZZER_PIN, LOW); 
  mevcutDurum = DURUM_HAYATTA_KALMA; 
}

void hayattaKalmaModCalistir() {
  // 1. Ortamı dinle (Senden ses geliyor mu?)
  String ortamSesi = sesDinle();

  // 2. Durum kodunu belirle (11, 10, 01, 00)
  String durumKodu = kodBelirle(ortamSesi);

  // 3. LoRa ile yukarıya mesajı ateşle!
  loraGonder(durumKodu);

  // 4. Yukarıdan "Onay" bekle (Bunu daha sonra yazacağız)
  delay(10000); // Şimdilik 10 saniyede bir göndersin ki sistemi boğmasın
}

// --- İçi Doldurulacak Alt Fonksiyonlar (Şimdilik Boş) ---
// Sahte veriyi silip bu gerçek fonksiyonu yapıştır
float ivmeOlcerOku() { 
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp); // Sensörden verileri çek

  // X, Y ve Z eksenlerindeki ivmeyi birleştirerek "Toplam Sarsıntı Vektörünü" buluyoruz
  // Pisagor teoreminin 3 boyutlu hali: sqrt(x^2 + y^2 + z^2)
  float toplamIvme = sqrt(pow(a.acceleration.x, 2) + 
                          pow(a.acceleration.y, 2) + 
                          pow(a.acceleration.z, 2));

  // Normal durumda yerçekimi (g) zaten ~9.8 m/s^2'dir. 
  // Biz sadece "ekstra" sarsıntıyı (depremi) istiyoruz.
  float netSarsinti = abs(toplamIvme - 9.8); 

  // Eğer sarsıntı ufak tefek masaya vurmalardan büyükse, log'a yazdır
  if(netSarsinti > 1.0) {
      Serial.print("Anlik Sarsinti Siddeti: ");
      Serial.println(netSarsinti);
  }

  return netSarsinti; 
}


void konumGuncelle() {
  Serial.println("Av kopegi modu: Sahibin kokusu (Bluetooth) araniyor...");
  
  // Radarı çalıştır ve 3 saniye boyunca havadaki tüm sinyalleri dinle
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  BLEScanResults foundDevices = pBLEScan->start(3); // 3 saniyelik tarama

  bool sahipBulundu = false;
  
  // Havada bulduğu cihazların hepsine tek tek bak
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    
    // Eğer bulduğu cihazın adresi, senin telefonunun adresiyle eşleşirse:
    if (device.getAddress().toString() == hedefTelefonMAC.c_str()) {
      sonSinyalGucu = device.getRSSI(); // Sinyal gücünü (mesafeyi) kaydet
      sahipBulundu = true;
      
      Serial.print("SAHIP BULUNDU! Sinyal Gucu (Yakinlik Derecesi): ");
      Serial.println(sonSinyalGucu);
      break; // Seni bulduğu için diğer cihazlara bakmayı bırakır
    }
  }
  
  if(!sahipBulundu) {
     Serial.println("Sahip menzil disinda veya telefonun Bluetooth'u kapali.");
  }
  
  pBLEScan->clearResults(); // Sonraki tarama için hafızayı temizle
}
String sesDinle() {
  // ESP32 analog pinleri 0 ile 4095 arasında değer okur.
  int sesSeviyesi = analogRead(MIC_PIN); 
  
  // Test için ekrana yazdıralım ki Pazartesi hassasiyet ayarı yapabilelim
  Serial.print("Anlik Ses Seviyesi: ");
  Serial.println(sesSeviyesi);

  // Eğer ortamdaki ses belirli bir eşiği geçiyorsa (Sen inliyorsan/bağırıyorsan)
  // (2000 değeri örnektir, odanın gürültüsüne göre Pazartesi ayarlayacağız)
  if (sesSeviyesi > 2000) {
    return "SES_ALINDI";
  } else {
    return "SESSİZLİK";
  }
}

String kodBelirle(String ortamSesi) {
  if (ortamSesi == "SES_ALINDI") {
    return "01"; // Acil durum: Yaralı var, ses veriyor.
  } else {
    return "00"; // Kritik durum: Ses yok, bilinç kapalı olabilir.
  }
}


void loraGonder(String durumKodu) {
  Serial.print("LORA ILE MERKEZE GONDERILIYOR -> Durum Kodu: ");
  Serial.println(durumKodu);

  // LoRa veri paketini oluştur ve havaya gönder
  LoRa.beginPacket();
  LoRa.print("UNICORN_ACIL_DURUM:"); // Paketin kimden geldiği belli olsun
  LoRa.print(durumKodu);
  LoRa.endPacket();

  Serial.println("Mesaj basariyla iletildi!");
}

int mesafeOlc() {
  // 1. Sensörü temizle ve hazırla
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // 2. 10 mikrosaniyelik bir ses dalgası fırlat (TETİKLE)
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 3. Sesin gidip çarpıp geri dönme süresini (mikrosaniye olarak) ölç
  long sure = pulseIn(ECHO_PIN, HIGH);
  
  // 4. Süreyi santimetreye çevir (Ses hızı hesabı)
  int santimetre = sure * 0.034 / 2;
  
  return santimetre;
}

void ayagaKalk() {
  Serial.println("Unicorn ayaga kalkti, hazir olta bekliyor.");
  // Tüm motorları orta noktaya (90 derece) getir
  bacakSolOn.write(90);
  bacakSagOn.write(90);
  bacakSolArka.write(90);
  bacakSagArka.write(90);
  kafa.write(90);   // Kafa dümdüz ileri bakıyor
  kuyruk.write(90); // Kuyruk ortada
}

void ileriYuru() {
  // Çapraz bacak yürüyüşü (Trot Gait) - Basit versiyon
  // 1. Adım
  bacakSolOn.write(120);
  bacakSagArka.write(120);
  bacakSagOn.write(60);
  bacakSolArka.write(60);
  delay(300);
  
  // 2. Adım
  bacakSolOn.write(60);
  bacakSagArka.write(60);
  bacakSagOn.write(120);
  bacakSolArka.write(120);
  delay(300);
}