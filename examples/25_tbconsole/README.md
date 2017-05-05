# Atempt to start basic interpreter

http://hackaday.com/2016/10/27/basic-interpreter-hidden-in-esp32-silicon/

pull GPIO 12 high and hit reset. Connect to the ESP32 over serial, and hit enter to stop it from continually rebooting. 

Have not been able to get it to run

  wifi read 1c 

# qemu
To connect to uart
  nc 127.0.0.1 8880

Stops in this loop,


  wifi read 1c 
  io read 4808c 
  io write 4808c,0 
  io read 5f048 
  io write 5f048,0 
  wifi read 0 

