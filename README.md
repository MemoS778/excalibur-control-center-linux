# Excalibur Control Center Linux

Casper Excalibur dizüstü bilgisayarlar için C++ / GTK4 ile geliştirilen, topluluk destekli kontrol arayüzü.

[FeusX/excalibur-control-center](https://github.com/FeusX/excalibur-control-center) projesinden türetilmiştir. Bu sürüm, mevcut donanım kontrol kodunun üzerine Windows Control Center görünümünden esinlenen yeni bir GTK4 arayüzü ekler. Casper ile resmî bir bağlantısı yoktur.

**Durum:** Geliştirme aşamasında. G870 / CachyOS üzerinde önceki arayüzün açıldığı kullanıcı tarafından doğrulandı. Yeni arayüz derlendi; görsel ve donanım kontrollerinin kullanıcı tarafından doğrulanması bekleniyor. Diğer modeller için uyumluluk garantisi verilmez.

## Arayüz ve özellikler

- Koyu tema, turuncu vurgular ve ayrı LED / Sistem Modu sayfaları.
- Üç klavye bölgesi veya tüm bölgeler için renk seçimi.
- Renk çemberi, şematik klavye önizlemesi ve parlaklık kontrolü.
- Statik, nefes, renk döngüsü ve upstream Ambilight modu.
- İşlemci frekans sınırları ve Intel Turbo Boost kontrolleri.
- Onay iletişim kutusuyla Intel / NVIDIA geçişi (deneysel).
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

Ekran kartı geçişi mevcut upstream uygulamasını kullanır: sistem yapılandırması, servisler ve çekirdek modülleri üzerinde değişiklik yapabilir. Çalışan çekirdeğe uygun başlık dosyaları ve ek araçlar gerekir. CachyOS'ta başlık paketi çekirdek çeşidiyle eşleşmelidir; genel `linux-headers` paketi her çekirdek için uygun değildir. Bu özellik bu sürümde donanım üzerinde doğrulanmadı.

## Uyumluluk ve sınırlamalar

- Geliştirme ortamı: Excalibur G870, CachyOS, GTK4.
- Bölge ve EC kayıtları donanım sürümüne göre değişebilir.
- Upstream, G9xx serisini desteklemediğini belirtir.
- GPU geçişi, CPU kontrolleri ve efektlerin her modelde çalıştığı varsayılmamalıdır.
- Arayüz şu anda Türkçedir; upstream dil dosyaları henüz yeni arayüze uyarlanmamıştır.
- Yeni arayüzün ekran görüntüsü görsel doğrulamadan sonra eklenecektir. Depodaki `ecc.png` upstream arayüze aittir.

## Katkı ve kaynak

Hata bildirimine model, dağıtım, çekirdek sürümü ve terminal çıktısını ekleyin. Donanıma erişmeden arayüz üzerinde çalışmak için `make preview` kullanın.

- Orijinal Linux uygulaması ve donanım kontrolü: [FeusX](https://github.com/FeusX/excalibur-control-center).
- GPU kontrol kodunda upstream tarafından belirtilen kaynak: [EnvyControl](https://github.com/bayasdev/envycontrol).
- Bu türevdeki arayüz ve proje düzenlemeleri: MemoS778, Codex desteğiyle.

Orijinal Git geçmişi ve lisans dosyası korunur. Lisans: [GNU GPL v3](LICENSE).
