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
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include <sampler.h>
#include <mlist.h>

#define DEF_PERIOD 100000

#define xstr(S) str(S)
#define str(S) #S

const char *program_version =
"sampler 0.01";

#define HELP_USAGE     1
#define HELP_LONG      2
#define HELP_VERSION   4
#define HELP_TRY       8
#define HELP_EXIT     16
#define HELP_EXIT_ERR 32

void help(FILE* file, int flags) {
	if (file && flags & HELP_USAGE) {
		fprintf(file, "%s",
			"Usage: sampler [-DlvhuV] [-d level] [-f char] [-T time] [-s file]\n"
			"            [-x value] [-t dir] \n"
			"            [--debuglevel=level] [--documentation] [--delimeter=char]\n"
			"            [--legend] [--period=time] [--sigs_file=file] [--verbose]\n"
			"            [-E {1,0,\"yes\",\"no\"}] [--inheritenv={1,0,\"yes\",\"no\"}]\n"
			"            [-e string[,string]] [--environment=string[,string]]\n"
			"            [-p path[,path]] [--path=path[:path]] [--tmpdir=dir]\n"
			"            [--disconnect=value] [--help] [--usage] [--version]\n");
		fflush(file);
	}
	if (file && flags & HELP_LONG) {
		fprintf(file, "%s",
			"Usage: sampler [OPTION...] \n"
			"sampler -- A rule-based utility to produce vectorized "
			"data-samples from system\n"
			"internals at certain intervals or at specific events\n"
			"\n"
			"Common options:\n"
			"  -d, --debuglevel=level     Debug-level\n"
			"  -D, --documentation        Output full documentation, then exit\n"
			"  -f, --delimeter=char       Change delimeter\n"
			"  -l, --legend               Produce legend only, then exit\n"
			"  -R, --realtime             Run sampling in real-time mode (requires\n"
			"                             root-privileges)\n"
			"  -T, --period=time          Period-time in uS. -1 equals event-driven output.\n"
			"                             Default period-time is [100000] uS\n"
			"  -s, --sigs_file=file       Signals description file (MANDATORY)\n"
			"  -v, --verbose              Produce verbose output. This flag lets you see how\n"
			"                             sigs_file file is interpreted\n"
			"  -x, --disconnect=value     Set a new default value (or string) to use as\n"
			"                             sample default magic value when signal is detected\n"
			"                             not to be either missing or not updated (or\n"
			"                             connected)\n"
			"  -t, --tmpdir=dir           Directory for temporary files (named pipes e.t.a.).\n"
			"                             Alternative way to runt-time set is via environment\n"
			"                             variable SAMPLER_TMPDIR. If both are set, flag \n"
			"                             overrides environment. If neither is set, directory\n"
			"                             falls back to hard-coded default:\n"
			"                             ["xstr(SAMPLER_TMPDIR)"].\n"
			"                             If SAMPLER_TMPDIR is set during compile time, it\n"
			"                             also becomes the hard-coded fall-back. Having this\n"
			"                             set as environment variable is convenient when \n"
			"                             chaining or cascading several samplers.\n"
			"                             Note that for remote chained samplers, both -t and an\n"
			"                             entry for -e may be needed. If systems differ,\n"
			"                             tmpdir may also be different\n"
			"Process substitution:\n"
			"  -E, --inheritenv=val       If children for process substitution \n"
			"                             inherit environment. Valid values are \"yes\",\n"
			"                             \"no\",1,0. Default value is \"yes\" (1) \n"
			"  -e, --environment=string   Environment to be either added or used as is\n"
			"                             depending on -E. Specified either by several \n"
			"                             invocations of the same flag or a list with\n"
			"                             settings delimited by ',' \n"
			"  -p, --path=string          Path to be appended to sub-shells PATH environment\n"
			"                             Note: must be set (recommended practice) if \n"
			"                             binary is not fully qualified and not in standard\n"
			"                             PATH on remote host (most likely). Specified \n"
			"                             by several invocations of the same flag or a list\n"
			"                             with settings delimited by ':' \n"
			"                             NOTE: will always be appended (hence differs from\n"
			"                             shell) and always appended before current PATH\n"
			"                             If PATH is needed clean, -E must be used (-Eno)\n"
			"Special:\n"
			"  -n, --daemon               Run as a daemon\n"
			"  -h, --help                 Give this help list\n"
			"  -u, --usage                Give a short usage message\n"
			"  -V, --version              Print program version\n"
			"\n"
			"Mandatory or optional arguments to long options are also mandatory or optional\n"
			"for any corresponding short options.\n"
			"\n"
			"Read the manual by passing the -D option\n"
			"\n"
			"Report bugs to <SG-SELD-SW-CI-C-G@sonymobile.com>.\n");
		fflush(file);
	}

	if (file && flags & HELP_VERSION) {
		fprintf(file, "%s\n", program_version);
		fflush(file);
	}

	if (file && flags & HELP_TRY) {
		fprintf(file, "%s",
			"Try `sampler --help' or `sampler --usage' for more information.\n");
		fflush(file);
	}

	if (file && flags & HELP_EXIT)
		exit(0);

	if (file && flags & HELP_EXIT_ERR)
		exit(1);
}

