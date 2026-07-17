/*
 * $Id: chttpdconfig.h,v 1.2 2005/10/19 16:41:30 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
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

#ifndef __chttpdconfig__
#define __chttpdconfig__

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

using namespace std;

#define CHTTPD_CONFIGFILE "/var/tuxbox/chttpd.conf"
#define PRIVATEDOCUMENTROOT "/var/tuxbox/httpd"
// #define PUBLICDOCUMENTROOT "/var/tmp/httpd"

#ifdef ECOS_OS
#define PUBLICDOCUMENTROOT "/mnt"
#else
#define PUBLICDOCUMENTROOT "/media"
#endif

class chttpdConfig
{
public:
	bool AutoStart;
	int Port;
	bool THREADS;
	bool MustAuthenticate;
	bool Verbose;
	bool Log;
	string PrivateDocumentRoot;
	string PublicDocumentRoot;
	string AuthUser;
	string AuthPassword;

	chttpdConfig()
	{
		AutoStart = false;
		Port = 8085;
		THREADS = true;
		Verbose = false;
		Log = false;
		MustAuthenticate = false;
		PrivateDocumentRoot = PRIVATEDOCUMENTROOT;
		PublicDocumentRoot = PUBLICDOCUMENTROOT;
		AuthUser = "root";
		AuthPassword = "luckyserver";
	}
	~chttpdConfig() {}
};
#endif
