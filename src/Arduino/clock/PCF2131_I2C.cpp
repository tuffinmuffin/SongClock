#include "RTC_NXP.h"

void showRed(uint8_t bright = 255);
void showGreen(uint8_t bright = 255);
void showBlue(uint8_t bright = 255);
void showPurple(uint8_t bright = 255);
void showYellow(uint8_t bright = 255);
void showWhite(uint8_t bright = 255);
void showMagenta(uint8_t bright = 255);
void showCyan(uint8_t bright = 255);
void showOff();

PCF2131_I2C::PCF2131_I2C( uint8_t i2c_address ) : I2C_device( i2c_address )
{
}

PCF2131_I2C::PCF2131_I2C( TwoWire& wire, uint8_t i2c_address ) : I2C_device( wire, i2c_address )
{
}

PCF2131_I2C::~PCF2131_I2C()
{
}

void PCF2131_I2C::_reg_w( uint8_t reg, uint8_t *vp, int len )
{
  showCyan();
	reg_w( reg, vp, len );
  showGreen();
}

void PCF2131_I2C::_reg_r( uint8_t reg, uint8_t *vp, int len )
{
  showYellow();
	reg_r( reg, vp, len );
  showGreen();
}

void PCF2131_I2C::_reg_w( uint8_t reg, uint8_t val )
{
  showCyan();
	reg_w( reg, val );
  showGreen();
}

uint8_t PCF2131_I2C::_reg_r( uint8_t reg )
{
  showWhite();
  uint8_t d = reg_r( reg );
  showGreen();
	return 	d;
}

void PCF2131_I2C::_bit_op8( uint8_t reg, uint8_t mask, uint8_t val )
{
	bit_op8( reg, mask, val );
}
