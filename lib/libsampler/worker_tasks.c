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

/* Worker tasks go in this file (i.e. what they actually do), there might
 * be different variations of tasks, unless they become too many all will be
 * kept here.
 *
 * A "task" is work of a certain "modality", i.e. it represent a, possibly
 * specialized or optimized, way for a certain signal to harvest it's data.
 * Modality for a certain signal lives for as long as sampler lives and is
 * connected to the signal via a function-pointer in the sig_sub struct.
 * Modalities could had been implemented as threads. It's however much more
 * flexible and maintainable to separate the engine from it's actual tasks
 * if possible. In sampler the work-tasks don't need separate scheduling
 * principles or stack-sizes and can be treated equal from all aspects
 * except from the details of what they do.  */

#define LDATA struct smpl_signal
#include <mlist.h>
#include <stdio.h>
#include <assert.h>
#include <assure.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <regex.h>
#include <parsers.h>
#include <sys/select.h>
#include <sys/param.h>

/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"
#ifndef VERBOSE_TYPE
#define VERBOSE_TYPE 3
#endif

#ifndef TESTSPEED
#define TESTSPEED 5
#endif
#include "local.h"

#ifndef SIMULATE_WTASK_BLOCKED_ERROR
#define SIMULATE_WTASK_BLOCKED_ERROR 0
#endif

#define BUF_MAX (1024 * 16)
#define MAX_SUBLINES 1024

#ifndef USE_SIG_ARRAY
#define USE_SIG_ARRAY 'n'
#endif

#if !defined(__ANDROID__)
# define BUF_FRMTR "\n{%s}\n"
#else
# define BUF_FRMTR "%s"
#endif

#ifndef MIN
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

/* Last Character in Buffer */
#define LCB (buf[strnlen(buf,BUF_MAX)-1])

#define SIGSUB_FSEEK_SET( OFFS ) \
	sig_sub->fp_was = sig_sub->fp_curr; \
	assert_np( fseek(sig_sub->tfile, OFFS, SEEK_SET) == 0); \
	sig_sub->fp_curr = ftell(sig_sub->tfile);


/* Keep following constant as short as possible but long enough (2uS mean
   value is enough on host but needs worst-case calibration on target). If
   constant is too long and if buffer contain more than one non-relevant
   line, it will delay detection that the event belongs to someone else.
 */
#define SELECT_TIMEOUT_USEC 1000

#ifndef MIN
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

inline
void static buf2val(
	struct sig_sub *sig_sub,/* Set value in this sub_sig */
	const char *bufS,   	/* From this buffer */
	regmatch_t *idxs,		/* Using start, stop indexes from here */
	int vsize            	/* Target value size-max */
) {
	int n = sig_sub->idx;	/* For the n'th sub-expression */
	int ss_len;             /* Sub-string length */
	int i;
	int nonEOLfound;
	char *tval_ptr;
	int cpy_sz = vsize;
	struct val_msg val_msg;

	if (idxs)
		ss_len = (MATCH_END(idxs,n) - MATCH_START(idxs,n));
	else
		if (sig_sub->isstr)
			ss_len=strnlen(bufS,BUF_MAX);
		else
			ss_len=strnlen(bufS,vsize);

	if (sig_sub->isstr) {
		tval_ptr = sig_sub->valS;
		/* If this bails, that means harvest hasn't finished using valS and
		 * not freed and we'd have a mem-leak, as when it does finish, it
		 * would free the wrong address (i.e. the one malloced here) */
		assert_np(tval_ptr != NULL);
		tval_ptr=malloc(ss_len+1);
		memset(tval_ptr,0,ss_len+1);
		if (sig_sub->val_pipe)
			/* Send message instead of update output directly */
			val_msg.valS = tval_ptr;
		else 
			sig_sub->valS = tval_ptr;

		cpy_sz=ss_len+1;
	} else {
		if (sig_sub->val_pipe) {
			val_msg.valS = malloc(vsize);
			tval_ptr = val_msg.valS;
		} else
			tval_ptr = sig_sub->val;
		memset(tval_ptr,0,vsize);
		cpy_sz=vsize;
	}

	if (idxs!=NULL)
		strncpy(
			tval_ptr,
			&bufS[MATCH_START(idxs,n)],
			MIN( cpy_sz-1, ss_len)
		);
	else
		strncpy(tval_ptr, bufS, cpy_sz);

	if (sig_sub->isstr)
		tval_ptr[ss_len]=0;
	else
		tval_ptr[vsize]=0;

#ifdef PARSER_REMOVE_EOLS
	/* Remove any trailing EOL characters */
	for(
		i=strnlen(tval_ptr,cpy_sz)-1, nonEOLfound=0;
		i>0 && !nonEOLfound;
		i--
	) {
		if (
			(tval_ptr[i] == 0x0D) ||
			(tval_ptr[i] == 0x0A) ||
			(tval_ptr[i] == '\n')
		) {
			tval_ptr[i]=0;
		} else
			nonEOLfound=1;
	}
#endif //PARSER_REMOVE_EOLS
	if (sig_sub->val_pipe) {
		val_msg.len=sig_sub->isstr?ss_len:ss_len;
		assert_ext(mq_send(sig_sub->val_pipe->write, 
			(char*)&val_msg, 
			sizeof(struct val_msg), 1) != (mqd_t)-1);
	}
}

