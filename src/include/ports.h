/*
 * ports.h
 *
 *  Created on: Nov 16, 2017
 *      Author: jzhu62
 */

#ifndef PORTS_H_
#define PORTS_H_
#include <types.h>


void outportb (uint16_t p_port,uint8_t data);
void outportw (uint16_t p_port,uint16_t data);
void outportl (uint16_t p_port,uint32_t data);
uint8_t inportb( uint16_t p_port);
uint16_t inportw( uint16_t p_port);
uint32_t inportl( uint16_t p_port);

#endif /* PORTS_H_ */
