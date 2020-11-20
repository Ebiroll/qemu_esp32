# docker build -t qemu_esp32/ubuntu -f ./Dockerfile .
# run image with:
# docker run -v ~/shared-vm:/home/src  -p 1234:1234 -ti qemu_esp32/ubuntu /bin/bash
# where ~/shared-vm is a shared folder, and /home/src is where you work in this docker container.
# In windows
# docker run -v C:\work\qemu_esp32\:/home/src -p 1234:1234 -ti qemu_esp32/ubuntu   /bin/bash
# The esp32flash.bin must be in the shared-vm path
# You have to build the qemu-system-xtensa file and copy it to the correct location

FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

# Make sure the image is updated, install some prerequisites,
 RUN apt-get update && apt-get install -y \
   xz-utils libpixman-1-0 libpng16-16 libjpeg8 libglib2.0 \
   wget \
   unzip \   
   &&    wget https://github.com/espressif/qemu/raw/esp-develop/pc-bios/esp32-r0-rom.bin  

 EXPOSE 1234

 # Start from a Bash prompt
 CMD [ "/bin/bash" ]

# wget https://github.com/Ebiroll/qemu-xtensa-esp32/suites/1160719439/artifacts/16816963  && \
# &&  unzip 16816963 -d .
# && \
#   echo 'export PATH=/clang_8.0.0/bin:$PATH' >> ~/.bashrc && \
#   echo 'export LD_LIBRARY_PATH=/clang_8.0.0/lib:LD_LIBRARY_PATH' >> ~/.bashrc
