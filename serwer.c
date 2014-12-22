/*
 *  Autor: Kasjan Siwek
 *  Indeks: 306827
 *  Data: 20.12.2014
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <pthread.h>
#include "err.h"
#include "message.h"

#define MAX_WORKING_THREADS 100

int ilosc_watkow = 0;
int processed, eligible, wazne, niewazne;
int requestsId, permissionsId, attendanceId, votesId, confirmationId, mainReportId, reportId;
int *global_kandydat;
unsigned int l, k, m;
pthread_mutex_t global_mutex, data_mutex;
pthread_cond_t na_zasoby;
typedef struct {
	pid_t pid;
	int list;
} report_t;

void lock(pthread_mutex_t *mutex) {
	if (pthread_mutex_lock(mutex) != 0) {
		fatal("mutex lock failed");
	}
}

void unlock(pthread_mutex_t *mutex) {
	if (pthread_mutex_unlock(mutex) != 0) {
		fatal("mutex lock failed");
	}
}

void inc_threads() {
	lock(&global_mutex);
	++ilosc_watkow;
	unlock(&global_mutex);
}

void dec_threads() {
	lock(&global_mutex);
	--ilosc_watkow;
	unlock(&global_mutex);
}

void *komisja(void *data) {
	lock(&global_mutex);
	while (ilosc_watkow >= MAX_WORKING_THREADS)
		pthread_cond_wait(&na_zasoby, &global_mutex);
	unlock(&global_mutex);
	
	pid_t pid = *(pid_t *)data;
	free(data);
	int kandydat[l][k];
	int i, j;
	
	for (i = 0; i < l; ++i)
		for (j = 0; j < k; ++j)
			kandydat[i][j] = 0;
	
	AttendanceMsg attendance;
	VotesMsg votes;
	ConfirmationMsg confirmation;
	
	if (msgrcv(attendanceId, &attendance, AttendanceMsgSize, (long) pid, 0) != AttendanceMsgSize) {
		fatal("msgrcv failed");
	}
	
	i = attendance.i;
	j = attendance.j;
	
	int data_in = 1;
	int w = 0;
	int sum = 0;
	
	while (data_in) {
		if (msgrcv(votesId, &votes, VotesMsgSize, (long) pid, 0) != VotesMsgSize) {
			data_in = 0;
			fatal("msgrcv failed");
		}
		
		if (votes.n < 0 || votes.l < 0 || votes.k < 0) {
			data_in = 0;
			continue;
		}
		
		++w;
		sum += votes.n;
		if (votes.l > 0 && votes.l <= l && votes.k > 0 && votes.k <= k)
			kandydat[votes.l-1][votes.k-1] += votes.n;
	}
	
	confirmation.sum = sum;
	confirmation.w = w;
	confirmation.msg_type = (long) pid;
	
	if (msgsnd(confirmationId, &confirmation, ConfirmationMsgSize, 0) != 0) {
		fatal("msgsnd failed");
	}
		
	int iter1 = 0;
	int iter2 = 0;
	lock(&data_mutex);
	eligible += i;
	wazne += sum;
	niewazne += (j - sum);
	for (iter1 = 0; iter1 < l; ++iter1) {
		for (iter2 = 0; iter2 < k; ++iter2) {
			global_kandydat[iter1 * l + iter2] += kandydat[iter1][iter2];
		}
	}
	unlock(&data_mutex);
	dec_threads();
}

void *report(void *data) {
	/* Celem ulatwienia sprawdzenia:
	 * Tak, wysylam dane dotyczace wszystkich list, nawet jesli program
	 * raport chce dane o tylko jednej z nich. Bardzo nieelegnackie,
	 * ale juz nie zdaze napisac porzadnej wersji. Z reszta w moim 
	 * to nie jest jedyne nieeleganckie miejsca w tym kodzie.
	 * */
	
	lock(&global_mutex);
	while (ilosc_watkow >= MAX_WORKING_THREADS)
		pthread_cond_wait(&na_zasoby, &global_mutex);
	unlock(&global_mutex);
	
	report_t rep = *(report_t *)data;
	free(data);
	
	int kandydat[l][k];
	int i, j;
	MainReportMsg mainrep;
	mainrep.msg_type = (long) rep.pid;

	lock(&data_mutex);
	mainrep.ilosc_komisji = m;
	mainrep.niewazne = niewazne;
	mainrep.przetworzone = processed;
	mainrep.eligible = eligible;
	mainrep.wazne = wazne;
	mainrep.k = k;
	mainrep.l = l;
	
	for (i = 0; i < l; ++i)
		for (j = 0; j < k; ++j)
			kandydat[i][j] = global_kandydat[i * l + j];
	unlock(&data_mutex);
	
	if (msgsnd(mainReportId, &mainrep, MainReportMsgSize, 0) != 0) {
		fatal("msgsnd failed");
	}
	for (i = 0; i < l; ++i) {
		for (j = 0; j < k; ++j) {
			ReportMsg new_rep;
			new_rep.msg_type = (long) rep.pid;
			new_rep.k = j;
			new_rep.l = i;
			new_rep.n = kandydat[i][j];
			
			if (msgsnd(reportId, &new_rep, ReportMsgSize, 0) != 0) {
				fatal("msgsnd failed");
			}
		}
	}
	
	ReportMsg new_rep;
	new_rep.msg_type = (long) rep.pid;
	new_rep.k = -1;
	new_rep.l = -1;
	new_rep.n = -1;
	
	if (msgsnd(reportId, &new_rep, ReportMsgSize, 0) != 0) {
		fatal("msgsnd failed");
	}
	
	dec_threads();
}