inline
static int transfer_regmatch(struct sig_sub* sig_sub, const char *buf) {
	struct sig_data* sig_data = sig_sub->ownr;
	struct sig_def* sig_def = &sig_data->ownr->sig_def;
	int rc=0;
	int j=0;

	regmatch_t mtch_idxs[MAX_MTCH_PERLINE+1];
	char err_str[80];

	switch (sig_def->sops.bits.parser) {
		case PARS_CUT:
			rc=cutexec(&sig_def->rgx_sig.str[0], buf, MAX_MTCH_PERLINE+1,
				mtch_idxs, 0);
			break;
		case PARS_REVCUT:
			rc=rcutexec(&sig_def->rgx_sig.str[0], buf, MAX_MTCH_PERLINE+1,
				mtch_idxs, 0);
			break;
		case PARS_REGEXP:
		default:
			rc=regexec(&sig_def->rgx_sig.rgx, buf, MAX_MTCH_PERLINE+1,
				mtch_idxs, 0);
	}

	if (rc) {
		regerror(rc, &sig_def->rgx_sig.rgx, err_str, 80);
		DBG_INF(0,("Regex failure (%s) on buff:"BUF_FRMTR, err_str, buf));
		RETURN(-10);
	} else {
		if (sig_def->sops.bits.tops == 0) {
			buf2val(sig_sub, buf, mtch_idxs, VAL_STR_MAX);
			j++;
		} else {
#if (USE_SIG_ARRAY == 'y')
#warning Using sig_sub array. This is mechanism is obsolete.
			DBG_INF(7,("Picking values from buff (array-ver): "BUF_FRMTR,
				buf));
			for (j=0; j<sig_data->nsigs; j++) {
				struct sig_sub* sig_sub_inferior;
				sig_sub_inferior = &(sig_data->sig_sub[j]);
				buf2val(sig_sub_inferior, buf, mtch_idxs, VAL_STR_MAX);
				DBG_INF(7,("val[%d]: %s\n", j, sig_sub_inferior->val));
				if (strnlen(sig_sub_inferior->val,VAL_STR_MAX)>0) {
					sig_sub_inferior->is_updated = 1;
				} else {
					sig_sub_inferior->is_updated = 0;
					DBG_INF(7,("Bad match idx %d for string: "BUF_FRMTR,
						sig_def->idxs[j], buf));
				}

			}
#else
			struct node *n;
			handle_t tmp_list;
			assert_np(mlist_dup(&tmp_list, sig_data->sub_list) == 0);

			DBG_INF(7,("Picking values from buff (list-ver): "BUF_FRMTR,
				buf));

			for(
				n=mlist_head(tmp_list);
				n;
				n=mlist_next(tmp_list)
			) {
				assert_ext(n->pl);
				buf2val((struct sig_sub *)(n->pl),
						buf, mtch_idxs, VAL_STR_MAX);
				j++;

				if (strnlen(((struct sig_sub *)(n->pl))->val,
					VAL_STR_MAX)>0) {
					((struct sig_sub *)(n->pl))->is_updated = 1;
				} else {
					((struct sig_sub *)(n->pl))->is_updated = 0;
					DBG_INF(7,("Bad match idx %d for string: "BUF_FRMTR,
						sig_def->idxs[j], buf));
				}
			}
			assert_np(mlist_close(tmp_list) == 0);
#endif
		}
	}
	return j;
}

