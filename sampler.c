/***************************************************************************
 *   Copyright (C) 2012 by Michael Ambrus                                  *
 *   ambrmi09@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

// Test reading from sysfs to see what happens when/if EOF is reached.
void test_fread_sysfs( void ) {
	FILE *in;
	char line[160];
	memset(line,0,80); 
	
	//in=fopen("/sys/devices/system/cpu/cpu1/cpufreq/cpuinfo_cur_freq","r");
	in=fopen("/dev/stdout","r");
	//in=fopen("/dev/ttyUSB0","r");

	if (in==NULL) {
		perror("Something wrong with the file:");
		exit(1);
	}
	while (!feof(in) && !ferror(in)) {
		//fscanf(in,"%180c",line);
		//fread(line,180,1,in);
		fgets(line,180,in);
		printf("%s",line);
	}
	printf("Finished reading?\n");
}

// Test reading from sysfs to see what happens when/if EOF is reached.
void test_read_sysfs( void ) {
	int in,rc=0;
	char line[160];
	memset(line,0,80);
	struct stat tstat;
	struct timespec *atim,*mtim,*ctim;
	struct tm ttime;
	//strftime
	//asctime
	//ctime
	char *times;
	struct timeval now;
	
	in=open("/sys/devices/system/cpu/cpu1/cpufreq/cpuinfo_cur_freq",O_RDONLY);
	//in=open("/dev/ttyUSB0",O_RDONLY);

	if (in<0) {
		perror("Something wrong with the file:");
		exit(1);
	}
	while (rc>=0) {
		gettimeofday(now, NULL);
		rc=fstat(in,&tstat);
		if (rc<0) {
			perror("Syscall fstat reurned error:");
			exit(1);
		} else {
			atim=&(tstat.st_atim);
			mtim=&(tstat.st_mtim);
			ctim=&(tstat.st_ctim);
			
			times=ctime(&(atim->tv_sec));
			//times=ctime(now.tv_sec);
			printf("atime: %s",times);
			times=ctime(&(mtim->tv_sec));
			printf("mtime: %s",times);
			times=ctime(&(ctim->tv_sec));
			printf("ctime: %s",times);
		}

		rc=pread(in,line,160,0);
		//read(in,line,160);
		printf("%s %d \n",line,rc);
	}
}

int sampler_init(const char *filename, int n) {
	int rc;
	regex_t preg;
	char tstr[80];
	char estr[80];
	char rstr[80];
	regmatch_t mtch_idxs[80];
	int so,eo;

	test_read_sysfs();

	memset(tstr,0,80); 
	memset(estr,0,80); 
	memset(rstr,0,80); 
/*
	rc=regcomp(&preg, "\\(.*\\)",
		REG_EXTENDED);
*/	
	rc=regcomp(&preg, filename,
		REG_EXTENDED);
	if (rc) {
		regerror(rc, &preg, estr, 80);
		fprintf(stderr, "Regcomp faliure: %s\n", estr);
		exit(1);
	}
	scanf("%80c\n",tstr);
	printf("You entered: %s\n",tstr);
	/*
	int regexec(const regex_t *preg, const char *string, size_t nmatch,
					regmatch_t pmatch[], int eflags);
	*/
	rc=regexec(&preg, tstr, 80,
					mtch_idxs, /*REG_NOTBOL|REG_NOTEOL*/ 0);
	if (rc) {
		regerror(rc, &preg, estr, 80);
		fprintf(stderr, "Regcomp faliure: %s\n", estr);
		exit(1);
	}
	so=mtch_idxs[n].rm_so;
	eo=mtch_idxs[n].rm_eo;
	strncpy(rstr, &(tstr[so]), eo-so);
	printf("Your embedded data is: %s\n", rstr);

	//Dear gcc, shut up
	return 1;
}
