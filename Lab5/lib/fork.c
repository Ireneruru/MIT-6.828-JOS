// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
    static void
pgfault(struct UTrapframe *utf)
{
    void *addr = (void *) utf->utf_fault_va;
    uint32_t err = utf->utf_err;
    int r;
    pte_t pte;
    // Check that the faulting access was (1) a write, and (2) to a
    // copy-on-write page.  If not, panic.
    // Hint:
    //   Use the read-only page table mappings at uvpt
    //   (see <inc/memlayout.h>).
    // LAB 4: Your code here.
    if (!(utf->utf_err & FEC_WR)) panic("pgfault(): FEC_WR\n");
    if (!uvpd[((uintptr_t)addr >> 22)]) panic("pgfault(): page not mapped");
    if (!(pte = uvpt[(uintptr_t)addr >> 12])) panic("pgfault(): page not mapped");
    if (!(pte & PTE_COW)) panic("pgfault: PTE_COW\n");
    // Allocate a new page, map it at a temporary location (PFTEMP),
    // copy the data from the old page to the new page, then move the new
    // page to the old page's address.
    // Hint:
    //   You should make three system calls.
    // LAB 4: Your code here.
    sys_page_alloc(0, (void*)PFTEMP, PTE_P | PTE_U | PTE_W);
    memcpy((void*)PFTEMP, (void*)ROUNDDOWN((uintptr_t)addr, PGSIZE), PGSIZE);
    sys_page_map(0, (void*)PFTEMP, 0, (void*)ROUNDDOWN((uintptr_t)addr, PGSIZE), PTE_P | PTE_U | PTE_W);
    sys_page_unmap(0, (void*)PFTEMP);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
    static int
duppage(envid_t envid, unsigned pn)
{
    int r;

    // LAB 4: Your code here.
    int perm = uvpt[pn] & 0xfff;

    if (perm & PTE_SHARE) {
	if ((r = sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), perm)))
	    return r;
	return 0;
    }
    if (perm & (PTE_W | PTE_COW)) {
	if ((r = sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), (perm & ~PTE_W) | PTE_COW)))
	    return r;
	if ((perm & PTE_W)&&(r = sys_page_map(0, (void*)(pn * PGSIZE), 0, (void*)(pn * PGSIZE), (perm & ~PTE_W) | PTE_COW)))
	    return r;
    }
    else {
	if ((r = sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), perm)))
	    return r;
    }
    return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
    envid_t
fork(void)
{
    // LAB 4: Your code here.
    extern void _pgfault_upcall(void);
    uintptr_t p = 0;
    envid_t envid;
    set_pgfault_handler(pgfault);

    envid = sys_exofork();
    if (envid < 0) return envid;
    if (envid == 0) {
	thisenv = &envs[ENVX(sys_getenvid())];
	return 0;
    }
    while (p < UTOP) {
	if (!uvpd[p >> 22]) {
	    p += PGSIZE << 10;
	    continue;
	}
	if (p != UXSTACKTOP - PGSIZE && uvpt[p >> 12]) duppage(envid, p >> 12);
	p += PGSIZE;
    }
    if (sys_page_alloc(envid, (void*)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W) < 0) panic("fork(): 111111\n");
    if (sys_env_set_pgfault_upcall(envid, _pgfault_upcall) < 0) panic("fork(): 22222222\n");
    if (sys_env_set_status(envid, ENV_RUNNABLE) < 0) panic("fork(): 3333333\n");
    return envid;
}

// Challenge!
    int
sfork(void)
{
    panic("sfork not implemented");
    return -E_INVAL;
}
