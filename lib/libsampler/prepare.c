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

/* Prepare a run by doing sanity-checks and optimizations */

#include <stdio.h>
#include <assert.h>
#include <assure.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <mqueue.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <sys/types.h>
#ifndef __ANDROID__
#include <sys/capability.h>
int capset(cap_user_header_t hdrp, const cap_user_data_t datap);
#endif
#include <linux/capability.h>


#if _LINUX_CAPABILITY_VERSION != 0x19980330
	#error "The linux/capability.h is not libcap v1"
#endif

#define LDATA struct sig_sub
#include <mlist.h>

/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"
#include "local.h"
#define XTRA_LEN (sizeof("/_NN.XXXXX")) /* Prefix & Suffix identifier size for
                                           a signal Q. I.e. "_NN" */

#define CHECK_RT_SETTING( S )                                                  \
if ( sampler_setting.prio_##S != INT_MAX )                                     \
	if ( sampler_setting.policy_##S  == INT_MAX ) {                            \
		fprintf(stderr,                                                        \
			"Error: Setting a priority while assuming a policy for "#S"\n");   \
		return EINVAL;                                                         \
	}                                                                          \
	if ( sampler_setting.policy_##S  != INT_MAX ) {                            \
			switch (sampler_setting.policy_##S) {                              \
				case SCHED_OTHER:                                              \
					break;                                                     \
				case SCHED_FIFO:                                               \
				case SCHED_RR:                                                 \
					break;                                                     \
				default:                                                       \
					fprintf(stderr,                                            \
						"Error: Unknown or unsupported policy requested "      \
						"[%d] for "#S"\n",                                     \
						sampler_setting.policy_##S);                           \
					return EINVAL;                                             \
					break;                                                     \
			}                                                                  \
	}                                                                          \

/* Pre-check if requested run-settings are reasonable or even possible*/
static int check_rt_settings() {
	int uid, euid;
#ifndef __ANDROID__
	cap_t cap;
	cap = cap_init();
#endif

	if (sampler_setting.realtime) {
		int pfifo_higher_greater = 0;
		int prr_higher_greater = 0;

		if ( sched_get_priority_max(SCHED_FIFO) >
			 sched_get_priority_min(SCHED_FIFO) )
			pfifo_higher_greater = 1;
		if ( sched_get_priority_max(SCHED_RR) >
			 sched_get_priority_min(SCHED_RR) )
			prr_higher_greater = 1;

		sampler_setting.policy_master  = SCHED_FIFO;
		sampler_setting.policy_events  = SCHED_FIFO;
		sampler_setting.policy_workers = SCHED_RR;

		if (pfifo_higher_greater) {
			sampler_setting.prio_events = sched_get_priority_max(SCHED_FIFO) -1;
			sampler_setting.prio_master = sched_get_priority_max (SCHED_FIFO)-2;
		} else {
			sampler_setting.prio_events = sched_get_priority_max(SCHED_FIFO) +1;
			sampler_setting.prio_master = sched_get_priority_max(SCHED_FIFO) +2;
		}
		if (prr_higher_greater) {
			sampler_setting.prio_workers = sched_get_priority_max(SCHED_RR) -2;
		} else {
			sampler_setting.prio_workers = sched_get_priority_max(SCHED_RR) +2;
		}
	}

	uid = getuid();
	euid = geteuid();
	INFO(("UID: %d EUID: %d\n", uid, euid));

	CHECK_RT_SETTING( master );
	CHECK_RT_SETTING( workers);
	CHECK_RT_SETTING( events );

	if (
		(sampler_setting.policy_master    != INT_MAX) ||
		(sampler_setting.policy_workers   != INT_MAX) ||
		(sampler_setting.policy_events    != INT_MAX) ||
		(sampler_setting.prio_master      != INT_MAX) ||
		(sampler_setting.prio_workers     != INT_MAX) ||
		(sampler_setting.prio_events      != INT_MAX) )
	{
		/* Check that process will be able to deploy this */
		if (euid != 0) {
			fprintf(stderr, "ERROR: you need to be root or euid root to be able to "
				"modify RT properties. Exiting...\n");
			return -1;
		}
		/* If so, make sure that we do */
		struct __user_cap_header_struct header;
		struct __user_cap_data_struct cap;
		header.version = _LINUX_CAPABILITY_VERSION;
		header.pid = 0;
		cap.effective = cap.permitted =
			(1 << CAP_IPC_OWNER) |
			(1 << CAP_SYS_NICE);
		cap.inheritable = 1;
		if ( capset(&header, &cap) ) {
			DBG_INF(0,("Failed to request extra capabilities. %s.", strerror(errno)));
			return -1;
		}
	}
#ifndef __ANDROID__
	cap = cap_get_proc();
	if (sampler_setting.verbose) {
		INFO(("Running with capabilities: %s\n", cap_to_text(cap, NULL)));
		cap_free(cap);
	}
#endif
	return 0;
}

#define PINFO_STRUCT_ENUM( STRUCT, M )							\
	INFO(("  %s: {%d} \n", #M, STRUCT.M));						\

#define PINFO_STRUCT_INT( STRUCT, M )							\
	INFO(("  %s: [%d] \n", #M, STRUCT.M));						\

#define PINFO_STRUCT_XINT( STRUCT, M )							\
	INFO(("  %s: [0x%X] \n", #M, STRUCT.M));					\

#define PINFO_STRUCT_CHAR( STRUCT, M )							\
	INFO(("  %s: '%c' \n", #M, STRUCT.M));						\

#define PINFO_STRUCT_STR( STRUCT, M )							\
	INFO(("  %s: \"%s\" \n", #M, STRUCT.M));					\


/* Same as above but for vectors */
#define PINFO_STRUCT_VENUM( STRUCT, M , I )						\
	INFO(("  %s[%d]: {%d}\n", #M, I, STRUCT.M[I]));				\

#define PINFO_STRUCT_VINT( STRUCT, M , I )						\
	INFO(("  %s[%d]: [%d]\n", #M, I, STRUCT.M[I]));				\

#define PINFO_STRUCT_VCHAR( STRUCT, M , I )						\
	INFO(("  %s[%d]: '%c'\n", #M, I, STRUCT.M[I]));				\

#define PINFO_STRUCT_VSTR( STRUCT, M , I )						\
	INFO(("  %s[%d]: \"%s\"\n", #M, I, STRUCT.M[I]));			\

static void print_module_settings() {
	char **ptr;
	int i;

	INFO(("SAMPLER module-settings:"));
	PINFO_STRUCT_INT(  sampler_setting, ptime );
	PINFO_STRUCT_ENUM( sampler_setting, clock_type);
	PINFO_STRUCT_ENUM( sampler_setting, plotmode);
	PINFO_STRUCT_CHAR( sampler_setting, delimiter);
	PINFO_STRUCT_INT(  sampler_setting, dolegend);
	PINFO_STRUCT_INT(  sampler_setting, legendonly);
	PINFO_STRUCT_INT(  sampler_setting, debuglevel);
	PINFO_STRUCT_INT(  sampler_setting, verbose);
	PINFO_STRUCT_STR(  sampler_setting, presetval);
	PINFO_STRUCT_STR(  sampler_setting, tmpdir);
	PINFO_STRUCT_INT(  sampler_setting, realtime);
	PINFO_STRUCT_XINT( sampler_setting, policy_master);
	PINFO_STRUCT_XINT( sampler_setting, policy_workers);
	PINFO_STRUCT_XINT( sampler_setting, policy_events);
	PINFO_STRUCT_XINT( sampler_setting, prio_master);
	PINFO_STRUCT_XINT( sampler_setting, prio_workers);
	PINFO_STRUCT_XINT( sampler_setting, prio_events);
	PINFO_STRUCT_INT(  sampler_setting.procsubst, inherit_env);

	for (i=0,ptr=sampler_setting.procsubst.path;*ptr;ptr++,i++){
		PINFO_STRUCT_VSTR( sampler_setting.procsubst, path, i);
	}
	for (i=0,ptr=sampler_setting.procsubst.env;*ptr;ptr++,i++){
		PINFO_STRUCT_VSTR( sampler_setting.procsubst, env, i);
	}
}

/* Prepare a run by doing more necessary bindings, optimizations, further
 * checks */
int sampler_prep(handle_t list) {
	int rc,i,j,x,y;
	struct node *n;
	struct node *n2;
	struct sig_sub *sig_sub;
	struct sig_sub *sig_sub2;
	struct sig_data *sig_data;
	struct sig_data *sig_data2;
	struct smpl_signal *smpl_signal;
	struct smpl_signal *smpl_signal2;
	struct sig_def *sig_def;
	struct sig_def *sig_def2;
	handle_t list2;

	if (sampler_setting.verbose) {
		print_module_settings();
	}

	assert_ext((rc=check_rt_settings()) == 0);

	/* For signals with sub-sigs, create a list in the leader with the
	 * sub-sigs to be used if it's not sub-threading */
	for(n=mlist_head(list); n; n=mlist_next(list)){
		assert_ext(n->pl);

		smpl_signal=(struct smpl_signal *)(n->pl);
		sig_data=&((*smpl_signal).sig_data);
		sig_def=&(sig_data->ownr->sig_def);
		for(j=0; j<sig_data->nsigs; j++) {
			sig_sub=&((sig_data->sig_sub)[j]);
			if (sig_data->nsigs &&  sig_sub->sub_id == 0){

				assert_ext(mlist_dup(&list2,list) == 0);

				assert_ext(
					rc=mlist_opencreate(sizeof(struct sig_sub*),
						NULL, &sig_data->sub_list
				)==0);
				assert_ext(sig_data->sub_list);

				n2=mlist_curr(list2);
				assert_ext(n2->pl);

				smpl_signal2=(struct smpl_signal *)(n2->pl);
				sig_data2=&((*smpl_signal).sig_data);
				sig_def2=&(sig_data2->ownr->sig_def);
				for(x=0; x<sig_data2->nsigs; x++){
					sig_sub2=&((sig_data2->sig_sub)[x]);
					sig_sub2->idx = sig_def2->idxs[sig_sub2->sub_id];
					sig_sub2->isstr =
						sig_def2->issA[sig_sub2->sub_id] == 'Y' ? 1 : 0;
					sig_sub2->name = malloc(
							strnlen(sig_def2->name, PATH_MAX)+XTRA_LEN);
					sig_sub2->val_pipe = NULL;
					snprintf(sig_sub2->name, PATH_MAX,
							"/%s_%d.%d", sig_def2->name, x, getpid());
					mq_unlink(sig_sub2->name);
					assert_ext(mlist_add(sig_data->sub_list,sig_sub2) !=
							NULL);
				}
				assert_ext(mlist_close(list2) == 0);
			}

		}
	}

	if (sampler_setting.verbose) {
		for(n=mlist_head(list); n; n=mlist_next(list)) {
			assert_ext(n->pl);

			smpl_signal=(struct smpl_signal *)(n->pl);
			sig_data=&((*smpl_signal).sig_data);
			sig_def=&(sig_data->ownr->sig_def);
			sig_sub=&((sig_data->sig_sub)[0]);
			for(
				n2=mlist_head(sig_data->sub_list);
				n2;
				n2=mlist_next(sig_data->sub_list)
			) {
				assert_ext(n2->pl);
				sig_sub=(struct sig_sub *)(n2->pl);
				INFO(("Signal [%15s] has sig_sub [%d]\n",
						sig_sub->ownr->ownr->sig_def.name,
						sig_sub->id));
			}
		}
	}
}

