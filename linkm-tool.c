#define linkm_debug false

#include "linkm-lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define I2C_ADXL345 83
#define I2C_HMC5843 30
#define I2C_PSITG3200 104

struct int_bytes{
	char lsb;
	char msb;
};

union ADXL345_component{
	struct int_bytes component;
	int16_t data;
};

struct ADXL345_data{
	union ADXL345_component x,y,z;
};

union HMC5843_component{
	struct int_bytes component;
	int16_t data;
};

struct HMC5843_data{
	union HMC5843_component x,y,z;
	int16_t data;
};

union PSITG3200_component{
	struct int_bytes component;
	uint16_t data;
};

struct PSITG3200_data{
	union PSITG3200_component x,y,z;
	int16_t data;
};

uint8_t recbuf[16];
uint8_t cmdbuf[10];


/*
 * LinkM firmware - USB HID to I2C adapter for BlinkM
 *
 * Command format:  (from host perspective)
 *
 * pos description
 *  0    <startbyte>      ==  0xDA
 *  1    <linkmcmdbyte>   ==  0x01 = i2c transaction, 0x02 = i2c bus scan, etc. 
 *  2    <num_bytes_send> ==  starting at byte 4
 *  3    <num_bytes_recv> ==  may be zero if nothing to send back
 *  4..N <cmdargs>        == command 
 *
 * For most common command, i2c transaction (0x01):
 * pos  description
 *  0   0xDA
 *  1   0x01
 *  2   <num_bytes_to_send>
 *  3   <num_bytes_to_receive>
 *  4   <i2c_addr>   ==  0x00-0x7f
 *  5   <send_byte_0>
 *  6   <send_byte_1>
 *  7   ...
 *
 * Command byte values
 *  0x00 = no command, do not use
 *  0x01 = i2c transact: read + opt. write (N arguments)
 *  0x02 = i2c read                        (N arguments)
 *  0x03 = i2c write                       (N arguments)
 *  0x04 = i2c bus scan                    (2 arguments, start addr, end addr)
 *  0x05 = i2c bus connect/disconnect      (1 argument, connect/disconect)
 *  0x06 = i2c init                        (0 arguments)
 *
 *  0x100 = set status LED                  (1 argument)
 *  0x101 = get status LED
 * 
 * Response / Output buffer format:
 * pos description
 *  0   transaction counter (8-bit, wraps around)
 *  1   response code (0 = okay, other = error)
 *  2   <resp_byte_0>
 *  3   <resp_byte_1>
 *  4   ...
 *
 * 2009, Tod E. Kurt, ThingM, http://thingm.com/
 *
 */

/*
	int linkm_open(usbDevice_t** dev);

	void linkm_close(usbDevice_t* dev);

	int linkm_command(usbDevice_t* dev,
                  int cmd,
                  int bytes_send,
                  int bytes_recv,
                  uint8_t* buf_send,
                  uint8_t* buf_recv);

	char* linkm_error_msg(int errCode);

*/

int init_sensors(usbDevice_t* dev){
	int ret=1;
	int err;
	//init ADXL345
	cmdbuf[0] = I2C_ADXL345;		//I2C address
	cmdbuf[1] = 0x2D;
	cmdbuf[2] = 8;

	if((err = linkm_command(dev,LINKM_CMD_I2CTRANS, 3,8, cmdbuf, recbuf))){		//Send I2C command
		fprintf(stderr,"error on cmd: %s\n",linkm_error_msg(err));
		ret=0;
	};
	// Resolution dataformat, etc...

	//init HMC5843
	cmdbuf[0] = I2C_HMC5843;
	cmdbuf[1] = 0x0;
	cmdbuf[2] = 0b00011000;		// Set to maximum rate
	if((err = linkm_command(dev,LINKM_CMD_I2CTRANS, 3,8, cmdbuf, recbuf))){		//Send I2C command
		fprintf(stderr,"error on cmd: %s\n",linkm_error_msg(err));
		ret=0;
	}
	cmdbuf[0] = I2C_HMC5843;
	cmdbuf[1] = 0x02;
	cmdbuf[2] = 0;		// Enable measurement
	if((err = linkm_command(dev,LINKM_CMD_I2CTRANS, 3,8, cmdbuf, recbuf))){		//Send I2C command
		fprintf(stderr,"error on cmd: %s\n",linkm_error_msg(err));
		ret=0;
	}

	//init PS-ITG-3200
	cmdbuf[0] = I2C_PSITG3200;
	cmdbuf[1] = 0x16;
	cmdbuf[2] = 0b00011000;		// Enable measurement
	if((err = linkm_command(dev,LINKM_CMD_I2CTRANS, 3,8, cmdbuf, recbuf))){		//Send I2C command
		fprintf(stderr,"error on cmd: %s\n",linkm_error_msg(err));
		ret=0;
	}
	return ret;
};

