/*
 *  Autor: Kasjan Siwek
 *  Indeks: 306827
 *  Data: 20.12.2014
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "message.h"
#include "err.h"


int main (int argc, char *argv[]) {
    if (argc != 2) {
        fatal("Usage: ./komisja <M>");
        exit(0);
    }

    unsigned int m = atoi(argv[1]);

    if (m < 1|| m > 9999) {
        fatal("M has to be 0 < M < 10000");
        exit(0);
    }

    int requestId, permissionId, confirmationId, attendanceId, votesId;
    RequestMsg request;
	PermissionMsg permission;
    pid_t mypid = getpid();

    if ((requestId = msgget(REQUESTS_MKEY, 0)) == -1)
        syserr("msgget failed");
    if ((permissionId = msgget(PERMISSIONS_MKEY, 0)) == -1)
        syserr("msgget failed");
	if ((attendanceId = msgget(ATTENDANCE_MKEY, 0)) == -1)
		syserr("msgget failed");
	if ((confirmationId = msgget(CONFIRMATION_MKEY, 0)) == -1)
		syserr("msgget failed");
	if ((votesId = msgget(VOTES_MKEY, 0)) == -1)
		syserr("msgget failed");

	request.msg_type = 1L;
	request.mypid = mypid;
	request.m = m;
	
    if (msgsnd(requestId, &request, RequestMsgSize, 0) != 0)
        syserr("Failed to send message");
	
	if (msgrcv(permissionId, &permission, PermissionMsgSize, (long) mypid, 0) != PermissionMsgSize)
		syserr("Failed to get message");
	
	if (permission.permission == 0) {
		printf("Odmowa dostępu\n");
		return 0;
	}
	
    int i, j, k, l, n;
    scanf("%d %d\n", &i, &j);
	AttendanceMsg attendance;
	attendance.i = i;
	attendance.j = j;
	attendance.msg_type = (long) mypid;
	
	if (msgsnd(attendanceId, &attendance, AttendanceMsgSize, 0) != 0)
        syserr("Failed to send message");
	
	VotesMsg votes;
    char* line = NULL;
    size_t len = 0;
    while (getline(&line, &len, stdin) != -1) {
        if (sscanf(line, "%d %d %d", &l, &k, &n) != 3) {
            fatal("Za malo danych w wierszu");
        }
        
        votes.k = k;
		votes.l = l;
		votes.n = n;
		votes.msg_type = (long) mypid;
		
		if (msgsnd(votesId, &votes, VotesMsgSize, 0) != 0)
			syserr("Failed to send message");
	}
    
	votes.k = -1;
	votes.l = -1;
	votes.n = -1;
	votes.msg_type = (long) mypid;
	
	if (msgsnd(votesId, &votes, VotesMsgSize, 0) != 0)
		syserr("Failed to send message");
	
	ConfirmationMsg confirmation;
	
	if (msgrcv(confirmationId, &confirmation, ConfirmationMsgSize, (long) mypid, 0) != ConfirmationMsgSize)
		syserr("Failed to get confirmation message");
	
	printf("Przetworzonych wpisów: %d\n", confirmation.w);
	printf("Uprawnionych do głosowania: %d\n", i);
	printf("Głosów ważnych: %d\n", confirmation.sum);
	printf("Głosów nieważnych: %d\n", j - confirmation.sum);
	float attend = (float) j / (float) i;
	attend *= 100;
	printf("Frekwencja w lokalu: %f\n", attend);

    free(line);

    return 0;
}
