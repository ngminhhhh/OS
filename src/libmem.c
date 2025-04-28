/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c 
 */

 #include "string.h"
 #include "mm.h"
 #include "syscall.h"
 #include "libmem.h"
 #include <stdlib.h>
 #include <stdio.h>
 #include <pthread.h>
 
 static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
 
 /*enlist_vm_freerg_list - add new rg to freerg_list
  *@mm: memory region
  *@rg_elmt: new region
  *
  */
 int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
 {
   struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
 
   if (rg_elmt->rg_start >= rg_elmt->rg_end)
     return -1;
 
   if (rg_node != NULL)
     rg_elmt->rg_next = rg_node;
 
   /* Enlist the new region */
   mm->mmap->vm_freerg_list = rg_elmt;
 
   return 0;
 }
 
 /*get_symrg_byid - get mem region by region ID
  *@mm: memory region
  *@rgid: region ID act as symbol index of variable
  *
  */
 struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
 {
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
     return NULL;
 
   return &mm->symrgtbl[rgid];
 }
 
 /*__alloc - allocate a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *@alloc_addr: address of allocated memory region
  *
  */
 int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
 {
   struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
 
   pthread_mutex_lock(&mmvm_lock);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   printf("[__alloc] Requested size: %d bytes (rgid = %d)\n", size, rgid);
 
   // Thử tìm trong vùng nhớ trống
   if (get_free_vmrg_area(caller, vmaid, size, rgnode) == 0)
   {
     printf("[__alloc] Found free region: [%lu -> %lu]\n", rgnode->rg_start, rgnode->rg_end);
     printf("[sbrk]: [%lu]\n",cur_vma->sbrk);
     printf("[freerg] Start - End:[%lu - %lu]\n", cur_vma->vm_freerg_list->rg_start, cur_vma->vm_freerg_list->rg_end);
 
     caller->mm->symrgtbl[rgid].rg_start = rgnode->rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode->rg_end;
     *alloc_addr = rgnode->rg_start;
     caller->regs[rgid] = (addr_t)rgnode->rg_start;
 
     pthread_mutex_unlock(&mmvm_lock);
     return 0;
   }
 
   // Không tìm được vùng trống → mở rộng sbrk
   
   
   if (cur_vma == NULL) {
     printf("[__alloc] Failed to get vm_area_struct\n");
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }

   struct vm_rg_struct *area = get_vm_area_node_at_brk(caller);
  //  struct vm_rg_struct *area = caller->mm->mmap->vm_freerg_list;
   
   int inc_sz;
   if (area == NULL) // Nếu không có free rg tại sbrk
   {
    inc_sz = PAGING_PAGE_ALIGNSZ(size);
   }
   else 
   {
    inc_sz = PAGING_PAGE_ALIGNSZ(size - (area->rg_end-area->rg_start)); 
   }
   unsigned long old_sbrk = cur_vma->sbrk;
   printf("[__alloc] No free region found → need to increase sbrk\n");
   printf("[__alloc] Current sbrk: %d, inc_sz (aligned): %d\n", old_sbrk, inc_sz);
   
   if (cur_vma->sbrk + inc_sz > cur_vma->vm_end) //check xem vma còn chỗ trống không
   {
    printf("Out of memory!\n");
    return -1;
   } 

   // Gọi syscall để mở rộng sbrk
   struct sc_regs regs;
   regs.a1 = SYSMEM_INC_OP;
   regs.a2 = vmaid;
   regs.a3 = inc_sz;
 
   int ret = libsyscall(caller, 17, regs.a1, regs.a2, regs.a3);
   if (ret != 0) {
     printf("[__alloc] SYSCALL failed to increase sbrk\n");
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
 
   // Ghi nhận vùng được cấp phát từ old_sbrk
   get_free_vmrg_area(caller, vmaid, size, rgnode);
   printf("[__alloc] Found free region: [%lu -> %lu]\n", rgnode->rg_start, rgnode->rg_end);
   printf("[sbrk]: [%lu]\n",cur_vma->sbrk);
   printf("[freerg] Start - End:[%lu - %lu]\n", cur_vma->vm_freerg_list->rg_start, cur_vma->vm_freerg_list->rg_end);
   caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
   caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
   *alloc_addr = old_sbrk;
   caller->regs[rgid] = (addr_t)rgnode->rg_start;
 
   printf("[__alloc] Allocated at: [%d -> %d] via sbrk extension\n", old_sbrk, old_sbrk + inc_sz);
 
   pthread_mutex_unlock(&mmvm_lock);
   return 0;
 }
 
 /*__free - remove a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
//  int __free(struct pcb_t *caller, int vmaid, int rgid)
//  {
//    struct vm_rg_struct *rgnode;
 
//    // Dummy initialization for avoding compiler dummay warning
//    // in incompleted TODO code rgnode will overwrite through implementing
//    // the manipulation of rgid later
 
//    if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
//      return -1;
 
//    /* TODO: Manage the collect freed region to freerg_list */
   
 
//    /*enlist the obsoleted memory region */
//    //enlist_vm_freerg_list();
//    // **************************************************************************** testing//
//    rgnode = malloc(sizeof(struct vm_rg_struct));
   
//    /* Copy the information from symbol region table */
//    rgnode->rg_start = caller->mm->symrgtbl[rgid].rg_start;
//    rgnode->rg_end = caller->mm->symrgtbl[rgid].rg_end;
   
//    /* Reset symbol region entry */
//    caller->mm->symrgtbl[rgid].rg_start = caller->mm->symrgtbl[rgid].rg_end = 0;
//    printf("[__free] Freed region: [%lu -> %lu]\n", rgnode->rg_start, rgnode->rg_end);

//    /* Enlist the obsoleted memory region */
//    return enlist_vm_freerg_list(caller->mm, rgnode);
 
 
//    // return 0;
//  }
int __free(struct pcb_t *caller, int vmaid, int rgid) {
  pthread_mutex_lock(&mmvm_lock);

  // Bước 1: Kiểm tra tính hợp lệ
  if (!caller || !caller->mm || !caller->mm->mmap || vmaid != 0 ||
      rgid < 0 || rgid >= 10 || rgid >= PAGING_MAX_SYMTBL_SZ) {
      pthread_mutex_unlock(&mmvm_lock);
      return -1;
  }

  // Bước 2: Kiểm tra vùng nhớ và regs
  struct vm_rg_struct *rgnode = &caller->mm->symrgtbl[rgid];
  if (rgnode->rg_start == -1 && rgnode->rg_end == -1) {
      pthread_mutex_unlock(&mmvm_lock);
      return -1; // Vùng không tồn tại
  }
  if (rgnode->rg_start >= rgnode->rg_end) {
      pthread_mutex_unlock(&mmvm_lock);
      return -1; // Vùng rỗng hoặc không hợp lệ
  }
  if (rgnode->rg_start != caller->regs[rgid]) {
      pthread_mutex_unlock(&mmvm_lock);
      return -1; // regs[rgid] không khớp với vùng
  }

  // Bước 3: Lưu thông tin vùng
  unsigned long rg_start = rgnode->rg_start;
  unsigned long rg_end = rgnode->rg_end;

  // Bước 4: Xóa vùng khỏi symrgtbl và regs
  rgnode->rg_start = -1;
  rgnode->rg_end = -1;
  caller->regs[rgid] = 0;

  // Bước 5: Thêm vào vm_freerg_list
  struct vm_rg_struct *new_free = malloc(sizeof(struct vm_rg_struct));
  if (!new_free) {
      pthread_mutex_unlock(&mmvm_lock);
      return -1; // Lỗi phân bổ bộ nhớ
  }
  new_free->rg_start = rg_start;
  new_free->rg_end = rg_end;
  // new_free->rg_next = caller->mm->mmap->vm_freerg_list;
  // caller->mm->mmap->vm_freerg_list = new_free;
  enlist_vm_freerg_list(caller->mm, new_free);

  // Bước 6: In thông tin gỡ lỗi
  printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n            PID=%d   -   Region=%d            \n", caller->pid, rgid);
  print_pgtbl(caller, 0, -1);
  printf("[__free] Freed region: [%lu -> %lu]\n", new_free->rg_start, new_free->rg_end);

  // Bước 7: Mở khóa và trả về
  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

 /*liballoc - PAGING-based allocate a region memory
  *@proc:  Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
 {
   /* TODO Implement allocation on vm area 0 */
   int addr;
 
   /* By default using vmaid = 0 */
   return __alloc(proc, 0, reg_index, size, &addr);
 }
 
 /*libfree - PAGING-based free a region memory
  *@proc: Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 
 int libfree(struct pcb_t *proc, uint32_t reg_index)
 {
   /* TODO Implement free region */
 
   /* By default using vmaid = 0 */
   return __free(proc, 0, reg_index);
 }
 
  /*pg_getpage - get the page in ram
  *@mm: memory region
  *@pagenum: PGN
  *@framenum: return FPN
  *@caller: caller
  *
  */
 int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
 {
  pthread_mutex_lock(&mmvm_lock);
  uint32_t pte = mm->pgd[pgn];

  if (!PAGING_PAGE_PRESENT(pte)) {
      int vicpgn, swpfpn;
      int vicfpn;
      uint32_t vicpte;
      int tgtfpn = PAGING_PTE_SWP(pte); // Khung swap của trang cần tải

      /* Chọn trang nạn */
      if (find_victim_page(caller->mm, &vicpgn) != 0) {
          pthread_mutex_unlock(&mmvm_lock);
          return -1;
      }
      vicpte = caller->mm->pgd[vicpgn];
      vicfpn = PAGING_PTE_FPN(vicpte);
      // có đầy đủ thông tin về nạn nhân

      /* Lấy khung trống trong swap */
      if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) != 0) {
          pthread_mutex_unlock(&mmvm_lock);
          return -1;
      }

      /* Swap out: Sao chép trang nạn nhân từ RAM sang swap */
      struct sc_regs regs;
      regs.a1 = SYSMEM_SWP_OP;
      regs.a2 = vicfpn; // Khung RAM của trang nạn
      regs.a3 = swpfpn; // Khung swap để lưu trang nạn
      syscall(caller, 17, &regs);
      // lệnh này vơi sysmem_swp_op sẽ đổi dữ liệu của swap và vic

      /* Swap in: Sao chép trang cần thiết từ swap sang RAM */
      regs.a2 = tgtfpn; // Khung swap của trang cần tải
      regs.a3 = vicfpn; // Khung RAM trống sau swap out
      syscall(caller, 17, &regs);

      /* Cập nhật bảng trang: Đánh dấu trang nạn là swapped */
      pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);

      /* Cập nhật bảng trang: Đánh dấu trang mới là present */
      pte_set_fpn(&pte, vicfpn);
      mm->pgd[pgn] = pte;

      /* Thêm trang mới vào danh sách FIFO */
      enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(mm->pgd[pgn]);
  pthread_mutex_unlock(&mmvm_lock);
  return 0;
 }

 /*pg_getval - read value at given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  // Gọi hàm lấy trang (gồm cả swap-in nếu cần)
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; // Page fault không xử lý được

  // Tính địa chỉ vật lý
  int phyaddr = (fpn << PAGING_ADDR_OFFST_LOBIT) + off;

  // Chuẩn bị syscall để đọc
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  regs.a3 = 0;

  if (libsyscall(caller, 17, regs.a1, regs.a2, regs.a3) != 0)
    return -1;

  // Sau syscall, đọc trực tiếp từ RAM (RAM là local, syscall chỉ mô phỏng I/O)
  return MEMPHY_read(caller->mram, phyaddr, data);
}
 /*pg_setval - write value to given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  int phyaddr = (fpn << PAGING_ADDR_OFFST_LOBIT) + off;

  // Gọi syscall mô phỏng ghi I/O
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = (uint32_t)value;

  if (libsyscall(caller, 17, regs.a1, regs.a2, regs.a3) != 0)
    return -1;

  return MEMPHY_write(caller->mram, phyaddr, value);
}
 
 /*__read - read value in region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
 
   pg_getval(caller->mm, currg->rg_start + offset, data, caller);
 
   return 0;
 }
 
 /*libread - PAGING-based read a region memory */
 int libread(
     struct pcb_t *proc, // Process executing the instruction
     uint32_t source,    // Index of source register
     uint32_t offset,    // Source address = [source] + [offset]
     uint32_t* destination)
 {
   BYTE data;
   int val = __read(proc, 0, source, offset, &data);
 
   /* TODO update result of reading action*/
   *destination = (uint32_t)data; 
   //destination 
 #ifdef IODUMP
   printf("read region=%d offset=%d value=%d\n", source, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); //print max TBL
 #endif
   MEMPHY_dump(proc->mram);
 #endif
 
   return val;
 }
 
 /*__write - write a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
 
   pg_setval(caller->mm, currg->rg_start + offset, value, caller);
 
   return 0;
 }
 
 /*libwrite - PAGING-based write a region memory */
 int libwrite(
     struct pcb_t *proc,   // Process executing the instruction
     BYTE data,            // Data to be wrttien into memory
     uint32_t destination, // Index of destination register
     uint32_t offset)
 {
 #ifdef IODUMP
   printf("write region=%d offset=%d value=%d\n", destination, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); //print max TBL
 #endif
   MEMPHY_dump(proc->mram);
 #endif
 
   return __write(proc, 0, destination, offset, data);
 }
 
 /*free_pcb_memphy - collect all memphy of pcb
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@incpgnum: number of page
  */
 int free_pcb_memph(struct pcb_t *caller)
 {
   int pagenum, fpn;
   uint32_t pte;
 
   for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
   {
     pte= caller->mm->pgd[pagenum];
 
     if (!PAGING_PAGE_PRESENT(pte))
     {
       fpn = PAGING_PTE_FPN(pte);
       MEMPHY_put_freefp(caller->mram, fpn);
     } else {
       fpn = PAGING_PTE_SWP(pte);
       MEMPHY_put_freefp(caller->active_mswp, fpn);    
     }
   }
 
   return 0;
 }
 
 
 /*find_victim_page - find victim page
  *@caller: caller
  *@pgn: return page number
  *
  */
 int find_victim_page(struct mm_struct *mm, int *retpgn)
 {
   struct pgn_t *pg = mm->fifo_pgn;
 
   /* TODO: Implement the theorical mechanism to find the victim page */
 
   free(pg);
 
   return 0;
 }
 
 /*get_free_vmrg_area - get a free vm region
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@size: allocated size
  *
  */
 int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
 {
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
 
   if (rgit == NULL)
     return -1;
     
   /* Probe unintialized newrg */
   newrg->rg_start = newrg->rg_end = -1;
   /* Traverse the list of free vm regions to find a fit space */
   while (rgit != NULL) {
    printf("[freerg**] Start - End:[%lu - %lu]\n", rgit->rg_start, rgit->rg_end);
     if (rgit->rg_end - rgit->rg_start >= size) {
       /* Found a free region large enough */
       newrg->rg_start = rgit->rg_start;
       newrg->rg_end = rgit->rg_start + size;
       printf("[newrg] Start - End: [%ld - %ld]\n",newrg->rg_start,newrg->rg_end);
      
       /* Update the free region list */
       if (cur_vma->sbrk - rgit->rg_start == size) {
         /* If the free region is exactly the size we need, 
          * remove it from the free list */
         if (rgit == cur_vma->vm_freerg_list) {
           cur_vma->vm_freerg_list = rgit->rg_next;
         } else {
           /* Find the previous region to update its next pointer */
           struct vm_rg_struct *prev = cur_vma->vm_freerg_list;
           while (prev->rg_next != rgit) {
             prev = prev->rg_next;
           }
           prev->rg_next = rgit->rg_next;
         }
         free(rgit);
       } else {
         /* If the free region is larger than needed,
          * adjust its start point */
         rgit->rg_start += size;
       }
       
       return 0;
     }
     rgit = rgit->rg_next;
   }
 
   return -1; /* No suitable free region found */
 }
 
 //#endif
 