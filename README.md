# Excalibur RGB Control

Casper Excalibur dizüstü bilgisayarlar için C++ / GTK4 ile geliştirilen, topluluk destekli klavye aydınlatma uygulaması.

[FeusX/excalibur-control-center](https://github.com/FeusX/excalibur-control-center) projesinden türetilmiştir. Bu sürüm, mevcut donanım kontrol kodunun üzerine Windows Control Center görünümünden esinlenen yeni bir GTK4 arayüzü ekler. Casper ile resmî bir bağlantısı yoktur.

**Durum:** Geliştirme aşamasında. G870 / CachyOS üzerinde yeniden tasarlanan arayüzün açıldığı ekran görüntüsüyle doğrulandı. Yalnızca RGB kontrollerini içeren bu son sürümün donanım doğrulaması bekleniyor. Diğer modeller için uyumluluk garantisi verilmez.

## Arayüz ve özellikler

- Koyu tema, turuncu vurgular ve klavye aydınlatmasına odaklanan tek ekran.
- Üç klavye bölgesi veya tüm bölgeler için renk seçimi.
- Renk çemberi, şematik klavye önizlemesi ve parlaklık kontrolü.
- Statik, nefes, renk döngüsü ve donanım efekti (upstream 0x07 modu; ekran renklerini takip etmez).
- Seçili bölge, renk, efekt, parlaklık ve açık/kapalı ayarını saklayan üç profil yuvası.
- Donanım ayarı göndermeden arayüzü açmak için önizleme modu.

Klavye çizimi seçilen ayarların şematik önizlemesidir; donanımdan okunan canlı durum değildir. Fan kontrolü ve hazır Ofis / Oyun profilleri bu sürümde bulunmaz.

## Derleme

Gereksinimler: C++17 derleyici, Make, pkg-config ve GTK **4.12 veya üzeri** geliştirme dosyaları.

CachyOS / Arch Linux:

```bash
sudo pacman -S --needed base-devel gtk4 pkgconf
make
```

Depoyu indirdiğiniz dizinde `make` çalıştırın. Makefile derleyici seçeneklerini kendisi çözer; farklı kabuklarda komut ikamesi gerekmez.

## Çalıştırma

Önce donanıma ayar göndermeyen arayüz önizlemesi:

```bash
make preview
```

Donanım erişimi için:

```bash
sudo ./ecc
```

LED sayfasında bölge, renk, efekt ve parlaklığı seçip **Uygula** düğmesine basın. LED aç/kapat anahtarı ayarı hemen uygular. Uygulama açılışında klavye erişimi için `ec_sys` modülünü yazma desteğiyle yüklemeyi dener.

## Renk profilleri

**Seçili ayarı kaydet**, seçili profil yuvasını mevcut bölge, renk, efekt, parlaklık ve açık/kapalı ayarıyla günceller. **Profili yükle** ayarları ekrana getirir; donanıma göndermek için **Uygula** düğmesine basın.

Her profil tek bir bölge seçimini (A, B, C veya Hepsi) saklar; üç bölgenin farklı renklerini tek dosyada toplamaz. Dosyalar uygulamayı başlatan kullanıcının `~/.config/excalibur-rgb-control/` dizininde tutulur; sudo ile çalıştırıldığında da aynı kullanıcıya aittir. Bozuk/geçersiz profil dosyaları uygulanmaz.

CPU, ekran kartı geçişi ve sistem performans ayarları bu sürümden çıkarılmıştır. Klavye aydınlatması için bunlar gerekli değildir.

## Uyumluluk ve sınırlamalar

- Geliştirme ortamı: Excalibur G870, CachyOS, GTK4.
- Bölge ve EC kayıtları donanım sürümüne göre değişebilir.
- Upstream, G9xx serisini desteklemediğini belirtir.
- Aydınlatma efektlerinin her donanım sürümünde çalıştığı varsayılmamalıdır.
- Arayüz şu anda Türkçedir.
- Yeni arayüzün ekran görüntüsü görsel doğrulamadan sonra eklenecektir. Depodaki `ecc.png` upstream arayüze aittir.

## Katkı ve kaynak

Hata bildirimine model, dağıtım, çekirdek sürümü ve terminal çıktısını ekleyin. Donanıma erişmeden arayüz üzerinde çalışmak için `make preview` kullanın.

- Orijinal Linux uygulaması ve donanım kontrolü: [FeusX](https://github.com/FeusX/excalibur-control-center).
- Bu türevdeki arayüz ve proje düzenlemeleri: MemoS778, Codex desteğiyle.

Orijinal Git geçmişi ve lisans dosyası korunur. Lisans: [GNU GPL v3](LICENSE).

## Geliştirme kontrolü

`make test` profil kaydetme/okuma ve geçersiz dosya testlerini çalıştırır. `make` uygulamayı derler. Bu kontroller donanım uyumluluğu testi değildir.

## Uygulama menüsünden açma

```bash
make install-user
```

Bu komutu **sudo olmadan** çalıştırın. Ardından uygulama menüsünde **Excalibur RGB Control** arayın. Başlatıcı, klavye erişimi için terminalde yönetici parolanızı ister ve GTK penceresini açar. Parola saklanmaz. Program hata ile kapanırsa terminal, hata çıktısını okuyabilmeniz için açık kalır.

Kurulum dosyaları `~/.local/lib/excalibur-rgb-control/` altına kopyalanır. Yeni sürüm derledikten sonra `make install-user` komutunu tekrar çalıştırın; menüdeki kopya güncellenir. Profiller korunur. Menü girdisi kullanıcı veri dizininin `applications/` klasöründeki `io.github.MemoS778.ExcaliburRGB.desktop` dosyasıdır.

Başlatıcı eklemek donanım uyumluluğunu değiştirmez; G870 üzerindeki renk değişimi sorununun çözümü hâlâ doğrulanmayı beklemektedir.

## Komut zamanlaması

Renk ve efekt komutları aynı kilit altında sıralanır. Mod değişiminden sonra 50 ms beklenir; hızlı Uygula tıklamaları 150 ms içinde birleştirilir. Bu süre donanım onayı değildir, geçiş komutuyla renk yazımının çakışma ihtimalini azaltan bir düzenlemedir. Tek tıklamada fiziksel LED yanıtı cihaz üzerinde ayrıca doğrulanmalıdır.
