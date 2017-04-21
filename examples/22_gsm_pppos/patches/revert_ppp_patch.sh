#!/bin/bash

IDF_PATH=$PWD

cd ${IDF_PATH}/components/esp32
git checkout Kconfig
rm -f *.orig

cd ${IDF_PATH}/components/esp32/include
git checkout esp_task.h
rm -f *.orig

cd ${IDF_PATH}/components/lwip
git checkout Kconfig
git checkout component.mk
rm -f *.orig

cd ${IDF_PATH}/components/lwip/api
git checkout pppapi.c
rm -f *.orig

cd ${IDF_PATH}/components/lwip/include/lwip/port
git checkout lwipopts.h
rm -f *.orig

cd ${IDF_PATH}/components/lwip/port/freertos
git checkout sys_arch.c
rm -f *.orig

rm -f -R ${IDF_PATH}/examples/protocols/pppos_client