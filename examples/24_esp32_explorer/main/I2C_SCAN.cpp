/*
 * I2C_SCAN.cpp
 *
 *  Created on: 18.09.2017
 *      Author: darek
 */


#include "sdkconfig.h"
#include <stdlib.h>
#include <JSON.h>
#include <I2C.h>
#include <stdio.h>
#include <sstream>

//static const char *LOG_TAG = "I2C_SCAN";

/**
 * @begin Scan the I2C ports.
 * @return A JsonObject that describes the I2C scan.
 */
JsonObject I2C_SCAN_JSON() {
	JsonObject obj    = JSON::createObject();
	JsonObject tmpObj = JSON::createObject();
	JsonArray tmpArr  = JSON::createArray();
	//tmpArr = JSON::createArray();
	int i;
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("00:         ");

	I2C i2c;
//	i2c.init(0);

	for (i=3; i<0x78; i++) {   // Loop over each of the possible I2C addresses.
		bool slavePresent = i2c.slavePresent(i);

		if (i%16 == 0) {
			std::stringstream ss;
			ss << "address " << std::hex << (i-16);
			obj.setObject(ss.str(), tmpObj);
			tmpObj = JSON::createObject();
			printf("\n%.2x:", i);
		}
		std::stringstream ss;
		ss << "address " << std::hex << i;
		if (slavePresent) {
			tmpObj.setBoolean(ss.str(), true);
			tmpArr.addBoolean(true);
			printf(" %.2x", i);
		} else {
			tmpObj.setBoolean(ss.str(), false);
			tmpArr.addBoolean(false);
			printf(" --");
		}
	} // End for each of the I2C addresses.
	printf("\n");
	obj.setObject("70", tmpObj);
	obj.setArray("present", tmpArr);
	//obj.setObject("I2C", tmpObj);
	return obj;
} // I2C_SCAN_JSON
