/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * seL4 tutorial part 2: create and run a new thread
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <vka/object.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>
 #include <utils/util.h>
#include <platsupport/io.h>
#include <sel4platsupport/io.h>

#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

/* global environment variables */

// seL4_BootInfo defined in bootinfo.h
//  Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#Globals_links:
seL4_BootInfo *info;

/* simple_t defined in simple.h
 * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#Globals_links: */
simple_t simple;

/* vka_t defined in vka.h
 * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#Globals_links: */
vka_t vka;

/* allocaman_t defined in allocman.h
 * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#Globals_links: */
allocman_t *allocman;


vspace_t vspace;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];


// My variables
#define AXI_GPIO_PADDR 0x41200000
#define LED 0x03
#define LED_DELAY 100000000
    


/* stack for the new thread */
#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

UNUSED static sel4utils_alloc_data_t data;

/* convenience function in util.c: 
 * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#Globals_links:
 */
extern void name_thread(seL4_CPtr tcb, char *name);

/* function to run in the new thread */
void thread_2(void) {
    /* TODO 15: print something */
    /* hint: printf() */
    printf("Printing from thread 2 \n");
    /* never exit */
    while(1);
}

int main(void)
{
    UNUSED int error;

    /* give us a name: useful for debugging if the thread faults */
    /* seL4_CapInitThreadTCB is a cap pointer to the root task's initial TCB.
     * It is part of the root task's boot environment and defined in bootinfo.h from libsel4:
     * https://wiki.sel4.systems/seL4%20Tutorial%202#Globals_links:
     */
    zf_log_set_tag_prefix("hello-2:");
    name_thread(seL4_CapInitThreadTCB, "hello-2");

    /* TODO 1: get boot info */
    /* hint: seL4_GetBootInfo()
     * seL4_BootInfo* seL4_GetBootInfo(void);
     * @return Pointer to the bootinfo, no failure.
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_1:
     */
     info = seL4_GetBootInfo();

    /* TODO 2: init simple */
    /* hint: simple_default_init_bootinfo() 
     * void simple_default_init_bootinfo(simple_t *simple, seL4_BootInfo *bi);
     * @param simple Structure for the simple interface object. This gets initialised.
     * @param bi Pointer to the bootinfo describing what resources are available
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_2:
     */
     simple_default_init_bootinfo(&simple, info);


    /* TODO 3: print out bootinfo and other info about simple */
    /* hint: simple_print()
     * void simple_print(simple_t *simple);
     * @param simple Pointer to simple interface.
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_3:
     */ 
     simple_print(&simple);

    /* TODO 4: create an allocator */
    /* hint: bootstrap_use_current_simple()
     * allocman_t *bootstrap_use_current_simple(simple_t *simple, uint32_t pool_size, char *pool);
     * @param simple Pointer to simple interface.
     * @param pool_size Size of the initial memory pool.
     * @param pool Initial memory pool.
     * @return returns NULL on error
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_4:
     */
    allocman = bootstrap_use_current_simple(&simple,ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize alloc manager.\n"
        "\tMemory pool sufficiently sized?\n"
        "\tMemory pool pointer valid?\n");

    /* TODO 5: create a vka (interface for interacting with the underlying allocator) */
    /* hint: allocman_make_vka()
     * void allocman_make_vka(vka_t *vka, allocman_t *alloc);
     * @param vka Structure for the vka interface object.  This gets initialised.
     * @param alloc allocator to be used with this vka
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_5:
     */
     allocman_make_vka(&vka, allocman);

    /* TODO 6: get our cspace root cnode */
    /* hint: simple_get_cnode()
     * seL4_CPtr simple_get_cnode(simple_t *simple);
     * @param simple Pointer to simple interface.
     * @return The cnode backing the simple interface. no failure.
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_6:
     */
    seL4_CPtr cspace_cap;
    cspace_cap = simple_get_cnode(&simple);

    /* TODO 7: get our vspace root page diretory */
    /* hint: simple_get_pd()
     * seL4_CPtr simple_get_pd(simple_t *simple);
     * @param simple Pointer to simple interface.
     * @return The vspace (PD) backing the simple interface. no failure.
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_7:
     */
    seL4_CPtr pd_cap;
    pd_cap = simple_get_pd(&simple);

    /* TODO 8: create a new TCB */
    /* hint: vka_alloc_tcb()
     * int vka_alloc_tcb(vka_t *vka, vka_object_t *result);
     * @param vka Pointer to vka interface.
     * @param result Structure for the TCB object.  This gets initialised.
     * @return 0 on success
     * https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_8:
     */
    vka_object_t tcb_object = {0};
    error = vka_alloc_tcb(&vka, &tcb_object);
    ZF_LOGF_IFERR(error, "Failed to allocate new TCB.\n"
        "\tVKA given sufficient bootstrap memory?");
    /* TODO 9: initialise the new TCB */
    /* hint 1: seL4_TCB_Configure()
     * int seL4_TCB_Configure(seL4_TCB service, seL4_Word fault_ep, seL4_Uint8 priority, seL4_CNode cspace_root, seL4_CapData_t cspace_root_data, seL4_CNode vspace_root, seL4_CapData_t vspace_root_data, seL4_Word buffer, seL4_CPtr bufferFrame)
     * @param service Capability to the TCB which is being operated on.
     * @param fault_ep Endpoint which receives IPCs when this thread faults (must be in TCB's cspace).
     * @param priority The thread's new priority.
     * @param cspace_root The new CSpace root.
     * @param cspace_root_data Optionally set the guard and guard size of the new root CNode. If set to zero, this parameter has no effect.
     * @param vspace_root The new VSpace root.
     * @param vspace_root_data Has no effect on IA-32 or ARM processors.
     * @param buffer Address of the thread's IPC buffer. Must be 512-byte aligned. The IPC buffer may not cross a page boundary.
     * @param bufferFrame Capability to a page containing the thread?s IPC buffer.
     * @return 0 on success.
     * Note: this function is generated during build.  It is generated from the following definition:
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_9:
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     *
     * hint 2: use seL4_CapNull for the fault endpoint
     * hint 3: use seL4_NilData for cspace and vspace data
     * hint 4: we don't need an IPC buffer frame or address yet
     */
    error = seL4_TCB_Configure(tcb_object.cptr,seL4_CapNull,seL4_MaxPrio,cspace_cap,seL4_NilData,pd_cap,seL4_NilData,0,0);
    ZF_LOGF_IFERR(error, "Failed to configure the new TCB object.\n"
        "\tWe're running the new thread with the root thread's CSpace.\n"
        "\tWe're running the new thread in the root thread's VSpace.\n"
        "\tWe will not be executing any IPC in this app.\n");

    /* TODO 10: give the new thread a name */
    /* hint: we've done thread naming before */
    name_thread(tcb_object.cptr, "hello-2 - thread 2");


    /*
     * set start up registers for the new thread:
     */

    seL4_UserContext regs = {0};

    /* TODO 11: set instruction pointer where the thread shoud start running */
    /* hint 1: sel4utils_set_instruction_pointer()
     * void sel4utils_set_instruction_pointer(seL4_UserContext *regs, seL4_Word value);
     * @param regs Data structure in which to set the instruction pointer value
     * @param value New instruction pointer value
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_11:
     *
     * hint 2: we want the new thread to run the function "thread_2"
     */
     sel4utils_set_instruction_pointer(&regs,(seL4_Word)thread_2);

    /* check that stack is aligned correctly */
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);
    ZF_LOGF_IF(thread_2_stack_top % (stack_alignment_requirement) != 0,
        "Stack top isn't aligned correctly to a %dB boundary.\n"
        "\tDouble check to ensure you're not trampling.",
        stack_alignment_requirement);

    /* TODO 12: set stack pointer for the new thread */
    /* hint 1: sel4utils_set_stack_pointer()
     * void sel4utils_set_stack_pointer(seL4_UserContext *regs, seL4_Word value);
     * @param regs  Data structure in which to set the stack pointer value
     * @param value New stack pointer value
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_12:
     *
     * hint 2: remember the stack grows down!
     */
     sel4utils_set_stack_pointer(&regs, thread_2_stack_top);

    /* TODO 13: actually write the TCB registers.  We write 2 registers:
     * instruction pointer is first, stack pointer is second. */
    /* hint: seL4_TCB_WriteRegisters()
     * int seL4_TCB_WriteRegisters(seL4_TCB service, seL4_Bool resume_target, seL4_Uint8 arch_flags, seL4_Word count, seL4_UserContext *regs)
     * @param service Capability to the TCB which is being operated on.
     * @param resume_target The invocation should also resume the destination thread.
     * @param arch_flags Architecture dependent flags. These have no meaning on either IA-32 or ARM.
     * @param count The number of registers to be set.
     * @param regs Data structure containing the new register values.
     * @return 0 on success
     *
     * Note: this function is generated during build.  It is generated from the following definition:
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_13:
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     */
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0,0,2,&regs);
    ZF_LOGF_IFERR(error, "Failed to write the new thread's register set.\n"
        "\tDid you write the correct number of registers? See arg4.\n");

    /* TODO 14: start the new thread running */
    /* hint: seL4_TCB_Resume()
     * int seL4_TCB_Resume(seL4_TCB service)
     * @param service Capability to the TCB which is being operated on.
     * @return 0 on success
     *
     * Note: this function is generated during build.  It is generated from the following definition:
     * Links to source: https://wiki.sel4.systems/seL4%20Tutorial%202#TODO_14:
     * You can find out more about it in the API manual: http://sel4.systems/Info/Docs/seL4-manual-3.0.0.pdf
     */
    //error = seL4_TCB_Resume(tcb_object.cptr);
    //ZF_LOGF_IFERR(error, "Failed to start new thread.\n");

    /* we are done, say hello */
    printf("main: hello world first app something \n");


    static const uintptr_t gpio_paddr = AXI_GPIO_PADDR;


    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data, simple_get_pd(&simple), &vka, info);


    //printf("The address of gpio is %c " *gpio_addr);

    seL4_CPtr frame_cap;
    error = vka_cspace_alloc(&vka, &frame_cap);
    if (error) {
        ZF_LOGE("Failed to allocate cslot for frame cap\n");
        return error;
    }

    cspacepath_t ep_cap_path;
    /* find the physical frame */
    vka_cspace_make_path(&vka, frame_cap, &ep_cap_path);
    error = simple_get_frame_cap(&simple, (void*)gpio_paddr, 20, &ep_cap_path);
    if (error) {
        ZF_LOGE("Failed to find frame at paddr %p\n", gpio_paddr);
        return error;
    }
    volatile seL4_Word * axi_gpio_vaddr = vspace_map_pages(&vspace, &ep_cap_path.capPtr, NULL, seL4_CanRead | seL4_CanWrite , 1 , 20 , 0 );


/*    ps_io_mapper_t io_mapper;
    error = sel4platsupport_new_io_mapper(simple,vspace,vka,&io_mapper);
*/
//    volatile seL4_Word * axi_gpio_vaddr = ps_io_map(&io_mapper, gpio_paddr, 20, 0, PS_MEM_NORMAL);
    #define LED 0x03
    int led = LED;
    
    int j = 0;
    while(1)
    {
        int delay = 0;
        *(axi_gpio_vaddr) = led;
        led =~led;
        for(delay;delay<LED_DELAY;delay++)
        {
            int k =0;
            k = k+1;
        };
        printf("Loop Counter %d",j);
        j++;
    }


    return 1;

    


    /*
    XGpio_WriteReg(InstancePtr->BaseAddress, ((Channel - 1) * XGPIO_CHAN_OFFSET) + XGPIO_DATA_OFFSET,   Data);


    *(t->vaddr + CTRL_REG_OFFSET) |= (1 << START_TIMER_BIT);



    *(axi_gpio_vaddr) = 0x00 (For first channel)
    *(axi_gpio_vaddr + 8) = 0x01 (For second channel) */



}