/* Wait for data to be ready (consistent with the notify event) */
static uint32_t sync_rbuff(int fd) {
	struct timeval tv;
	struct timeval t0;
	struct timeval t1;
	struct timeval td;
	fd_set readfds;
	int rc;

	tv.tv_sec=0;
	tv.tv_usec=SELECT_TIMEOUT_USEC;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	assert_np(time_now(&t0) == 0);
	assert_ext((
		rc=select(FD_SETSIZE, &readfds, NULL, NULL, &tv)
	) != -1);
	assert_np(time_now(&t1) == 0);
	if (rc==0) {
		return SELECT_TIMEOUT_USEC;
	}
	td=tv_diff(t0,t1);
	assert_np(FD_ISSET(fd,&readfds));
	return (td.tv_sec%1000)*1000000+td.tv_usec;
}

#if defined( __GLIBC__ )
pid_t gettid( void ) {
	return syscall(SYS_gettid);
}
#endif

/*
 * Specialized harvester for log-files. I.e. whence opened, never close and
 * never fseek (except to push back incomplete read lines if needed). Read
 * until *one* hit is reached. Rely on the fact that the event generator has
 * put several events in queue in case buffer contains more hits than one.
 * Also handle logics when awoken, but bad hit (i.e. possible
 * fractioned line) or events belonging to someone else (other iNotify or
 * time-event).
 * */
