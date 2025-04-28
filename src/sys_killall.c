/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"

#include "queue.h"
#include "string.h"
#include "sched.h"
#include <stdlib.h>

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    char proc_name[100];
    uint32_t data;

    uint32_t memrg = regs->a1;
    
    /* Get name of the target proc */
    int i = 0;
    data = 0;
    while(data != -1){
        libread(caller, memrg, i, &data);
        proc_name[i]= data;
        if(data == -1) proc_name[i]='\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    //caller->mlq_ready_queu
    for (int i = 0 ; i < caller->running_list->size ; i ++) {
        if (strcmp(caller->running_list->proc[i]->path, proc_name) == 0) {
            struct pcb_t * proc = caller->running_list->proc[i];
            free_pcb_memph(proc);

            // * Free region
            struct vm_area_struct * vma_it = proc->mm->mmap;

            while (vma_it != NULL) {
                struct vm_area_struct * cur_vma = vma_it;
                struct vm_rg_struct * region_it = vma_it->vm_freerg_list;

                while (region_it != NULL) {
                    struct vm_rg_struct * cur_region = region_it;
                    region_it = region_it->rg_next;
                    free(cur_region);
                }

                vma_it = vma_it->vm_next;
                free(cur_vma);
            }
            remove_running_proc(proc);
            free(proc);
            i --;
        }
    }

    return 0; 
}
