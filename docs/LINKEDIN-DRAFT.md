Linux'a geçince dizüstü bilgisayarın üreticiye özel kontrol yazılımını geride bırakmak zorunda kalabiliyoruz. Ben de CachyOS kullandığım Excalibur G870 için bu deneyimi geliştirmeye başladım.

FeusX'in açık kaynak Excalibur Control Center projesini temel alarak C++ ve GTK4 ile klavye aydınlatmasına odaklanan Excalibur RGB Control arayüzünü hazırladım: koyu tema, üç bölgeli klavye önizlemesi, renk çemberi, efekt seçimi ve parlaklık kontrolü.

Projede klavye kontrol altyapısını koruyup CPU ve ekran kartı ayarlarını çıkardım. Renk, bölge, efekt ve parlaklık ayarlarını tekrar kullanabilmek için üç profil yuvası ekledim. Ayrıca donanım ayarı göndermeden tasarım üzerinde çalışmak için bir önizleme modu ve Makefile ekledim. Geliştirme sürecinde Codex'ten destek aldım.

Proje henüz geliştirme aşamasında; farklı Excalibur modellerinde geri bildirimlerle ilerletmek istiyorum. Orijinal projeyi açık kaynak olarak paylaşan FeusX'e teşekkürler.

GitHub: https://github.com/MemoS778/excalibur-control-center-linux

#Linux #CachyOS #OpenSource #CPP #GTK #Excalibur

---
Yayımlamadan önce: yeni arayüzün gerçek ekran görüntüsünü ekle, G870 üzerindeki kontrolleri doğrula ve depo bağlantısını güncelle. Bu dosya paylaşım taslağıdır; LinkedIn'e gönderilmemiştir.
