#include "include/i2c.hpp"

#define _DELAY ets_delay_us(1)
#define _SDA1 gpio_set_level(sda_pin,1)
#define _SDA0 gpio_set_level(sda_pin,0)

#define _SCL1 gpio_set_level(scl_pin,1)
#define _SCL0 gpio_set_level(scl_pin,0)

#define _SDAX gpio_get_level(sda_pin)
#define _SCLX gpio_get_level(scl_pin)

I2C::I2C(gpio_num_t scl, gpio_num_t sda)
{
    scl_pin = scl;
    sda_pin = sda;

    gpio_set_pull_mode(scl_pin,GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(sda_pin,GPIO_PULLUP_ONLY);

    gpio_set_direction(scl_pin,GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(sda_pin,GPIO_MODE_INPUT_OUTPUT);

    // I2C bus idle state.
    gpio_set_level(scl_pin,1);
    gpio_set_level(sda_pin,1);
}


bool I2C::start(void)
{
    _SDA1;
    _SCL1;
    _DELAY;
    if (_SDAX == 0) return false; // Bus busy
    _SDA0;
    _DELAY;
    _SCL0;
    return true;
}



void I2C::stop(void)
{
    _SDA0;
    _SCL1;
    _DELAY;
    while (_SCLX == 0); // clock stretching
    _SDA1;
    _DELAY;
}


// return: true - ACK; false - NACK
bool I2C::write(uint8_t data)
{
    uint8_t ibit;
    bool ret;

    for (ibit = 0; ibit < 8; ++ibit)
    {
        if (data & 0x80)
            _SDA1;
        else
            _SDA0;
        _DELAY;
        _SCL1;
        _DELAY;
        data = data << 1;
        _SCL0;
    }
    _SDA1;
    _DELAY;
    _SCL1;
    _DELAY;
    ret = (_SDAX == 0);
    _SCL0;
    _DELAY;

    return ret;
}


uint8_t I2C::read(void)
{
    uint8_t data = 0;
    uint8_t ibit = 8;

    _SDA1;
    while (ibit--)
    {
        data = data << 1;
        _SCL0;
        _DELAY;
        _SCL1;
        _DELAY;
        if (_SDAX)
            data = data | 0x01;
    }
    _SCL0;

    return data;
}


void I2C::set_ack(bool ack)
{
    _SCL0;
    if (ack)
        _SDA0;  // ACK
    else
        _SDA1;  // NACK
    _DELAY;
    // Send clock
    _SCL1;
    _DELAY;
    _SCL0;
    _DELAY;
    // ACK end
    _SDA1;
}