extern struct module_sampler_setting sampler_setting;
extern struct mlistmod_settings mlistmod_settings;

/* General arguments */
struct arguments
{
	int legend, verbose, abort;   /* ‘-s’, ‘-v’, ‘--abort’ */
	char *sigs_file;              /* file arg to ‘--sigs_file’ */
	int ptime;                    /* time arg to ‘--period’ */
	int debuglevel;               /* level arg to ‘--debug’ */
	int daemon;
};

static void addstr2ptable(char ***table, const char *str) {
	int i,sz;
	char **ptr, **nptr;
	for (i=1, ptr=*table; *ptr; ptr++,i++);
	nptr=calloc(i+1, sizeof(char*));
	memcpy(nptr, *table, i*sizeof(char*));
	nptr[i-1]=strndup(str,PATH_MAX);
	nptr[i]=NULL;
	free(*table);
	*table=nptr;
}

static void addstrs2ptable(char ***table, const char *strs, char delim) {
	int i,instr;
	char *tstr = strndup(strs,PATH_MAX);
	int slen=strnlen(strs,PATH_MAX);

	for(i=0; tstr[i]; i++)
		tstr[i] = tstr[i]==delim ? 0 : tstr[i];

	for(i=0,instr=0; i<slen; i++) {
		if (!instr && tstr[i]) {
			instr=1;
			addstr2ptable(table, &tstr[i]);
		}
		if (!tstr[i])
			instr=0;
	}
	free(tstr);
}

/* Parse a single option. */
static void parse_opt(
	const char *cmd,
	int key,
	char *arg,
	struct arguments *arguments
){
	extern const char sampler_doc[];

	switch (key) {
		case 's':
			arguments->sigs_file = arg;
			break;
		case 'v':
			arguments->verbose = 1;
			sampler_setting.verbose = 1;
			mlistmod_settings.verbose = 1;
			break;
		case 'T':
			arguments->ptime = arg ? atoi (arg) : -1;
			break;
		case 'R':
			sampler_setting.realtime         = 1;
			break;
		case 'l':
			sampler_setting.legendonly       = 1;
			break;
		case 'M':
			sampler_setting.policy_master    =
				arg ? atoi (arg) : INT_MAX;
			break;
		case 'W':
			sampler_setting.policy_workers   =
				arg ? atoi (arg) : INT_MAX;
			break;
		case 'Y':
			sampler_setting.policy_events    =
				arg ? atoi (arg) : INT_MAX;
			break;
		case 'm':
			sampler_setting.prio_master      =
				arg ? atoi (arg) : INT_MAX;
			break;
		case 'w':
			sampler_setting.prio_workers     =
				arg ? atoi (arg) : INT_MAX;
			break;
		case 'y':
			sampler_setting.prio_events      =
				arg ? atoi (arg) : INT_MAX;
			break;

		case 't':
			free(sampler_setting.tmpdir);
			sampler_setting.tmpdir=strndup(arg,PATH_MAX);
			break;
		case 'x':
			strncpy(sampler_setting.presetval,arg,VAL_STR_MAX);
			break;
		case 'f':
			sampler_setting.delimiter = arg[0];
			break;
		case 'd':
			arguments->debuglevel = arg ? atoi (arg) : 0;
			sampler_setting.debuglevel = arguments->debuglevel;
			mlistmod_settings.debuglevel = arguments->debuglevel;
			break;
		case 'D':
			printf("%s\n",sampler_doc);
			exit(0);
			break;
		case 'E':
			if (strncmp(arg,"yes",3 == 0))
				sampler_setting.procsubst.inherit_env=1;
			else if (strncmp(arg,"no",2 == 0))
				sampler_setting.procsubst.inherit_env=0;
			else
				sampler_setting.procsubst.inherit_env=atoi(arg);

			if ((sampler_setting.procsubst.inherit_env != 0) &&
			   (sampler_setting.procsubst.inherit_env != 1))
			{
				fprintf(stderr, "%s: option `-%c' given invalid argument\n",
					cmd, optopt);
				help(stderr, HELP_TRY | HELP_EXIT_ERR);
			}
			break;
		case 'e':
			addstrs2ptable(&sampler_setting.procsubst.env, arg, ',');
			break;
		case 'p':
			addstrs2ptable(&sampler_setting.procsubst.path, arg, ':');
			break;

		case 'u':
			help(stdout, HELP_USAGE | HELP_EXIT);
			break;

		case 'h':
			help(stdout, HELP_LONG | HELP_EXIT);
			break;

		case '?':
			/* getopt_long already printed an error message. */
			help(stderr, HELP_TRY | HELP_EXIT_ERR);
			break;

		case ':':
			/* getopt_long already printed an error message. */
			fprintf(stderr, "%s: option `-%c' requires an argument\n",
				cmd, optopt);
			help(stderr, HELP_TRY | HELP_EXIT_ERR);
			break;

		case 'V':
			help(stdout, HELP_VERSION | HELP_EXIT);
			break;

		case 'n':
			arguments->daemon = 1;
			break;

		default:
			fprintf(stderr, "sampler: unrecognized option '-%c'\n", (char)key);
			help(stderr, HELP_TRY | HELP_EXIT_ERR);
			break;
		}
}

