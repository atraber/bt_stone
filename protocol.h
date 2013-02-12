/*
 * protocol.h
 *
 *  Created on: 01.12.2012
 *      Author: Appl
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

void protocol(unsigned char packet[], unsigned int size, Word_t LCID, unsigned int BluetoothStackID);
void send_port2_status(int port_stat);

int port2_poll();

#endif /* PROTOCOL_H_ */
