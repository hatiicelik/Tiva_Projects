Uygulama açıldığında Windows, bilgisayardaki tüm COM portlarını tarar ve liste kutusuna (comboBox1) ekler.

Kullanıcı listeden bir COM port seçer ve Port Aç butonuna basar.

Program seçilen COM portu 9600 baud, 8N1 ayarlarıyla açar ve bağlantı başarılı olursa Port Aç pasif, Port Kapa aktif hâle gelir.

Kullanıcı bağlantıyı sonlandırmak istediğinde Port Kapa butonuna basar; port kapatılır ve butonlar başlangıç durumuna döner.

Saat göndermek için kullanıcı textBox1 içine HH:MM:SS formatında bir değer yazar.

Saat Gönder butonuna basıldığında program saat formatını kontrol eder.

Format doğruysa Tiva'ya ?HH:MM:SS\n şeklinde bir komut gönderilir.

Metin göndermek için kullanıcı textBox2 içine istediği yazıyı girer.

Program bu metni otomatik olarak tam 3 karakter olacak şekilde ayarlar (kısaysa boşlukla tamamlar, uzunsa keser).

Metin Gönder butonuna basıldığında Tiva’ya *ABC\n formatında gönderilir.

Tiva kartı PC’ye veri gönderdiğinde DataReceived olayı tetiklenir ve gelen karakterler bir buffer’da biriktirilir.

Program, buffer içinde \n karakteri gördüğünde bunun tam bir satır olduğunu kabul eder.

Eğer satır R ile başlıyorsa (örn: R12:34:56,4095,1), veri işleme fonksiyonuna aktarılır.

Gelen satır üç parçaya ayrılır:

Saat bilgisi (ör. 12:34:56)

ADC değeri (ör. 4095)

Buton durumu (1 veya 0)

Saat bilgisi arayüzde textBox3 içine yazılır.

ADC değeri textBox4 içine yazılır.

Buton durumu:

1 ise textBox5 = "Butona Basıldı"

0 ise textBox5 = "Basılı Değil" olarak güncellenir.

Uygulama kapanırken eğer seri port açıksa güvenli bir şekilde kapatılır.
