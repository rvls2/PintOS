#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"   /* is_user_vaddr, PHYS_BASE */
#include "threads/palloc.h"
#include "threads/pte.h"     /* pg_round_down */
#include "threads/synch.h"
#include "userprog/process.h" /* thread_current / process_exit if you have it */

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
/* page_fault now handles user faults by terminating the user process
   with exit(-1) instead of panicking the kernel. */
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs. */
void
exception_init (void) 
{
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill, "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill, "#XF SIMD Floating-Point Exception");

  /* Page faults: we must disable interrupts while reading CR2. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* Determine where the exception originated (user or kernel). */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User code caused the exception; kill the user process. */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 
      break;

    case SEL_KCSEG:
      /* Kernel code: kernel bug. Panic. */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 
      break;
    default:
      /* Unknown code segment. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
      break;
    }
}

/* Page fault handler.
   For faults caused by user-mode accesses, we must *not* panic the kernel.
   Instead, terminate the user process with exit(-1) (and print the exit
   message). Kernel-mode faults remain a PANIC. */

static bool 
try_grow_stack (void *fault_addr, struct intr_frame *f){
  void *upage = pg_round_down (fault_addr);
  void *kpage;

  //Verificar pra não crescer acima de PHYS_BASE (Kernel).
  if (!is_user_vaddr (fault_addr) || fault_addr >= PHYS_BASE)
    return false;
  
  if (f != NULL){
    void *esp = f ->esp;
    // Se esp for inválido, não crescer
    if (!is_user_vaddr (esp))
      return false;
    
    if ((fault_addr < esp - 32) && (pg_round_down(esp) != upage))
      return false;
  }
  // Pegar pag física para o usuário:
  kpage = palloc_get_page(PAL_USER | PAL_ZERO);
  if (kpage == NULL)
    return false;

  if (!install_page(upage, kpage, true)){
    palloc_free_page (kpage);
    return false;
  }
  return true;
}
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Get faulting address from CR2. */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  /* Re-enable interrupts. */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Parse error_code for cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  /* If fault originated in user context, terminate the user process
     with the required exit output (<name>: exit(-1)\n). */
  if (user)
    {
      // Se for página não presente, tentar acrescimo de pilha
      if (not_present){
        if (try_grow_stack (fault_addr, f))
          return; 
      }
      /* Print required exit message and terminate thread. */
      printf ("%s: exit(%d)\n", thread_name (), -1);
      thread_exit ();
      return;
    }

  /* Otherwise, kernel fault -> print diagnostic and panic. */
  printf ("Page fault at %p: %s error %s page in %s context.\n",
          fault_addr,
          not_present ? "not present" : "rights violation",
          write ? "writing" : "reading",
          user ? "user" : "kernel");
  kill (f);
}
