/*
 * protocol.h
 *
 *  Created on: 01.12.2012
 *      Author: Appl
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

void protocol(unsigned int BluetoothStackID, Word_t LCID, unsigned char packet[], unsigned int size);
void send_port2_status(int port_stat);

void port2_poll();

void connectionOpened(unsigned int BluetoothStackID, Word_t LCID);

void connectionClosed();

#endif /* PROTOCOL_H_ */