int logfile_fdata(struct sig_sub* sig_sub, int cnt) {
	struct sig_data* sig_data = sig_sub->ownr;
	struct sig_def* sig_def = &sig_data->ownr->sig_def;
	int rc;
	char buf[BUF_MAX];
	char *cline;
	int found = 0, nlines = 0, posted_events=0;
	long int mark_pos,curr_pos;
	int pending_posts;

	/* Just a sanity check. Proves iNotify fires off way to often when GDB
	 * is running the code. Possible to optimize out but good to have for
	 * now */
	sem_getvalue(&sampler_data.master_event, &pending_posts);

	memset(buf,0,BUF_MAX);

	if (sig_sub->fd < 0) {
		sig_sub->fd=open(sig_def->fdata, O_RDONLY);
		rc=errno;
		assert(sig_sub->fd != -1);
		assert_np((sig_sub->tfile = fdopen(sig_sub->fd, "r")) >= 0);
		sig_sub->fp_curr = 0;
		sig_sub->fp_valid = 0;
		sig_data->cleared_backlog=0;
		if (sig_sub->fd < 0) {
			if (sig_def->sops.bits.always) {
				sprintf(buf,"Error: Can't open data-file for read: %s",
						sig_def->fdata); perror(buf);
				assert_ext(buf == NULL);
			}
			RETURN(-11);
		}
	}

	mark_pos=ftell(sig_sub->tfile);
	if (mark_pos < 0) {
		if (sig_sub->tfile != NULL) {
			assert_ext(fclose(sig_sub->tfile)==0);
			rc=errno;
			sig_sub->fd=-1;
			sig_sub->tfile=NULL;
		};
		sig_sub->fp_curr = 0;
		sig_sub->fp_valid = 0;
		sig_data->cleared_backlog=0;
		if (sig_sub->fd == -1) {
				sprintf(buf,"Warning: Data-file [%s] has stopped existing "
					"or is renamed signal is lost. Will try again next run.",
					sig_def->fdata);
			if (sig_def->sops.bits.always) {
				perror(buf);
				assert_ext(buf == NULL);
			} else {
				DBG_INF(1,("%s\n",buf));
			}
			RETURN(-12);
		}
	}

	if (feof(sig_sub->tfile)) {
		DBG_INF(2,("EOF passed! Fixing it...\n"));
		SIGSUB_FSEEK_SET(sig_sub->fp_curr);
	}
	curr_pos=ftell(sig_sub->tfile);
	do {
		int sync_usec;

		sync_usec=sync_rbuff(sig_sub->fd);

		if (sync_usec <= SELECT_TIMEOUT_USEC) {
			DBG_INF(6,("[%s]: Continues after waiting %duS "
				"for read sync\n",
				sig_def->name,sync_usec));
		} else {
			DBG_INF(1,("[%s]: select time-out (%duS) "
				"(no more data in buffer)\n",
				sig_def->name,sync_usec));
		}

		/* Scan from last current file-pointer of until EOF */
		mark_pos=curr_pos;
		cline=fgets(buf, BUF_MAX, sig_sub->tfile);
		curr_pos=ftell(sig_sub->tfile);
		sig_sub->fp_curr=curr_pos;
		nlines++;
		DBG_INF(8,("[%s,%lu]: Read {"BUF_FRMTR"}\n",
				sig_def->name, curr_pos, buf));
		if (regexec(&sig_def->rgx_line.rgx, buf, 0, NULL, 0) == 0)
			found = 1;
	} while (cline && !found);

	if (sig_sub->fp_valid == ftell(sig_sub->tfile)) {
		DBG_INF(2,("FP not advancing, not our call. Get out ASAP"));
		found = 0;
	}

	if (!found) {
		/*
		 * If last line read doesn't end with a valid EOL, then it's a time-out
		 * buffer read and it needs to be pushed back as it might contain the
		 * first part of a valid pattern.
		 */
		if (cline && (LCB != '\n')) {
			DBG_INF(2,("Last read was not a complete line 3(3) "
				"(0x%02X):"BUF_FRMTR, LCB, buf));
			SIGSUB_FSEEK_SET(mark_pos);
			RETURN(-1);
		}
		if (cline) {
			sig_sub->fp_curr=ftell(sig_sub->tfile);
			DBG_INF(3,("[%s]: Thread run in vain for:%s\n",
				sig_def->name, buf));
		} else {
			DBG_INF(2,("[%s]: Thread run totally unnecessary.\n",
				sig_def->name));
		}

		RETURN(-2);
	} else {
		/*
		 * Even if line-regex hit's, line can still be fractioned. Reject
		 * if so (it will be last in buffer and must be re-read when buffer
		 * is complete).
		 */
		if (cline && (LCB != '\n')) {
			DBG_INF(2,("Line regex matched correct but line not complete "
				"(0x%02X):"BUF_FRMTR, LCB, buf));
			SIGSUB_FSEEK_SET(sig_sub->fp_valid);
			RETURN(-3);
		}
	}
	DBG_INF(3,("Worker [%d] read %d lines. Now processing:"BUF_FRMTR,
		sig_sub->id, nlines, buf));

	if ((sig_data->backlog_events>0) && (sig_sub->sub_id==0)) {
			sig_data->backlog_events--;
			sig_data->cleared_backlog++;
			DBG_INF(3,("[%s]: Caught up one back-logged line. "
				"Remaining backlog-events: %d. Curr line:"BUF_FRMTR,
				sig_def->name, sig_data->backlog_events-1, buf));
	}
	if ( !sig_def->rgx_sig.str ||
		 strnlen(sig_def->rgx_sig.str, BUF_MAX) == 0)
	{
		/* Simplest case. Whole line contains data */
		/* Get rid of last LF is such exists */
		if (buf[strlen(buf) -1] == '\n')
			buf[strlen(buf) -1] = '\0';
		buf2val(sig_sub, buf, NULL, VAL_STR_MAX);
	} else {
		int rc;
		regmatch_t mtch_idxs[MAX_MTCH_PERLINE+1];
		char err_str[80];

		rc=regexec(&sig_def->rgx_sig.rgx, buf, MAX_MTCH_PERLINE+1,
			mtch_idxs, 0);
		if (rc) {
			regerror(rc, &sig_def->rgx_sig.rgx, err_str, 80);
			DBG_INF(0,("Regex failure (%s) on buff:"BUF_FRMTR, err_str, buf));
			assert_ext((rc==0) && ("Is your line-regexp unique enough?"!=NULL));
			RETURN(-10);
		} else {
			buf2val(sig_sub, buf, mtch_idxs, VAL_STR_MAX);
		}
	}

	/* Harvesting done, on to the next phase: Detect if there's more.
	 * Do this with all threads as it moves their FP:s along too*/
	sig_sub->fp_valid = curr_pos;
	DBG_INF(2,("Harvest done. FP is now at [%lu], value read was [%s]\n",
		(unsigned long)curr_pos, sig_sub->val));

	/* Detect if we're currently working-off back-log, if so don't continue
	 * to search for more matches, rely on the fact that if file has grown
	 * there will be extra iNotify events. Note that since we're comparing
	 * against value in sig_data, this is racy for sub_sigs, who might run
	 * one time to many (high likely-hood), but it doesn't matter as they
	 * won't produce any more data and no events anyway, only fw their FP.
	 * The cost is one extra fseek per sub_sig thread (cheap).
	 * */
	if (sig_data->backlog_events>0) {
		/* Return all-OK */
		RETURN(0);
	}

	if (sig_sub->sub_id==0) {
		sig_data->cleared_backlog=0;
	}
	DBG_INF(7,("[%d]: Continues traversing file searching for more valid "
		"patterns. Last line read:"BUF_FRMTR, sig_sub->id,buf));

	for (
		nlines = 0, found=0, mark_pos=ftell(sig_sub->tfile),cline="0xCACA";
		//!feof(sig_sub->tfile);
		cline;
		nlines++)
	{
		/* Catch-up with the rest of the file, there could be more data. If
		 * so, send events to master equal line-matches found. If not, just
		 * forward FP as it does no harm */
		curr_pos=mark_pos;
		mark_pos=ftell(sig_sub->tfile);
		cline=fgets(buf, BUF_MAX, sig_sub->tfile);
		if  (cline==NULL) {
			DBG_INF(1,("[%s]: Breaking out after %d catch-up lines\n",
				sig_def->name, nlines));
		}

		if ( cline && (regexec(&sig_def->rgx_line.rgx, buf, 0, NULL, 0)) == 0) {
			found++;
			/* Post self-event. I.e. let the master know there are more
			 * valid data for the notification). */
			if (sig_sub->sub_id==0) {
				/* Make sure only sig_data thread-leader does this or there
				 * will be (N-1)*M too many posts */
				DBG_INF(3,("[%s,%d]: Detected more valid line:%s",
					sig_def->name, nlines, buf));
				assert_ext(sem_post(&sampler_data.master_event) == 0);
				posted_events++;
			}
		}
	};

	if (sig_sub->sub_id==0) {
		sig_data->backlog_events+=posted_events;
	}

	if (nlines>1){
		if (LCB != '\n') {
			DBG_INF(2,("Last read was not a complete line 2(3) "
				"(0x%02X):"BUF_FRMTR, LCB, buf));
			SIGSUB_FSEEK_SET(curr_pos);
		}
	}
	if (found) {
		if (sig_sub->sub_id==0) {
			/* Pollute log only with thread-leader */
			DBG_INF(3,("Found %d extra matching patterns. Last line read: %s\n",
				posted_events, buf));
		}
		/* If found: need to back all the way to the position right after last
		 * succeeded match. If not: (ideal) leave fp as close to EOF as
		 * possible */
		SIGSUB_FSEEK_SET(sig_sub->fp_valid);
	}
	sig_sub->fp_curr = ftell(sig_sub->tfile);
	RETURN(0);
}

