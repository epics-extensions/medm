#ifndef _MEDM_INIT_TASK_H
#define _MEDM_INIT_TASK_H

#define INIT_SHARED_C      0
#define LAST_INIT_C        INIT_SHARED_C + 1

#ifdef ALLOCATE_STORAGE
  InitTask medmInitTask[LAST_INIT_C] = {
    {False, medmInitSharedDotC},};
#else
  extern InitTask medmInitTask[];
#endif

#endif
