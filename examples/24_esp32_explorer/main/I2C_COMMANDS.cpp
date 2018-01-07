/*
 * I2C.cpp
 *
 *  Created on: 18.09.2017
 *      Author: darek
 */
#include <string>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <driver/gpio.h>
#include <JSON.h>
extern "C" {
	#include <soc/i2c_struct.h>
}
#include "I2C.h"

JsonObject I2C_READ(std::map<std::string, std::string> parts) {
	uint8_t SDA, SCL, ADDR, REG;
	uint8_t bytes = atoi(parts.at("bytesCount").c_str());
	SDA  = atoi(parts.at("sda").c_str());
	SCL  = atoi(parts.at("scl").c_str());
	ADDR = atoi(parts.at("address").c_str());
	REG  = atoi(parts.at("register").c_str());


	JsonObject obj = JSON::createObject();
	JsonObject tmpObj = JSON::createObject();
	I2C* i2c = new I2C();
	i2c->init(ADDR, (gpio_num_t)SDA, (gpio_num_t)SCL);

	i2c->beginTransaction();
	i2c->write(REG);
	i2c->endTransaction();

	uint8_t data[bytes];
	i2c->beginTransaction();
	i2c->read(data, bytes-1, true);
	i2c->read(data+bytes-1, false);
	i2c->endTransaction();
	//i2c->stop();
	delete i2c;

	for(int i=0;i<bytes;i++){
		char tmp[bytes];
		sprintf(tmp, "%d", data[i]);
		tmpObj.setString("value_"+i, tmp);
	}

	obj.setObject("data", tmpObj);
	return obj;
}


JsonObject I2C_WRITE(std::map<std::string, std::string> parts) {
	uint8_t SDA, SCL, ADDR, REG;
	std::string hex_byte;
	uint8_t bytes = atoi(parts.at("bytesCount").c_str());
	uint8_t dat[1];

	SDA    = atoi(parts.at("sda").c_str());
	SCL    = atoi(parts.at("scl").c_str());
	ADDR   = atoi(parts.at("address").c_str());
	REG    = atoi(parts.at("register").c_str());
	dat[0] = atoi(parts.at("data").c_str());

	JsonObject obj = JSON::createObject();
	JsonObject tmpObj = JSON::createObject();

	printf("write to reg: %d, val: %d", REG, dat[0]);
	I2C* i2c = new I2C();
	i2c->init(ADDR, (gpio_num_t)SDA, (gpio_num_t)SCL);

	i2c->beginTransaction();
	i2c->write(REG);
	/*
	i2c->write(dat, bytes-1, true);
	i2c->write(dat[bytes-1], false);*/
	i2c->write(dat[0], false);
	i2c->endTransaction();
	delete i2c;


	obj.setObject("error", tmpObj);
//	free(dat);
	return obj;
}
