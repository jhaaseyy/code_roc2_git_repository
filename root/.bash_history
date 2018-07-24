cd ..
ls
ls -lt
ls -lt|head
date
more wpa_supplicant_wlan0.conf
cd wpa_supplicant-wlan0.conf 
more wpa_supplicant-wlan0.conf 
ls -lt|head
mv wpa_supplicant-wlan0.conf /wpa_supplicant/
cd wpa_supplicant
pwd
cd ..
mv wpa_supplicant-wlan0.conf /etc/wpa_supplicant
ls
ls wpa* -lt
cd ..
cd /etc/wpa_supplicant
ls
more wpa_supplicant-wlan0.conf 
ifdown wlan0
ifup wlan0
ifconfig
wpa_passphrase ROC_HUB_03 rocadminpw >/etc/wpa_supplicant/wpa_supplicant-wlan0.conf 
systemctl stop wpa_supplicant@wlan0
systemctl start wpa_supplicant@wlan0
systemctl restart systemd-networkd
ls
more wpa_supplicant-wlan0.conf 
reboot
gpio gps_power lo
ifconfig
ls
cd /data/roc/20180614/
ls
ls -lt|head
ls -lth|head
ls -lth
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
reboot
gpio gps_power lo
cd /data/roc/20180614
ls
ls -lt|head
ls -lt|head
ls -lt|head
ls -lt|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
reboot
gpio gps_power lo
cd /data/roc/20180614
ls -lt|head
cd /etc/opt/roc/
ls
more config1.txt 
shutdown now
cd /etc/init.d
ls
vi roc_gpio 
reboot
gpio gps_power hi
gpio gps_power l0
gpio gps_power lo
gpio led1 hi
gpio led1 lo
gpio led0 hi
gpio led1 hi
gpio led1 lo
gpio led0 lo
reboot
reboot
cd /lib/lsb
ls
more init-functions
ls
ls init-functions.d/
cd ..
ls
cd /var/log
ls
more messages
ls -l
cd
cd /etc/init.d
more roc_gpio 
more /opt/roc/gpio 
ls
cat > roc_gpio
cd ../rc6.d/
ls
cd ../rc5.d/
ls
more S02roc_gpio 
exit
more /etc/init.d/roc_gpio 
gpio gps_power hi
reboot
systemctl status roc_gpio.service
systemctl -l status roc_gpio.service
more /etc/init.d/roc_gpio 
/etc/init.d/roc_gpio start
vi /etc/init.d/roc_gpio
/etc/init.d/roc_gpio start
reboot
/etc/init.d/roc_gpio stop
/etc/init.d/roc_gpio start
reboot
vi /etc/init.d/roc_gpio 
reboot
cd roc
cd roc
ls
vi roc2.c
vi roc2.c
ls -l
make
make
exit
shutdown now
reboot
cd roc
cd roc
more zephyr.c
more /etc/opt/roc/zephyr.conf 
ps aux | grep roc
kil 313
kill 313
zephyr -v
cd /etc/init.d
more roc
vi roc_gpio 
shutdown now
cd /data
cd roc
ls
cd 20180617
ls -l
ls -l
ls -l
ls -l
cat roc_20180617205627.sbf
PuTTYPuTTYPuTTYPuTTYPuTTYPuTTY
ls -l
cd /etc/opt/roc
ls
more gps.conf 
more config1.txt 
more roc.conf
vi config1.txt 
ps aux | grep roc
kill 304
roc &
ls -l
cd /data/roc
ls
cd 20180617
ls -l
ls
ls -l
fg
cd /etc/opt/roc
ls
vi roc.conf
more zephyr.conf 
more roc.conf
minicom -s
minicom -s
ps aux | grep roc
kill 311
minicom -s
minicom -s
cd /etc/init.d
more roc_uart 
minicom -s
cd /etc/opt/roc
more roc.conf
roc -v
vi config1.txt 
roc -v
bg
cd /data/roc
ls
cd 20180617
ls -l
ls -l
more roc_20180617212200.sbf
 
