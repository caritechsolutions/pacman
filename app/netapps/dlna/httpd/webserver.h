/*
 * $Id: webserver.h,v 1.3 2005/10/18 19:20:34 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 * based on nhttpd (C) 2001/2002 Dirk Szymanski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __webserver__
#define __webserver__

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <map>

#ifdef ECOS_OS
#include <pkgconf/posix.h>
#include <cyg/posix/pthread.h>
#else
#include <pthread.h>
#endif

#include "chttpdconfig.h"
using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

class CWebserverRequest;

typedef bool (*SendFileFunc)(void *arg, CWebserverRequest *request);
struct CSendFile {
	void *arg;
	SendFileFunc func;
};

typedef std::map<std::string, CSendFile> CFileList;

class CWebserver {
private:
	int ListenSocket;
	pthread_t server_thread;
public:
	chttpdConfig cfg;
	bool STOP;
	CFileList FileList;

	CWebserver();
	~CWebserver();

	bool Start();
	void DoLoop();
	void Stop();
	void AddFile(const std::string filename, SendFileFunc fs, void *arg) {
		if (fs) {
			FileList[filename].arg = arg;
			FileList[filename].func = fs;
		}
	}
	bool SendFile(CWebserverRequest *request, const std::string filename) {
		CFileList::iterator iT = FileList.find(filename);

		if (iT != FileList.end())
			return iT->second.func(iT->second.arg, request);

		return false;
	}
	void Wait() { pthread_join(server_thread, NULL);}
};

bool WebServerRun(void);

#endif
