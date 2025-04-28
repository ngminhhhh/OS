// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, int vicfpn, int swpfpn)
{
  __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller)
{
  struct vm_rg_struct *newrg = caller->mm->mmap->vm_freerg_list;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  // struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  while(newrg->rg_next != NULL)
  {
    newrg = newrg->rg_next;
  }
  if (newrg->rg_end == caller->mm->mmap->sbrk) // Check xem free region cuối cùng có tại sbrk không
  {
    return newrg;
  }
  printf("%ld - %ld\n",newrg->rg_end,caller->mm->mmap->sbrk);
  return NULL;

  
  // newrg->rg_end = newrg->rg_start + size;
  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */

}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  // struct vm_area_struct *vma = caller->mm->mmap;
  struct vm_area_struct *vma = caller->mm->mmap;
  /* TODO validate the planned memory area is not overlapped */
  while (vma != NULL)
  {
    if (vma->vm_id != vmaid)
    {
      if ((vmastart >= vma->vm_start && vmastart < vma->vm_end) ||
          (vmaend > vma->vm_start && vmaend <= vma->vm_end) ||
          (vmastart <= vma->vm_start && vmaend >= vma->vm_end))
      {
        vma = vma->vm_next;
      }
    }
  }

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  // debug case
  if(newrg == NULL) {
    return -1; // mm reallocation fail
  }
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  /* TODO: Obtain the new vm area based on vmaid */
  // cur_vma->vm_end...

  //  inc_limit_ret...
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + inc_sz; 

  // gán regs[10]
  
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller);
  if (area == NULL) enlist_vm_rg_node(&cur_vma->vm_freerg_list,newrg);
  else 
  {
    area->rg_end += inc_sz;
    free(newrg);
  }
  cur_vma->sbrk += inc_sz;
  if (vm_map_ram(caller, cur_vma->vm_start, cur_vma->vm_end,
                 newrg->rg_start, incnumpage, newrg) < 0)
    return -1; /* Map the memory to MEMRAM */
  return 0;
}

// #endif
