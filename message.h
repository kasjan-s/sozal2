/*
 * Autor: Kasjan Siwek
 * Indeks: 306827
 *
 * SO, drugie zadanie zaliczeniowe
 */
#ifndef _MESSAGE_
#define _MESSAGE_

#define REQUESTS_MKEY 1357L
#define PERMISSIONS_MKEY 1337L
#define ATTENDANCE_MKEY 1470L
#define VOTES_MKEY 1234L
#define CONFIRMATION_MKEY 1222L
#define MAINREP_MKEY 1323L
#define REP_MKEY 1333L

typedef struct {
    long msg_type;
    pid_t mypid;
    int m;
	int l;
} RequestMsg;

const int RequestMsgSize = sizeof(pid_t) + 2 * sizeof(int);

typedef struct {
    long msg_type;
    pid_t mypid;
	int permission;
} PermissionMsg;

const int PermissionMsgSize = sizeof(pid_t) + sizeof(int);

typedef struct {
    long msg_type;
	int i;
	int j;
} AttendanceMsg;

const int AttendanceMsgSize = sizeof(int) * 2;

typedef struct {
    long msg_type;
	int l;
	int k;
	int n;
} VotesMsg;

const int VotesMsgSize = sizeof(int) * 3;

typedef struct {
	long msg_type;
	int w;
	int sum;
} ConfirmationMsg;

const int ConfirmationMsgSize = sizeof(int) * 2;

typedef struct {
	long msg_type;
	int ilosc_komisji;
	int k;
	int l;
	int przetworzone;
	int eligible;
	int wazne;
	int niewazne;
} MainReportMsg;

const int MainReportMsgSize = sizeof(int) * 7;

typedef struct {
	long msg_type;
	int l;
	int k;
	int n;
} ReportMsg;

const int ReportMsgSize = sizeof(int) * 3;

#endif
