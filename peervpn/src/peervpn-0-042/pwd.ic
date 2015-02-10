/***************************************************************************
 *   Copyright (C) 2012 by Tobias Volk                                     *
 *   mail@tobiasvolk.de                                                    *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/


#ifdef WIN32
	static void dropPrivileges(char *username, char *groupname, char *chrootdir) {
		int error = 0;
		if(strlen(username) > 0) {
			error = 1;
		}
		if(strlen(groupname) > 0) {
			error = 1;
		}
		if(strlen(chrootdir) > 0) {
			error = 1;
		}
		if(error) {
			throwError("user/group changing not implemented in windows version!\n");
		}
	}
	static void dropPrivilegesAuto() {
		HANDLE h_process;
		HANDLE h_token;
		h_process = GetCurrentProcess();
		if(OpenProcessToken(h_process,TOKEN_WRITE,&h_token)) {
			if(AdjustTokenPrivileges(h_token, TRUE, NULL, 0, NULL, NULL)) {
			}
			CloseHandle(h_token);
		}
	}
#else


#include <pwd.h>
#include <grp.h>


// drop privileges
static void dropPrivileges(char *username, char *groupname, char *chrootdir) {
	struct passwd *pwd = NULL;
	struct group *grp = NULL;

	int swuser = 0;
	int swgroup = 0;

	if(strlen(username) > 0) {
		if((pwd = getpwnam(username)) != NULL) {
			swuser = 1;
		}
		else {
			throwError("username not found!");
		}
	}
	if(strlen(groupname) > 0) {
		if((grp = getgrnam(groupname)) != NULL) {
			swgroup = 1;
		}
		else {
			throwError("groupname not found!");
		}
	}

	if(strlen(chrootdir) > 0) if(chroot(chrootdir) < 0) throwError("chroot failed!");
	if(swgroup) if(setgid(grp->gr_gid) < 0) throwError("could not switch group!");
	if(swuser) if(setuid(pwd->pw_uid) < 0) throwError("could not switch user!");
}


// drop privileges (automatic version, without specified user and group name)
static void dropPrivilegesAuto() {
	const char usernames[2][8] = { "nobody", "nogroup" };
	const int userids[2] = { 65534, 65533 };
	struct passwd *pwd = NULL;
	struct group *grp = NULL;
	int i; int n;
	
	// group switching
	for(i=0; i<4; i++) {
		if(i >= 0 && i < 2) {
			n = (i - 0);
			grp = getgrnam(usernames[n]);
			if(grp != NULL) {
				if(!(setgid(grp->gr_gid) < 0)) {
					break;
				}
			}
		}
		if(i >= 2 && i < 4) {
			n = (i - 2);
			if(!(setgid(userids[n]) < 0)) {
				break;
			}
		}
		
	}

	// user switching
	for(i=0; i<4; i++) {
		if(i >= 0 && i < 2) {
			n = (i - 0);
			pwd = getpwnam(usernames[n]);
			if(pwd != NULL) {
				if(!(setuid(pwd->pw_uid) < 0)) {
					break;
				}
			}
		}
		if(i >= 2 && i < 4) {
			n = (i - 2);
			if(!(setuid(userids[n]) < 0)) {
				break;
			}
		}
		
	}
}


#endif