/* Normal read-file function. Read interpret data from file. Very simplified
 * version, no lseek, no block/unblock-driving events e.t.a. */
int poll_fdata(struct sig_sub* sig_sub, int cnt) {
	struct sig_data* sig_data = sig_sub->ownr;
	struct sig_def* sig_def = &sig_data->ownr->sig_def;
	int rc=0;
	char buf[BUF_MAX];

	if (sig_sub->sub_id != 0)
		if (sig_def->sops.bits.tops != NO_THREAD_SUB_SIG) {
			/* This modality can handle sig-subs in-thread. Self-terminate
			   all but leader, let leader do the work */
			DBG_INF(9,("   [%d] Self terminating\n",sig_sub->id));
			assert_np(pthread_detach(pthread_self()) == 0);
			pthread_exit(NULL);
		}

	if (	(sig_def->sops.bits.tops == NO_THREAD_SUB_SIG) ||
			(sig_def->sops.bits.tops == NEVER_THREAD_SUB_SIG)
	) {
		struct sig_sub* sig_sub_inferior;
		struct node *n;

		for(
			n=mlist_head(sig_data->sub_list);
			n;
			n=mlist_next(sig_data->sub_list)
		) {
			assert_ext(n->pl);
			sig_sub_inferior=(struct sig_sub *)(n->pl);
			memset(sig_sub_inferior->val, 0, VAL_STR_MAX);
			sig_sub_inferior->is_updated = 0;
		}
	}

	if (sig_sub->fd < 0) {
		sig_sub->fd=open(sig_def->fdata, O_RDONLY);
		rc=errno;
		assert(sig_sub->fd != -1);
		if (sig_sub->fd < 0) {
			if (sig_def->sops.bits.always) {
				sprintf(buf,"Error: Can't open data-file for read: %s",
					sig_def->fdata);
				perror(buf);
				assert_ext(buf == NULL);
			}
			RETURN(-11);
		}
	}

	if (sig_def->sops.bits.no_rewind == 0) {
		rc = lseek(sig_sub->fd, 0, SEEK_SET);
	}

	if (rc < 0) {
		if (sig_sub->tfile != NULL) {
			assert_ext(fclose(sig_sub->tfile)==0);
			rc=errno;
			sig_sub->fd=-1;
			sig_sub->tfile=NULL;
		};
		if (sig_sub->fd == -1) {
				sprintf(buf,"Warning: Data-file [%s] has stopped existing or is "
					"renamed signal is lost, will try again next period.",
					sig_def->fdata);
			if (sig_def->sops.bits.always) {
				perror(buf);
				assert_ext(buf == NULL);
			} else {
				DBG_INF(1,("%s\n",buf));
			}
			RETURN(-12);
		}
	}

	assert_np((sig_sub->tfile = fdopen(sig_sub->fd, "r")) >= 0);
	if ( !sig_def->rgx_line.str ||
		 strnlen(sig_def->rgx_line.str, BUF_MAX) == 0)
	{
		/* Simple case. Whole file or first line contains data */
		/* Need to replace this with read for whole file parsing if special
		 * cases are satisfied (TBD) */
		fgets(buf, BUF_MAX, sig_sub->tfile);
	} else {
		int i,j,k = sig_def->lindex, found=0;
		char *rstr;
		for (i=0,j=0; i<MAX_SUBLINES && !found; i++) {
			rstr=fgets(buf, BUF_MAX, sig_sub->tfile);
			assert_ext(rstr || !(*sig_def).sops.bits.always);
			if (!rstr) {
				DBG_INF(1,("Could not find matching line [%s:%d] in file %s\n",
					sig_def->rgx_line.str, k, sig_def->fdata));
				RETURN(-1);
			}
			if (regexec(&sig_def->rgx_line.rgx, buf, 0, NULL, 0) == 0) {
				/* There is a match, but is it the n'th one? */
				j++;
				found = (j==k);
			}
		}
	}

	if ( !sig_def->rgx_sig.str ||
		 strnlen(sig_def->rgx_sig.str, BUF_MAX) == 0)
	{
		/* Simplest case. Whole line contains data */
		/* Get rid of last LF is such exists */
		if (buf[strlen(buf) -1] == '\n')
			buf[strlen(buf) -1] = '\0';
		buf2val(sig_sub, buf, NULL, VAL_STR_MAX);
	} else {
		rc=transfer_regmatch(sig_sub, buf);
		if (rc<0)
			return rc;
	}

	/*Close the file but not the descriptor*/
	fclose(sig_sub->tfile);
	sig_sub->tfile=NULL;
	sig_sub->fd = -1;
	RETURN(0);
}

