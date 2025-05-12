#ifndef _DEVICETREE_H
#define _DEVICETREE_H

#define uint32_t unsigned int
#define uint64_t unsigned long

#define FDT_BEGIN_NODE 0x00000001  // beginning of a node's representation; followed by the node's unit name (null-terminated string)
#define FDT_END_NODE   0x00000002  // end of a node's representation; 
#define FDT_PROP       0x00000003  // beginning of a property's representation  
#define FDT_NOP        0x00000004  // 
#define FDT_END        0x00000009  // end of the structure block

extern void *__dtb_addr;

struct fdt_header {
    uint32_t magic;              // 0xd00dfeed (big-endian)
    uint32_t totalsize;          // total size of DT block in bytes
    uint32_t off_dt_struct;      // offset to structure block
    uint32_t off_dt_strings;     // offset to strings block
    uint32_t off_mem_rsvmap;     // offset to memory reservation block
    uint32_t version;            // format version
    uint32_t last_comp_version;  // last compatible version
    uint32_t boot_cpuid_phys;    // physical ID of the boot CPU
    uint32_t size_dt_strings;    // size of strings block in bytes
    uint32_t size_dt_struct;     // size of structure block in bytes
};


struct fdt_reserve_entry {
    uint32_t address;
    uint32_t size;
};

struct fdt_prop_descriptor {
    uint32_t len;
    uint32_t nameoff;
};

void initramfs_callback(char *);
void fdt_traverse(void (* callback)(char *));


#endif