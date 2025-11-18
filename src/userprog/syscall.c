#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "lib/kernel/console.h" // Para putbuf

/* Declarações de Funções (Implementadas abaixo do handler) */
static void syscall_handler (struct intr_frame *);
static int get_syscall_arg (struct intr_frame *f, int index);
void process_exit_with_status (int status);
static void validate_user_ptr (const void *vaddr);

/* ------------------------------------------------------------------------- */
/* FUNÇÕES AUXILIARES DE VALIDAÇÃO E PROCESSO */
/* ------------------------------------------------------------------------- */

/* Termina o processo atual e imprime o status de saída. 
   O status é salvo na struct thread para process_wait. */
void
process_exit_with_status (int status)
{
    // Salva o status de saída na thread atual
    thread_current()->exit_status = status; 
    
    // Imprime a mensagem de saída (CRUCIAL para os testes)
    printf ("%s: exit(%d)\n", thread_current()->name, status);
    
    // Chama a rotina de saída do kernel (que fará a limpeza)
    thread_exit(); 
}

/* Valida um único endereço de ponteiro de usuário.
   Se for inválido (NULL, Kernel Space, ou não mapeado), 
   o processo é terminado imediatamente com exit(-1). */
static void
validate_user_ptr (const void *vaddr)
{
    // 1. Verificar se é NULL ou Kernel Space
    if (vaddr == NULL || !is_user_vaddr(vaddr)) {
        process_exit_with_status(-1); 
    }

    // 2. Verificar se o endereço está mapeado na tabela de páginas.
    // **NOTA:** A implementação robusta desta verificação
    // DEVE ser feita no handler de page fault (exception.c)
    // para que o Lazy Loading (Parte 3) funcione.
    // Por enquanto, uma checagem básica pode ser mantida:
    /*
    if (pagedir_get_page (thread_current()->pagedir, vaddr) == NULL) {
         process_exit_with_status(-1);
    }
    */
}

/* Obtém o argumento da pilha do usuário no índice especificado (1 para o primeiro arg). 
   Garante a validação do endereço do ponteiro. */
static int
get_syscall_arg (struct intr_frame *f, int index)
{
    // O ponteiro do argumento está em f->esp + (4 * index)
    int *ptr = (int *)f->esp + index; 
    
    // Valida o endereço do ponteiro antes de desreferenciá-lo!
    validate_user_ptr(ptr);
    
    // Retorna o valor contido nesse endereço
    return *ptr;
}

/* ------------------------------------------------------------------------- */
/* IMPLEMENTAÇÕES DAS SYSCALLS BÁSICAS */
/* ------------------------------------------------------------------------- */

void 
sys_halt (void) 
{
    shutdown_power_off();
}

int 
sys_write (int fd, const void *buffer, unsigned size) 
{
    // Valida o ponteiro do buffer (o endereço que aponta para os dados)
    validate_user_ptr(buffer);

    if (fd == 1) // STDOUT
    {
        putbuf(buffer, size);
        return (int)size;
    }
    
    // Lógica para gravação em arquivos (Fase 4 - Filesys)
    // ...
    
    return 0;
}

/* ------------------------------------------------------------------------- */
/* INICIALIZAÇÃO E HANDLER */
/* ------------------------------------------------------------------------- */

void
syscall_init (void) 
{
    // Registra a função syscall_handler para a interrupção 0x30
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
    // 
    
    // Valida o ponteiro base da pilha (f->esp)
    validate_user_ptr(f->esp); 

    int syscall_num = ((int *)f->esp)[0];
    
    // O valor de retorno da syscall será colocado em f->eax
    f->eax = -1; 

    switch (syscall_num) 
    {
        case SYS_HALT:
            sys_halt();
            break;
            
        case SYS_EXIT:
            // Argumento 1: status
            process_exit_with_status(get_syscall_arg(f, 1)); 
            break;
            
        case SYS_EXEC:
            // Argumento 1: cmd_line (Ponteiro para a string)
            // Lógica: Criar um novo processo com process_execute()
            f->eax = -1; // Substituir por tid ou -1
            break;
            
        case SYS_WAIT:
            // Argumento 1: tid
            // Lógica: Chamar process_wait(tid) (Fase 2-2)
            f->eax = -1; // Substituir por exit status ou -1
            break;
            
        case SYS_WRITE:
        {
            // Argumentos: fd=esp[1], buffer=esp[2], size=esp[3]
            int fd = get_syscall_arg(f, 1);
            void *buffer = (void *)get_syscall_arg(f, 2);
            unsigned size = (unsigned)get_syscall_arg(f, 3);
            
            f->eax = sys_write(fd, buffer, size);
            break;
        }

        /* -------------------------------------------------- */
        /* Syscalls de Arquivos e Memória - Implementar aqui: */
        /* -------------------------------------------------- */
        
        case SYS_CREATE: // ...
        case SYS_REMOVE: // ...
        case SYS_OPEN:   // ...
        case SYS_FILESIZE: // ...
        case SYS_READ:   // ...
        case SYS_SEEK:   // ...
        case SYS_TELL:   // ...
        case SYS_CLOSE:  // ...
        // case SYS_MMAP: // ... (Parte 3)
        // case SYS_MUNMAP: // ... (Parte 3)
            
        default:
            // Se a syscall não for reconhecida, termina o processo com erro.
            process_exit_with_status(-1);
            break;
    }
}