static struct option long_options[] = {
	{"verbose",       no_argument,       0, 'v'},
	{"realtime",      no_argument,       0, 'R'},
	{"policy_master", required_argument, 0, 'M'},
	{"policy_workers",required_argument, 0, 'W'},
	{"policy_events", required_argument, 0, 'E'},
	{"prio_master",   required_argument, 0, 'm'},
	{"prio_workers",  required_argument, 0, 'w'},
	{"prio_events",   required_argument, 0, 'y'},
	{"legend",        no_argument,       0, 'l'},
	{"delimeter",     required_argument, 0, 'f'},
	{"debuglevel",    required_argument, 0, 'd'},
	{"disconnect",    required_argument, 0, 'x'},
	{"period",        required_argument, 0, 'T'},
	{"tmpdir",        required_argument, 0, 't'},
	{"sigs_file",     required_argument, 0, 's'},
	{"inheritenv",    required_argument, 0, 'E'},
	{"environment",   required_argument, 0, 'e'},
	{"path",          required_argument, 0, 'p'},
	{"documentation", no_argument,       0, 'D'},
	{"usage",         no_argument,       0, 'u'},
	{"help",          no_argument,       0, 'h'},
	{"version",       no_argument,       0, 'V'},
	{"daemon",        no_argument,       0, 'n'},
	{0, 0, 0, 0}
};

/* Returns 0 on success, -1 on error */
int become_daemon()
{
	int fd;

	switch (fork()) {
	case -1: return -1;
	case 0:  break;
	default: _exit(EXIT_SUCCESS);
	}

	if (setsid() == -1)
		return -1;

	switch (fork()) {                   /* Ensure we are not session leader */
	case -1: return -1;
	case 0:  break;
	default: _exit(EXIT_SUCCESS);
	}

	umask(0);                       /* Clear file mode creation mask */
	chdir("/");                     /* Change to root directory */

	return 0;
}

int main(int argc, char **argv) {
	struct arguments arguments;

	/* Some defaults set before options are parsed */
	arguments.verbose        = 0;
	arguments.legend         = 0;
	arguments.debuglevel     = 0;
	arguments.ptime          = DEF_PERIOD;
	arguments.sigs_file      = NULL;
	arguments.daemon         = 0;

	/* Set if corresponding  environment variable is set, else does nothing
	   Gives a nice priority of how these variables are set: I.e. flags
	   have precedence over environment, environment has precedence over
	   hard-coded defaults. */
	SETFROMENV( SAMPLER_TMPDIR, sampler_setting.tmpdir, PATH_MAX);

	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv,
			"s:vT:t:x:f:ld:DuhVRM:W:Y:m:w:y:E:e:p:n",
			long_options,
			&option_index);
		/* Detect the end of the options. */
		if (c == -1)
			break;
		parse_opt(argv[0], c, optarg, &arguments);
	}
	/* Handle any remaining command line arguments (not options). */
	if (optind < argc) {
		perror("sampler: Too many arguments, sampler takes only options.\n");
		fflush(stderr);
		help(stderr, HELP_TRY | HELP_EXIT_ERR);
	}

	if (!arguments.sigs_file) {
		fprintf(stderr,"sampler: syntax error! Mandatory option missing.\n\n");
		fflush(stderr);
		help(stderr, HELP_TRY | HELP_EXIT_ERR);
	}

	if (arguments.daemon) {
		become_daemon();
	}
	sampler(arguments.sigs_file, arguments.ptime);
	return 0;
}