void turn_off(int sig) {
	
	if (msgctl(requestsId, IPC_RMID, 0) == -1)
		fatal("Failed to delete requests queue");
	if (msgctl(permissionsId, IPC_RMID, 0) == -1)
		fatal("Failed to delete permissions queue");
	if (msgctl(attendanceId, IPC_RMID, 0) == -1)
		fatal("Failed to delete attendance queue");
	if (msgctl(votesId, IPC_RMID, 0) == -1)
		fatal("Failed to delete votes queue");
	if (msgctl(confirmationId, IPC_RMID, 0) == -1)
		fatal("Failed to delete confirmation queue");
	if (msgctl(mainReportId, IPC_RMID, 0) == -1)
		fatal("Failed to delete confirmation queue");
	if (msgctl(reportId, IPC_RMID, 0) == -1)
		fatal("Failed to delete confirmation queue");
	
	pthread_cond_destroy(&na_zasoby);
	pthread_mutex_destroy(&global_mutex);
	pthread_mutex_destroy(&data_mutex);
	exit(0);
}

int main (int argc, char *argv[])
{
    if (argc != 4) {
        syserr("Usage: ./serwer <L> <K> <M>");
        exit(0);
    }
    
    if (signal(SIGINT, turn_off) == SIG_ERR)
		syserr("Failed to set SIGINT");

    l = atoi(argv[1]);
    k = atoi(argv[2]);
    m = atoi(argv[3]);

    if (l < 1|| l > 99) {
        fatal("L has to be 0 < L < 100");
        exit(0);
    }

    if (k < 1|| k > 99) {
        fatal("K has to be 0 < K < 100");
        exit(0);
    }

    if (m < 1|| m > 9999) {
        fatal("M has to be 0 < M < 10000");
        exit(0);
    }

    int i, j;
	int serwer_on = 1;
    int komisje[m];
	processed = 0;
	global_kandydat = malloc(l * k * sizeof(int));

    for (i = 0; i < m; ++i)
        komisje[i] = 0;
	
	for (i = 0; i < l; ++i)
		for (j = 0; j < k; ++j)
			global_kandydat[i*l + j] = 0;

	RequestMsg requestMsg;

    if ((requestsId = msgget(REQUESTS_MKEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("msgget failed");
    if ((permissionsId = msgget(PERMISSIONS_MKEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("msgget failed");
    if ((attendanceId = msgget(ATTENDANCE_MKEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("msgget failed");
    if ((votesId = msgget(VOTES_MKEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("msgget failed");
	if ((confirmationId = msgget(CONFIRMATION_MKEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("msgget failed");
	if ((mainReportId = msgget(MAINREP_MKEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("msgget failed");
	if ((reportId = msgget(REP_MKEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
        syserr("msgget failed");

	if (pthread_mutex_init(&global_mutex, 0) != 0)
		syserr("mutex_init failed");
	
	if (pthread_mutex_init(&data_mutex, 0) != 0)
		syserr("mutex_init failed");
	
	if (pthread_cond_init(&na_zasoby, 0) != 0)
		syserr("cond_init failed");


	pthread_t thread;
	pthread_attr_t thread_attr;
	
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
			
	while (serwer_on) {
		if (msgrcv(requestsId, &requestMsg, RequestMsgSize, 1L, 0) != RequestMsgSize && serwer_on) {
			fatal("msgrcv failed");
		}
		
		inc_threads();
		pid_t *id = malloc(sizeof(pid_t));
		*id = requestMsg.mypid;
		
		if (requestMsg.m > 0) {
			lock(&global_mutex);
			if (komisje[requestMsg.m] == 0) {
				++komisje[requestMsg.m];
				++processed;
				unlock(&global_mutex);
				PermissionMsg permission;
				permission.msg_type = (long) requestMsg.mypid;
				permission.permission = 1;
				if (msgsnd(permissionsId, &permission, PermissionMsgSize, 0) != 0)
					fatal("failed to snd msg");
				
				pthread_create(&thread, &thread_attr, komisja, (void *)id);
			} else {
				unlock(&global_mutex);
				PermissionMsg permission;
				permission.msg_type = (long) requestMsg.mypid;
				permission.permission = 0;
				if (msgsnd(permissionsId, &permission, PermissionMsgSize, 0) != 0)
					fatal("failed to snd msg");
			}
		} else {
			report_t *rep = malloc(sizeof(report_t));
			rep->list = requestMsg.l;
			rep->pid = requestMsg.mypid;
			
			pthread_create(&thread, &thread_attr, report, (void *)rep);
		}
	}
	
	free(global_kandydat);
    return 0;
}