struct ADXL345_data read_ADXL345(usbDevice_t* dev){
	struct ADXL345_data result;
	int err;
	cmdbuf[0] = I2C_ADXL345;
	cmdbuf[1] = 0x32;
	if((err = linkm_command(dev,LINKM_CMD_I2CTRANS, 2,6, cmdbuf, recbuf))){		//Send I2C command
		fprintf(stderr,"error on cmd: %s\n",linkm_error_msg(err));
		return result;
	};
	result.x.component.lsb=recbuf[0];
	result.x.component.msb=recbuf[1];
	result.y.component.lsb=recbuf[2];
	result.y.component.msb=recbuf[3];
	result.z.component.lsb=recbuf[4];
	result.z.component.msb=recbuf[5];

	return result;
};

struct HMC5843_data read_HMC5843(usbDevice_t* dev){
	struct HMC5843_data result;
	int err;
	cmdbuf[0] = I2C_HMC5843;
	cmdbuf[1] = 0x03;
	if((err = linkm_command(dev,LINKM_CMD_I2CTRANS, 2,6, cmdbuf, recbuf))){		//Send I2C command
		fprintf(stderr,"error on cmd: %s\n",linkm_error_msg(err));
		return result;
	};
	result.x.component.msb=recbuf[0];
	result.x.component.lsb=recbuf[1];
	result.y.component.msb=recbuf[2];
	result.y.component.lsb=recbuf[3];
	result.z.component.msb=recbuf[4];
	result.z.component.lsb=recbuf[5];
	return result;
};

struct PSITG3200_data read_PSITG3200(usbDevice_t* dev){
	struct PSITG3200_data result;
	int err;
	cmdbuf[0] = I2C_PSITG3200;
	cmdbuf[1] = 0x1D;
	if((err = linkm_command(dev,LINKM_CMD_I2CTRANS, 2,6, cmdbuf, recbuf))){		//Send I2C command
		fprintf(stderr,"error on cmd: %s\n",linkm_error_msg(err));
		return result;
	};
	result.x.component.msb=recbuf[0];
	result.x.component.lsb=recbuf[1];
	result.y.component.msb=recbuf[2];
	result.y.component.lsb=recbuf[3];
	result.z.component.msb=recbuf[4];
	result.z.component.lsb=recbuf[5];
	return result;
}

void print_ADXL345(struct ADXL345_data data){
	printf("ADXL345: %d, %d, %d\n",data.x.data,data.y.data,data.z.data);
};

void print_HMC5843(struct HMC5843_data data){
	printf("HMC5843: %d, %d, %d\n",data.x.data,data.y.data,data.z.data);
}

void print_PSITG3200(struct PSITG3200_data data){
	printf("PSITG3200: %d, %d, %d\n",data.x.data,data.y.data,data.z.data);
}

void read_devices(usbDevice_t* dev){
	static unsigned int seq=0;
	seq++;
	print_ADXL345(read_ADXL345(dev));
	print_HMC5843(read_HMC5843(dev));
	print_PSITG3200(read_PSITG3200(dev));
	printf("SEQ: %d\n",seq);
}

int main (int argc, char **argv){

	usbDevice_t *dev;
	int err;
	if( (err = linkm_open( &dev )) ) {
		fprintf(stderr, "Error opening LinkM: %s\n", linkm_error_msg(err));
		exit(1);
	};

	memset(recbuf,0,16);
	memset(cmdbuf,0,10);
	
	if(!init_sensors(dev)){
		fprintf(stderr, "Error initializing sensors...");
		exit(2);
	};

	for(;;)	read_devices(dev);

	linkm_close(dev);
};
