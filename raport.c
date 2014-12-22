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
	int l = 0;
    if (argc > 2) {
        fatal("Usage: ./raport [L]");
        exit(0);
    } else if (argc == 2) {
		l = atoi(argv[1]);
	} 

    if (l < 0|| l > 9999) {
        fatal("L has to be 0 < L < 10000");
        exit(0);
    }

    int requestId, mainReportId, reportId;
    RequestMsg request;
    pid_t mypid = getpid();

    if ((requestId = msgget(REQUESTS_MKEY, 0)) == -1)
        syserr("msgget failed");
	if ((mainReportId = msgget(MAINREP_MKEY, 0)) == -1)
		syserr("msgget failed");
	if ((reportId = msgget(REP_MKEY, 0)) == -1)
		syserr("msgget failed");

	request.msg_type = 1L;
	request.mypid = mypid;
	request.m = 0;
	request.l = l;
	
    if (msgsnd(requestId, &request, RequestMsgSize, 0) != 0)
        syserr("Failed to send message");
	
	MainReportMsg mainRep;
	if (msgrcv(mainReportId, &mainRep, MainReportMsgSize, (long) mypid, 0) != MainReportMsgSize)
		syserr("Failed to get main report message");
	
	printf("Przetworzonych komisji: %d / %d\n", mainRep.przetworzone, mainRep.ilosc_komisji);
	printf("Uprawnionych do głosowania: %d\n", mainRep.eligible);
	printf("Głosów ważnych: %d\n", mainRep.wazne);
	printf("Głosów nieważnych: %d\n", mainRep.niewazne);
	float frek;
	if (mainRep.eligible != 0) {
		frek = (float) (mainRep.wazne + mainRep.niewazne);
		frek = frek / mainRep.eligible;
		frek = frek * 100;
	} else {
		frek = 0;
	}
	printf("Frekwencja : %f\n", frek);
	printf("Wyniki poszczególnych list:\n");
	
	int *dane;
	dane = malloc(mainRep.l * mainRep.k * sizeof(int));
	int iter1, iter2;
	for (iter1 = 0; iter1 < mainRep.l; ++iter1)
		for (iter2 = 0; iter2 < mainRep.k; ++iter2)
			*(dane + iter1 * mainRep.l + iter2) = 0;
		
	int data_in = 1;
	while (data_in) {
		ReportMsg rep;
	
		if (msgrcv(reportId, &rep, ReportMsgSize, (long) mypid, 0) != ReportMsgSize)
			syserr("Failed to get report message");
		
		if (rep.l < 0 || rep.k < 0) {
			data_in = 0;
			break;
		}
		
		*(dane + rep.l * mainRep.l + rep.k) = rep.n;
	}
	
	int iter = 0;
	if (l > 0)
		iter = l - 1;
	
	for (; iter < mainRep.l; ++iter) {
		int it;
		
		printf("%d ", iter + 1);
		
		int sum = 0;
		for (it = 0; it < mainRep.k; ++it) {
			sum += *(dane + iter * mainRep.l + it);
		}
		printf("%d ", sum);
		
		for (it = 0; it < mainRep.k; ++it) {
			printf("%d ", *(dane + iter * mainRep.l + it));
		}
		printf("\n");
		
		if (l > 0)
			break;
	}
	
	free(dane);
	
    return 0;
}
