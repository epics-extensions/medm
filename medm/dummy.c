/*
 *  dummy.c
 *
 *  dummy routines for architectures (not yet) supporting full EPICS services
 *	(e.g., channel access client libraries)
 */

#include <stdio.h>

#if defined(DUMMY_CA)

void ca_task_initialize() {fprintf(stderr,"using dummy CA library!!"); }
void ca_signal() { fprintf(stderr,"using dummy CA library!!"); }
void ca_add_fd_registration() { fprintf(stderr,"using dummy CA library!!"); }
void ca_task_exit() { fprintf(stderr,"using dummy CA library!!"); }
void ca_pend() { fprintf(stderr,"using dummy CA library!!"); }
int SEVCHK() { fprintf(stderr,"using dummy CA library!!"); }
int ca_get_callback() { fprintf(stderr,"using dummy CA library!!"); }
int ca_puser() { fprintf(stderr,"using dummy CA library!!"); }
int ca_add_event() { fprintf(stderr,"using dummy CA library!!"); }
int ca_build_and_connect() { fprintf(stderr,"using dummy CA library!!"); }
int ca_add_masked_array_event() { fprintf(stderr,"using dummy CA library!!"); }
int ca_array_get_callback() { fprintf(stderr,"using dummy CA library!!"); }
int ca_clear_event() { fprintf(stderr,"using dummy CA library!!"); }
int ca_put() {fprintf(stderr,"using dummy CA library!!"); }
int ca_array_put() {fprintf(stderr,"using dummy CA library!!"); }
int ca_flush_io() {fprintf(stderr,"using dummy CA library!!"); }

#endif
