# docker build -t qemu_esp32/ubuntu -f ./Dockerfile .
# run image with:
# docker run -v ~/shared-vm:/home/src -ti qemu_esp32/ubuntu /bin/bash
# where ~/shared-vm is a shared folder, and /home/src is where you work in this docker container.
# The romfiles must be in the shared-vm path

FROM ubuntu:18.04
 
# Make sure the image is updated, install some prerequisites,
 RUN apt-get update && apt-get install -y \
   xz-utils libpixman-1-0 libpng16-16 libjpeg8 libglib2.0 \
   wget \
   unzip \   
   && wget https://github.com/Ebiroll/qemu-xtensa-esp32/suites/309734438/artifacts/315433  && \
   unzip 315433  -d .


 # Start from a Bash prompt
 CMD [ "/bin/bash" ]

#&& \
#   echo 'export PATH=/clang_8.0.0/bin:$PATH' >> ~/.bashrc && \
#   echo 'export LD_LIBRARY_PATH=/clang_8.0.0/lib:LD_LIBRARY_PATH' >> ~/.bashrc