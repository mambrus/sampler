/***************************************************************************
 *   Copyright (C) 2013 by Michael Ambrus                                  *
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "assert_np.h"
#include <limits.h>
#include "local.h"
//#define PIPE_TEST_1
//#define PIPE_TEST_2
//#define PROC_SUBST_USE_NAMED_PIPES

/* Creates a unique malloced name based on process ID thread ID and time
 * suitable for filenames.  The first two are enough for uniqueness between
 * executable elements, the last is mostly only practical, but if uniqueness
 * is required within the same thread, it's needed.
 *
 * Note that name is created from dynamic memory. I.e. it's necessary to
 * destroy the pointer returned whence you're done with it.
 *
 * Use this method of uniqueness if you find value in the implicit
 * structure and order. Alternatives:
 * tmpnam()
 * tempnam()
 *
 * Warning: Uniqueness is only guaranteed between threads. Within the same
 * thread you need to make sure the time between invocations is longer than
 * the smallest time-resolution. Depending on the time source used, this
 * might be as low as one jiffie. On system with support for hires-clocks
 * (TMS e.t.a.) this is in practice not a problem as the resolution is much
 * higher that the sys-call time it takes to read it.
 * */
char *create_unique_name(){
	char name[PATH_MAX]={'\0'};
	struct timeval tno1;

	assert_ign(time_now(&tno1) == 0);

	snprintf(name, PATH_MAX,
		"%04d.%04d.%06d.%08d",
		getpid(), gettid(), SEC(tno1),USEC(tno1));

	return strndup(name,PATH_MAX);
}

/* Creates a pipe is sampler temporary directory. Returns the full name of
 * the fifo which is created from dynamic memory. I.e. name  has to be
 * destroyed whence done even when fifo itself is not. */
static char *create_fifo(){
	char full_name[PATH_MAX]={'\0'};
	char *name;

	name = create_unique_name();
	snprintf(full_name,PATH_MAX,"%s/p_sampler_%s",
		sampler_setting.tmpdir, name);

	free(name);
	assert_ext(mkfifo(full_name, 0777) == 0);
	return strndup(full_name,PATH_MAX);
}

/* transforms cmdline to cmd and sentinel ended char table argv. argv will
 * be created from dynamic memory and must be freed when done, strings in it
 * will however be reused from *cmdline which is hence destructed in the
 * process */
static int cmdln2cmdargs(char *cmd, char **argv[], char *cmdln) {
	int i,j,instr,argc=1;
	int len = strnlen(cmdln, PATH_MAX);

	for (i=0; i<len; i++) {
		if (cmdln[i] == ' '){
			argc++;
			cmdln[i] = '\0';
		};
	}
	*argv=calloc(argc+1,sizeof(char*));
	strncpy(cmd,cmdln,strnlen(cmdln,PATH_MAX));

	for(i=0,instr=0,j=0; i<len; i++) {
		if (!instr && cmdln[i]) {
			instr=1;
			(*argv)[j++]=&cmdln[i];
		}
		if (!cmdln[i])
			instr=0;
	}
	(*argv)[j++]=NULL;
	return 0;
}

/*Fork and execv, dup2 the stdin/out arguments and redirect to named pipe */
static int proc_start(char *out_to_name, char *cmdline) {
	int childpid;
	char **exec_args;
	char cmd[PATH_MAX]={'\0'};
	int out_fd;

	cmdln2cmdargs(cmd, &exec_args, cmdline);
#if !defined(PROC_SUBST_USE_NAMED_PIPES)
	int un_pipe[2];
	assert_np(pipe(un_pipe) != -1);
#endif
	assert_ext((childpid = fork()) >= 0);

	if (childpid == 0) {
		int i;
		/* Child excutes this */
		DBG_INF(1,("Child runs!\n"));
		close(1);
#ifdef PROC_SUBST_USE_NAMED_PIPES
		assert_np(out_fd=open(out_to_name,O_WRONLY) != -1);
#else
		DBG_INF(1,("Unnamed pipe fd used: %d\n",un_pipe[1]));
		assert_np((out_fd=dup(un_pipe[1])) == 1);
#endif
		DBG_INF(1,("Child output at descriptor %d\n",out_fd));
		DBG_INF(1,("cmd: %s\n",cmd));
		for (i=0; exec_args[i]; i++)
			DBG_INF(1,("  %s\n",exec_args[i]));

#if !defined(PROC_SUBST_USE_NAMED_PIPES)
		close(un_pipe[0]);
		close(un_pipe[1]);
#endif
#ifdef PIPE_TEST_1
		while (1) {
			int i=0;
			DBG_INF(1,("Child test write to pipe \n"));
			printf("Test write to pipe %d\n",i);
			sleep(1);
		}
#endif
		execvp(cmd,exec_args);

		/* Should never execute */
		perror("exec error");
		exit(-1);
  	}
	/* Parent executes this */
	DBG_INF(1,("Parent has just created sub-proc %d\n",childpid));
	free(exec_args);
#if !defined(PROC_SUBST_USE_NAMED_PIPES)
	close(un_pipe[1]);
	out_fd=un_pipe[0];
	return out_fd;
#else
	return 0;
#endif
}

/* process substitute. Returns a valid open *FILE if successful, else NULL.
 * 1) I.e. fifo is created (named or unnamed)
 * 2) Process is started
 * 3) If all is successful so far (both creation of fifo and process start)
 *    a valid FILE will be returned.
 *
 * Note that even if started and starting to write to pipe, it will block
 * until we start reading.
 * */
FILE *proc_subst_in(char *cmds) {
	char *fifo_name;
	FILE *retfile=NULL;
	char *tcmds = strndup(cmds,PATH_MAX);
	int i,j,len = strnlen(tcmds,PATH_MAX);
	char strips[]="<()";
	int in_fd;

#ifdef PROC_SUBST_USE_NAMED_PIPES
	fifo_name = create_fifo();
#endif

	for (i=0; i<len; i++)
		for (j=0; j<strlen(strips); j++)
			if (tcmds[i] == strips[j])
				tcmds[i]='\0';

	for (i=0; !tcmds[i] && i<len; i++);

#ifdef PROC_SUBST_USE_NAMED_PIPES
	in_fd=proc_start(fifo_name, &tcmds[i]);
	if (in_fd!=0)
		goto proc_subst_in_end;
	retfile=fopen(fifo_name,"r");
	free(fifo_name);
#else
	in_fd=proc_start(fifo_name, &tcmds[i]);
	if (in_fd < 0)
		goto proc_subst_in_end;

	retfile=fdopen(in_fd,"r");
#endif
#ifdef PIPE_TEST_2
	while (!feof(retfile)) {
		int i=0;
		char buff[1000];

		DBG_INF(1,("Parent %d read pipe\n,gettid()"));
		printf("Read pipe %s\n",buff);
		fgets(buff,1000,retfile);
		printf("Parent %d read: %s",gettid(),buff);
	}
#endif

proc_subst_in_end:
	free(tcmds);
	return retfile;
}
