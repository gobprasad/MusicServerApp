/*************************************************************
 * gsocket.h : Socket Implementation
 * Copyright (C) 2014  Gobind Prasad (gobprasad@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *
 ************************************************************/


#ifndef __GSOCKET_H__
#define __GSOCKET_H__
#include "results.h"
int createServerSocket(char *localAdd, int port);
int createClientSocket(char *serverAdd, int port);
unsigned int receiveData(int sockFd, unsigned int size, char *buf);
unsigned int sendData(int sockFd, unsigned int size, char *buf);
void closeSocket(int sockFd);
RESULT setSocketBlockingEnabled(int fd, char blocking);

#endif //__GSOCKET_H__
