/*
 * CPlatform.h
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#ifndef CPLATFORM_H_
#define CPLATFORM_H_

#if defined(_WIN32)
#	include <windows.h>
#else // __linux__
#	include <unistd.h>
#	include <pthread.h>
#endif

#endif /* CPLATFORM_H_ */
