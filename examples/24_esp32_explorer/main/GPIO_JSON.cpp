#include <stdlib.h>
#include <JSON.h>
extern "C" {
	#include <soc/gpio_struct.h>
}

JsonObject GPIO_JSON() {
	JsonObject obj = JSON::createObject();
	JsonArray tmpArr = JSON::createArray();
	int i;
	for (i=0;i<32; i++) {
		tmpArr.addBoolean((GPIO.out & (1<<i)) != 0);
	}
	for (i=0; i<8; i++) {
		tmpArr.addBoolean((GPIO.out1.data & (1<<i)) != 0);
	}
	obj.setArray("out", tmpArr);

	tmpArr = JSON::createArray();
	for (i=0;i<32; i++) {
		tmpArr.addBoolean((GPIO.in & (1<<i)) != 0);
	}
	for (i=0; i<8; i++) {
		tmpArr.addBoolean((GPIO.in1.data & (1<<i)) != 0);
	}
	obj.setArray("in", tmpArr);

	tmpArr = JSON::createArray();
	for (i=0;i<32; i++) {
		tmpArr.addBoolean((GPIO.enable & (1<<i)) != 0);
	}
	for (i=0; i<8; i++) {
		tmpArr.addBoolean((GPIO.enable1.data & (1<<i)) != 0);
	}
	obj.setArray("enable", tmpArr);

	tmpArr = JSON::createArray();
	for (i=0;i<32; i++) {
		tmpArr.addBoolean((GPIO.status & (1<<i)) != 0);
	}
	for (i=0; i<8; i++) {
		tmpArr.addBoolean((GPIO.status1.intr_st & (1<<i)) != 0);
	}
	obj.setArray("status", tmpArr);

	JsonObject tmpObj = JSON::createObject();
	tmpObj.setBoolean("MTDI",  (GPIO.strap.strapping & (1<<5)) != 0);
	tmpObj.setBoolean("GPIO0", (GPIO.strap.strapping & (1<<4)) != 0);
	tmpObj.setBoolean("GPIO2", (GPIO.strap.strapping & (1<<3)) != 0);
	tmpObj.setBoolean("GPIO4", (GPIO.strap.strapping & (1<<2)) != 0);
	tmpObj.setBoolean("MTDO",  (GPIO.strap.strapping & (1<<1)) != 0);
	tmpObj.setBoolean("GPIO5", (GPIO.strap.strapping & (1<<0)) != 0);
	obj.setObject("strapping", tmpObj);
/*
	tmpArr = JSON::createArray();
	for (i=0; i<256; i++) {
		tmpObj = JSON::createObject();
		tmpObj.setBoolean("sig_in_sel", GPIO.func_in_sel_cfg[i].sig_in_sel);
		tmpObj.setBoolean("sig_in_inv", GPIO.func_in_sel_cfg[i].sig_in_inv);
		tmpObj.setInt("func_sel",       GPIO.func_in_sel_cfg[i].func_sel);
		tmpArr.addObject(tmpObj);
	}
	obj.setArray("func_in_sel_cfg", tmpArr);
*/
	tmpArr = JSON::createArray();
	for (i=0; i<40; i++) {
		tmpObj = JSON::createObject();
		tmpObj.setBoolean("oen_inv_sel", GPIO.func_out_sel_cfg[i].oen_inv_sel);
		tmpObj.setBoolean("oen_sel",     GPIO.func_out_sel_cfg[i].oen_inv_sel);
		tmpObj.setBoolean("inv_sel",     GPIO.func_out_sel_cfg[i].inv_sel);
		tmpObj.setInt("func_sel",        GPIO.func_out_sel_cfg[i].func_sel);
		tmpArr.addObject(tmpObj);
	}
	obj.setArray("func_out_sel_cfg", tmpArr);
	return obj;
}
