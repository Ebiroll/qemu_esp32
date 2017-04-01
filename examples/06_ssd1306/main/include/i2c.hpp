#ifndef I2C_H_
#define I2C_H_

#include "driver/gpio.h"
#include "rom/ets_sys.h"

class I2C {
private:
	gpio_num_t scl_pin, sda_pin;

public:
	I2C(gpio_num_t scl, gpio_num_t sda);
	void init(uint8_t scl_pin, uint8_t sda_pin);
	bool start(void);
	void stop(void);
	bool write(uint8_t data);
	uint8_t read(void);
	void set_ack(bool ack);
};

#endif