/*
 * First (1(3)) and simplest variant of piped harvesting.
 * Specialized harvester for pipes & character devices never fseek/lseek,
 * never close only try-open if needed.  Runs asynchronously as often as
 * pipe tells it to, but only copy to output variable(s).  Effect is
 * automatic piggy-backing on the next timer-driven sample without need for
 * sorting. Temporal accuracy is good enough for many cases.  Side-effect:
 * If something driving the shabang doesn't come, fast changing data away,
 * data in-between will be lost by design. In that case better let pipe
 * drive event (trigger), or output special out-of sync data.
 *
 * */
int blocked_reader(struct sig_sub* sig_sub, int cnt) {
	struct sig_data* sig_data = sig_sub->ownr;
	struct sig_def* sig_def = &sig_data->ownr->sig_def;
	int j,rc=0;
	char buf[BUF_MAX];
	handle_t tlist;
	struct node *np;

	if (sig_sub->sub_id != 0)
		assert_ext("This modality can only operate single-threaded" == NULL);
	
	assert_ext(pthread_mutex_lock(&sampler_data.mx_master_up) == 0);
	/*Tell the next guy is OK (if is any)*/
	assert_ext(pthread_mutex_unlock(&sampler_data.mx_master_up) == 0);


	for(j=0; j<sig_data->nsigs; j++){
		sig_sub=&((sig_data->sig_sub)[j]);
		if (sig_sub->val_pipe) {
			INFO(("%s: Initializing sending value-mqueue\n",
				sig_sub->name));
			assert_ext((sig_sub->val_pipe->write = mq_open(
				sig_sub->name,
				O_WRONLY,
				OPEN_MODE_REGULAR_FILE, NULL)) != (mqd_t)-1);
		}
	}

	if (sig_def->fdata[0] != '<')
		assert_ext((sig_sub->tfile = fopen(sig_def->fdata, "r")) != 0);
	else
		assert_ext((sig_sub->tfile = proc_subst_in(sig_def->fdata)) != 0);

	fseek(sig_sub->tfile,0,SEEK_END);

	assert_ext(sem_post(&sampler_data.pipes_complete)== 0);


	/*This test is put outside of loop, not for beauty but for performance.*/
	if ( !sig_def->rgx_line.str ||
		 strnlen(sig_def->rgx_line.str, BUF_MAX) == 0)
	{
		while (1) {
			fgets(buf, BUF_MAX, sig_sub->tfile);
			pthread_mutex_lock(&sig_sub->lock);
			sig_sub->is_updated=0; /* Only this can safely be used to marker */
			DBG_INF(7,("Piped input-line%s\n",buf));
			if (transfer_regmatch(sig_sub, buf) > 0) {
				if (sig_def->sops.bits.trigger) {
				   /* Self-trigger event. Shabang will follow, do DAQ
					  according to rules - and finally as a side-effect
					  transfer piped-in to out. Note: data can still be lost
					  if shabang is not fast enough. This however is not by
					  design and needs improvement (queues) */
					assert_ext(sem_post(&sampler_data.master_event) == 0);
				} /* else: Wait for lock-step master to piggy-back output.
					 Note that in this modality piped data input can be
					 missed (i.e. not propagated to output), this is by
					 design.*/
			}
			//usleep(10);
			pthread_mutex_unlock(&sig_sub->lock);
		}
	} else {
		while (1) {
			fgets(buf, BUF_MAX, sig_sub->tfile);
			pthread_mutex_lock(&sig_sub->lock);
			sig_sub->is_updated=0;
			if (regexec(&sig_def->rgx_line.rgx, buf, 0, NULL, 0) == 0) {
				DBG_INF(7,("Piped input-line%s\n",buf));
				if (transfer_regmatch(sig_sub, buf) > 0) {
					if (sig_def->sops.bits.trigger) {
						assert_ext(sem_post(&sampler_data.master_event) == 0);
					}
				}
			}
			pthread_mutex_unlock(&sig_sub->lock);
			/*Else skip and hurry back to continue read/block*/
		}
	}
}

/*
 * Second (2(3)) variant of piped harvesting.
 * Same as blocked_reader above, but also triggers shabang.
 *
 * Note that harvest is still asynchronous and that if piped data is very
 * fast, temporal accuracy can still suffer is shabang can't keep up.
 */
int blocked_reader_wtrigger(struct sig_sub* sig_sub, int cnt) {
	assert_np("Not finished TBD" == NULL);
}

/*
 * Third (3(3)) and variant of piped harvesting.
 * Has nothing in common with the other. It will not trigger shabang but
 * will drive full output on it's own. All other values will contain marker
 * for invalidity but which differs from NO_SIG handling.
 *
 * Time for such entry till be time-stamped with either the incomming time,
 * or if TS-field exists and settings are such, it will replace the sample
 * time-stamp. To prevent data comming out of time order, final actuation of
 * output needs be buffered and sorted w.r.t. to time-stamp.
 *
 */
int blocked_reader_gotrough(struct sig_sub* sig_sub, int cnt) {
	assert_np("Not finished TBD" == NULL);
}

