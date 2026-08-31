# excalibur-control-center
<h1> Excalibur Control Center Uygulamasının Linux Portu. </h1>

<h3> Eğer Arch Linux kullanıyorsanız "make" ve "linux-headers", Debian kullanıyorsanız "make" ve "linux-headers-amd64" paketlerini indirdiğinizden emin olun. Aksi takdirde ekran kartı değişimi çalışmayacaktır.</h3>

<h3>Eğer Excalibur Laptop'ınızda 10xx, 20xx veya 30xx serisi ekran kartı varsa, MUX Switch teknolojisi olmadığı için ekran kartı değiştirme özelliği çalışmayabilir.</h3>

<p> G9xx serisinde çalışmamaktadır.</p>

```sudo ./ecc``` ile çalıştırınız.

![Türkçe arayüz](https://raw.githubusercontent.com/FeusX/ecc-for-linux/main/ecc.png)

[![A video about making these kind of things](https://www.youtube.com/watch?v=kM0D9CpnmQY)](https://www.youtube.com/watch?v=kM0D9CpnmQY)

```g++ main.cc components/keyboard/keyboard.cc components/ui/ui.cc components/gpu/optimus.cc components/cpu/cpu.cc -o ecc -pthread $(pkg-config --cflags --libs gtk4)``` compile with this command.
