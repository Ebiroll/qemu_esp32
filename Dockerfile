# docker build -t qemu_esp32/ubuntu -f ./Dockerfile .
# run image with:
# docker run -v ~/shared-vm:/home/src  -p 1234:1234 -ti qemu_esp32/ubuntu /bin/bash
# where ~/shared-vm is a shared folder, and /home/src is where you work in this docker container.
# In windows
# docker run -v C:\work\qemu_esp32\:/home/src -p 1234:1234 -ti qemu_esp32/ubuntu   /bin/bash
# The esp32flash.bin must be in the shared-vm path

FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

# Make sure the image is updated, install some prerequisites,
 RUN apt-get update && apt-get install -y \
   xz-utils libpixman-1-0 libpng16-16 libjpeg8 libglib2.0 \
   wget \
   unzip \   
   && wget https://github.com/Ebiroll/qemu-xtensa-esp32/suites/317165267/artifacts/360727  && \
   wget https://github.com/espressif/qemu/raw/esp-develop/pc-bios/esp32-r0-rom.bin  && \
   unzip 360727  -d .

 EXPOSE 1234

 # Start from a Bash prompt
 CMD [ "/bin/bash" ]

#&& \
#   echo 'export PATH=/clang_8.0.0/bin:$PATH' >> ~/.bashrc && \
#   echo 'export LD_LIBRARY_PATH=/clang_8.0.0/lib:LD_LIBRARY_PATH' >> ~/.bashrc