fg
ps aux | grep roc
gpio gps_power hi
gpio gps_power lo
reboot
cd roc
cd roc
vi zephyr.c
ps aux | grep roc
kill 311
zephyr
vi /etc/init.d/roc
reboot
vi /etc/init.d
cd /etc/init.d
ls
cd ../rc2.d/
ls
cd ../init.d
update-rc.c roc defaults
update-rc.d roc defaults
cd ../rc2.d/
ls
cd ../init.d
ls
more roc
update-rc.d roc_gpio defaults
ls ../rc2.d/
ls ../rc3.d/
more roc_uart 
more roc_gpio 
vi roc
update-rc.d roc
update-rc.d roc defaults
ls ../rc2.d
ls -a
more .depend.start 
more /etc/insserv/overrides/
cd /etc/insserv/overrides/
ls
ls -la
cat > roc
update-rc.d roc defaults
cd /etc/rc2.d/
ls
ls
cd /etc/insserv/overrides/
ls
rm roc
cd /etc/init.d
vi roc
reboot
gpio led1 hi
cd /etc/init.d
vi roc_gpio 
ps aux | grep roc
kill 343 345
ls
gpio led0 lo
gpio led0 hi
gpio led0 lo
gpio led1 lo
ps aux | grep roc
./roc
reboot
vi /etc/init.d/roc_gpio 
vi roc
vi /etc/init.d/roc
reboot
cd roc
cd roc
ls -la
ls -la /opt/roc
more zephyr.c
vi zephyr.c
zephyr -v
cd /etc/init.d
more roc
gpio led1 lo
./roc
vi roc
./roc
shutdown now
cd /data/roc/
ls
cd 20180617
ls
ls -lt
ls -lt|head
ls -lt|head
ls -lt|head
cd /etc/opt/roc/
ls
more config1.txt 
vi config1.txt 
ls
more config1.txt 
reboot
cd /data/roc/20180617/
ls -lt|head
ls -lt|head
ls -lt|head
ls -lt|head
ls -lt|head
ls -lt|head
ls -lt|head
ls -lt|head
cd /etc/opt/roc/
ls
vi config1.txt 
more config1.txt 
reboot
cd /data/roc/
ls
cd 20180629/
ls
ls -lt
date
date
ls -lt
ls
ls -lt
ls -lt|head
cd /etc/opt/roc/
ls
more config1.txt 
cd /data/roc/20180629/
ls
ls -lt
ls -lt|h
ls -lt|head
cd /etc/opt/roc/
ls
more gps.conf 
ls
more roc.conf
cd /data/roc/20180629/
ls
rm *sbf
ls
ls
ls
ls -lt
ls -lte
ls ../queue/
ls ../queue/
ls -lt
ls -lt
ls -lt
ls -lt
ls -lt
ls -lt
ls -lt
ls ../queue/
ls ../queue/
cd /etc/opt/roc/
ls
more config1.txt 
cd /data/roc/20180629/
ls
ls -lt
ls -lt
ls -lth
ls ../queue/
ls ../queue/
ls ../queue/
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lth|head
ls -lt|head
ls -lt|head
ls -lt|head
ls -lt|head
ls
cd ..
ls
ls
cd queue/
ls
cd ..
ls
ls
cd 20180629/
ls
pwd
cd ..
ls
queue/
ls
cd queue/
ls
cd ..
ls
cd 20180629/
;s
ls
pwd
cd /
ls
cd 
ls
cd /root
ls
cd /data/roc/
ls
cd 20180629/
ls
cd ..
ls
 queue/
ls
cd queue/
ls
cd /root
ls
ls -lt
cd roc
ls
cd ..
ls
cd roc
ls
cd roc
ls
cd ..
ls
cd ..
cd /opt/roc/
ls
cd roc 
ls =lt
cd /etc/
ls
cd /etc/opt/
;s
cd /etc/opt/
ls
cd roc/
ls
cd /data/roc/
ls
cd queue/
ls
ls
ls
ls
ls
ls
mroe l
mroe l
mroe list.txt
mroe list.txt
mroe list.txt
more list.txt
more list.txt
more list.txt
more list.txt
more list.txt
date
ls 
hd -h
hd 
hf
dh
df
df -h
more list.txt
more list.txt
more list.txt
more list.txt
more list.txt
more list.txt
more list.txt
more list.txt
more list.txt
more list.txt
ls
more list.txt
more list.txt
more list.txt
ls
ls
more list.txt
more list.txt
more list.txt
more list.txt
more list.txt
cd ../20180629/
ls
ls -lt|head 
ls -lth|head 
ls
shutdown now
