# G870 RGB incelemesi — 17 Kasım 2026

## Sonuç ve yerel kanıt

Bu cihaz: **EXCALIBUR G870 / NLXB 001 / CQ171**. Kullanıcının gözlemi:
renk önce uygulanıyor, animasyon bazen birkaç Uygula tıklamasından sonra başlıyor;
0x07 seçeneğinde çoğunlukla sabit renk kalıyor. Loglar yalnızca EC dosyasına
başarılı yazmayı bildiriyor, firmware kabulünü veya LED yanıtını ölçmüyor.

Yerel sysfs envanterinde `644C5791-B7B0-4123-A90B-E93876E0DAAD` WMI GUID'i
var. Ancak `casper::kbd_backlight/led_control` veya `excalibur::kbd_backlight-*`
LED arabirimleri bulunmadı. GUID'in varlığı sürücünün yüklü olduğunu ya da bütün
komutların bu BIOS'ta çalışacağını kanıtlamaz. Seri numarası veya ACPI dökümü
rapora alınmadı; inceleme sırasında donanıma yeni komut gönderilmedi.

## Karşılaştırılan kaynaklar

| Kaynak | Bulgusu | Bu projeye etkisi |
|---|---|---|
| [Casper G870](https://www.casper.com.tr/excalibur-g870-p-137) ve [resmî sürücü seçimi](https://www.casper.com.tr/driver-indirme) | Ürün sayfası altı RGB modu tanıtıyor; Control Center modelin sürücü sayfasından ediniliyor. | Güncel ürün ailesinin tanıtımı CQ171 için komut belgesi değildir. Bu incelemede belirli bir Windows kurulum paketi indirilip tersine mühendislik yapılmadı. |
| [FeusX](https://github.com/FeusX/excalibur-control-center) | Projenin dayandığı doğrudan EC yaklaşımı. Yerel upstream geçmişinde renk kayıtları, latch ve mod kayıtları ayrı yazılıyor. | Yerel `44a0ac5` sürümü ile mevcut kod karşılaştırıldı. İnternet üzerindeki güncel klavye kaynak dosyası alınamadığı için güncel upstream ile birebir eşitlik iddia edilmiyor. |
| [Nextcalibur](https://github.com/efebedelcigil/nextcalibur-control-center), [protokol](https://github.com/efebedelcigil/nextcalibur-control-center/blob/main/docs/PROTOCOL.md) | Windows / JS970 / QQ141 referansı. WMI LED isteği renk, efekt ve parlaklığı birlikte taşıyor. 0x07 dalga olarak belgelenmiş; ekran örnekleme değil. | Farklı BIOS; doğrudan EC kayıtlarıyla aynı protokol sayılamaz. Bölge rengi ile ortak efekt/parlaklık kapsamları ayrılmalı. |
| [TUSECO sürücüsü](https://github.com/TUSECO/excalibur-control-center), [kaynak kod](https://raw.githubusercontent.com/TUSECO/excalibur-control-center/main/excalibur.c) | G870/CQ141 listeleniyor. WMI üzerinden bölge LED kontrolü; 0x06 rastgele, 0x07 gökkuşağı olarak tanımlanıyor. | Nextcalibur ile adlandırma/davranış farkı var. CQ171 üzerinde fiziksel doğrulama olmadan yeni efekt adları veya modlar kesin destek gibi eklenmemeli. |
| [Livicianaa paneli](https://github.com/Livicianaa/ArchLinux-Excalibur-Fan-RGB-control-panel) | NLXB/CQ121 üzerinde `casper-wmi` LED düğümünü kullanıyor; bazı BIOS'larda WMI fan okumalarının kilitlenmesini raporluyor. | Bu cihazla anakart ailesi benzer, BIOS farklı. RGB düzeltmek için fan sorgulama veya yeni fan kontrolü eklemek gerekmiyor. |
| [Mustafa Ekşi'nin sürücü önerisi v7](https://lists.openwall.net/linux-kernel/2024/08/06/1394), [ilk LED arabirimi](https://lists.openwall.net/linux-kernel/2024/02/21/1751) | WMI tabanlı Linux sürücüsü; farklı nesiller, LED ve güç profili arabirimleri ele alınıyor. İlk `led_control` ile sonraki multicolor API aynı değil. | Bir yama yayımlanmış olması mevcut çekirdekte bu arabirimin kurulu olduğu anlamına gelmez. Backend arabirim tespiti yapmalı. |

## Kod incelemesi ve yapılan düzeltmeler

1. `apply()` önce statik moda geçiyor, RGB/parlaklığı yazıp latch yapıyor,
   ardından animasyon komutu gönderiyor. Dolayısıyla önce seçilen rengin
   görünmesi kodun mevcut davranışı. Bunun birkaç tıklama gereksiniminin tek
   sebebi olduğu kanıtlanmadı. 50 ms bekleme firmware onayı değildir.
2. Bu işlem GTK ana iş parçacığında 30–130 ms bekleme, ayrıca EC I/O süresi
   harcıyordu. Artık arka plan işçisi yürütüyor; GTK yalnızca tamamlanmayı
   beklemeden kontrol ediyor. Bir aktif işlem ve en son bekleyen ayar tutuluyor.
   Aktif işlemle aynı ayara tekrar basmak yeni kopyalar biriktirmiyor.
3. Uygula anındaki değerler kopyalanıyor. İşlem sürerken renk seçimini
   değiştirmek zaten gönderilmiş isteği değiştirmiyor. Kapanış aktif işçinin
   bitmesini bekliyor; bekleyen yeni istekleri başlatmıyor. Kernel I/O süresiz
   takılırsa kapanış da bekleyebilir; iş parçacığı bir kernel zaman aşımı değildir.
4. Profil dosyası erişimi süreç kullanıcı kimliğini geçici değiştirdiği için,
   donanım işi sürerken profil işlemi ertelenmesi isteniyor. Bu işlem çakışması
   kontrol edildi; arka plan işçisi GTK widget'larına erişmiyor.
5. Yanıltıcı başlangıç `EC I/O not found` uyarısı kaldırıldı: ilk açma denemesi
   öncesinde yazılıyordu. Gerçek erişim/yazma hataları korunuyor.
6. “Donanım efekti” etiketi “Efekt 0x07 (deneysel)” oldu. Tooltip CQ171'deki
   belirsizliği açıklıyor. Profil değerleri değiştirilmedi; eski dosyalar okunuyor.
7. Slider CSS ölçüleri ve kenarlığı açık tanımlandı. GTK görsel doğrulaması
   bu ortamda yapılmadı; uyarının her temada giderildiği iddia edilmiyor.
8. İşlem loguna backend, sonuç ve süre eklendi. `written`, fiziksel başarı
   anlamına gelmiyor. Sürekli dosya kaydı henüz yok; `tee` kullanılabilir.

## Değiştirilmesi / eklenmesi / çıkarılması gerekenler

| Öncelik | Karar | Durum / doğrulama koşulu |
|---|---|---|
| P0 | GTK ana döngüsünden EC işlemlerini çıkar; tıklama kuyruğunu sınırla | Uygulandı; kontrollü bloke olan sahte işlem, son isteğin korunması, hata/istisna ve kapanış testleri eklendi. |
| P0 | CQ171 için doğru LED protokolünü doğrula | Açık. Mevcut EC dizisi değiştirilmedi; tahmini kayıt, daha uzun bekleme veya otomatik tekrarla düzeldi denmedi. |
| P1 | Uyumlu WMI LED backend'i | Sonraki donanım çalışması. Önce sürücü/API sürümü ve CQ171 desteği doğrulanmalı. Renk+efekt+parlaklığı tek protokol isteğinde taşımak hedeflenmeli; WMI parlaklık alanı doğrudan EC'nin 0–255 PWM alanıyla karıştırılmamalı. |
| P1 | Model, BIOS, backend ve kurulu kopya tanısı | `make diagnose` eklendi; firmware/sensör sorgulamadan dosya ve sysfs envanteri çıkarır. |
| P1 | Kalıcı, zaman damgalı kullanıcı logları | Henüz eklenmedi; log döndürme ve kullanıcı izinleriyle birlikte ele alınmalı. |
| P2 | Flash / heartbeat ve doğrulanmış dalga modu | CQ171 üzerinde destek doğrulanana kadar eklenmedi. |
| P2 | Üç bölgeyi tek profilde saklama | Mevcut profil tek bölgeyi saklıyor. Dosya sürümü ve eski profillerin geçişi tasarlanarak eklenebilir. |
| — | “Ambilight ekranı takip eder”, “yazma başarılı = efekt çalıştı” iddiaları | Desteklenmiyor; arayüz/belgelerde kullanılmamalı. |
| — | Fan, güç limiti, MUX veya BIOS değişimi | RGB sorununun çözümü kapsamına eklenmedi. |

## Doğrulama ve kalan cihaz testi

`make test` ve `make` yazılım doğrulamasıdır; gerçek klavye doğrulaması değildir.
Testler gerçek EC'yi açmaz. İşçi testi kontrollü bekleyen donanım varken ana
çağıranın devam ettiğini, işlem sırasını, bekleyen ayarın yenilenmesini ve hata
iletilmesini kontrol eder. Mevcut profil ve EC hata/yazma sırası testleri korunur.

Bu çalışma sonunda derleme, üç test programı, `git diff --check` ve tanı komutu
başarılı. İşçi testi AddressSanitizer/UndefinedBehaviorSanitizer ile de geçti;
ilk sandbox denemesinde LeakSanitizer ptrace kısıtı nedeniyle çalışamadı,
izinli sandbox dışı tekrar başarılı oldu. GTK penceresi ve fiziksel LED testi
yapılmadı. Tanı çıktısında proje ikilisi ile menüye kurulu ikilinin SHA-256
değerleri farklı; menüdeki kopya bu çalışma sırasında güncellenmedi.

Cihaz testi için eski uygulama kapatılmalı, güncel proje ikilisi çalıştırılmalı:

```bash
sudo ./ecc 2>&1 | tee /tmp/excalibur-rgb.log
```

Statik kırmızı → nefes → renk döngüsü → deneysel 0x07 → kapalı → açık
sırası her adımda **bir kez** uygulanıp birkaç saniye fiziksel olarak izlenmeli.
Bölge A/B/C ayrı kontrol edilmeli. Gözlem, mod, işlem süresi ve BIOS birlikte
kaydedilmeli. Tek tıklamada animasyon ve kapanıp açılma doğrulanmadan bu sorun
“çözüldü” sayılmamalı. Menüden açılan kurulu kopya proje ikilisinden bağımsızdır;
yeniden kurulum yapılmadıkça eski sürüm açılır.
