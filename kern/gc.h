#ifndef GC_H
#define GC_H 1

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

// shamelessly stolen from the henkaku wiki. 
typedef struct sd_context_part_base {
   struct sd_context_global* gctx_ptr;
   uint32_t unk_4;
   uint32_t def_sector_size_mmc; // looks like default sector size - used in mmc read/write commands for resp_block_size_24
   uint32_t def_sector_size_sd; // looks like default sector size - used in sd read/write commands for resp_block_size_24

   uint8_t CID[0x10]; // this is CID data but in reverse
   uint8_t CSD[0x10]; // this is CSD data but in reverse
} sd_context_part_base;


typedef struct GcInteruptInfo {
	SceUID request_id;
	SceUID op_sync_id;
	char unk[0x20];
} GcInteruptInfo;


typedef struct cmd_info {
   uint32_t state_flags;
   uint32_t command;
   uint32_t argument;
   void* buffer;
   uint16_t resp_block_size;
   uint16_t resp_n_blocks;
   
   union {
     struct {
        char data[0x10];
     } db;
     struct {
        uint32_t dw0;
        uint32_t dw1;
        uint32_t dw2;
        uint32_t dw3;
     } dw;
   } response;
   
   uint32_t error_code;
} cmd_info;

typedef struct host_info {
    void** host_registers;
    uint32_t unk_4;
    uint32_t base_clock; // = 48000000 dec
    uint32_t bus_width; // = 1 / 4 / 8 (bits)
    uint32_t clock_frequency; // = base_clock >> (SDCLK Frequency Select)
    uint8_t timeout_control_register;
    uint8_t specification_version_number; // = 1 / 2 / 3
    uint8_t vendor_version_number;
    uint8_t unk_17;
    uint32_t unk_18;
    uint32_t unk_1C;
    uint32_t unk_20;
    uint32_t unk_24;
} host_info;

typedef struct device_info {
    uint32_t dev_type_idx; // (1,2,3)
    uint32_t unk_4;
    uint16_t unk_8;
} device_info;

typedef struct sdif_context_general { //size is 0x40
    SceUID suspend_callback_id;
    uint32_t max_array_index; // typically 3
    uint32_t unk_8;
    uint32_t unk_C; 

    uint32_t unk_10;
    uint32_t unk_14;
    uint32_t unk_18;
    uint32_t unk_1C; 

    uint32_t unk_20;
    uint32_t unk_24;
    uint32_t unk_28;
    uint32_t unk_2C; 

    uint32_t unk_30;
    uint32_t unk_34;
    uint32_t unk_38;
    uint32_t unk_3C; 
} sdif_context_general;

typedef struct cmd_input { // size is 0x240
   uint32_t size; // 0x240

   // bit 10 (shift left 0x15) - request invalidate flag - invalidate vaddr_1C0 and vaddr_200
   // this flag is used for CMD56 and CMD17
   // bit 20 (shift left 0xB) - request mem_188 free - free memblock with uid mem_188

   // bit 20 or bit 9 cancels invalidation (both must be clear)
   uint32_t state_flags; // interrupt handler completion flag

   uint32_t command;
   uint32_t argument;
   
   // stores normal response without command index and crc-7
   // can also store CID or CSD. crr-7 will be cleared
   // storage order is reversed
   union {
     struct {
        char data[0x10];
     } db;
     struct {
        uint32_t dw0;
        uint32_t dw1;
        uint32_t dw2;
        uint32_t dw3;
     } dw;
   } response;

   void* buffer; // cmd data buffer ptr - dest for vaddr_1C0
   uint16_t resp_block_size_24; // block size of response. typically 0x200 which is default sector size
   uint16_t resp_n_blocks_26; // number of blocks in response. typically number of sectors to read/write
   uint32_t error_code; // error code from interrupt handler (confirmed)
   uint32_t unk_2C;

   uint8_t data0[0x30];   
   
   struct cmd_input* next_cmd;
   uint32_t unk_64; // some flag. must be 3 for invalidation to happen
   uint32_t array_index;
   int(*set_event_flag_callback)(void* ctx);
   
   SceUID evid; // event id SceSdif0, SceSdif1, SceSdif2 (SceSdif3 ?)
   struct cmd_input* secondary_cmd; // (when multiple commands are sent)
   struct sd_context_global* gctx_ptr;
   uint32_t unk_7C;
   
   char vaddr_80[0x80]; // 3 - mapped to paddr_184 (invalidate 0x80)

   void* vaddr_100;
   uint8_t data_104[0x7C];

   uint32_t unk_180;
   void* paddr_184; // 3 - phys address of vaddr_80
   SceUID mem_188; // SceSdif memblock
   uint32_t unk_18C;

   uint32_t unk_190;
   uint32_t unk_194;
   void* base_198; // dest base for vaddr_200 (also ptr for invalidate)
                   // data at base contains CMD17 data
                   // data at base also contains fragments of CMD56 response
                   // data at offset is unknown (zeroes)
   uint32_t offset_19C; //dest offset for vaddr_200 (also size for invalidate)

   uint32_t size_1A0; // size of vaddr_1C0 - only valid if request invalidate flag is set
   uint32_t size_1A4; // size of vaddr_200 - only valid if request invalidate flag is set
   void* paddr_1A8; // 1 - phys address of vaddr_1C0
   void* paddr_1AC; // 2 - phys address of vaddr_200

   SceInt64 wide_time1; // 0x1B0
   SceInt64 wide_time2; // 0x1B8 - relevant for commands that need to wait for data on DAT lines

   char vaddr_1C0[0x40]; // 1 - mapped to paddr_1A8 (invalidate 0x40)
                         //   - only valid if request invalidate flag is set
                         //   - contains fragments of CMD56 request/response
                         //   - does not contain CMD17 data

   char vaddr_200[0x40]; // 2 - mapped to paddr_1AC (invalidate 0x40)
                         //   - only valid if request invalidate flag is set
                         //   - contains unknown data (zeroes)
} cmd_input;


typedef struct sd_context_data { // size is 0xC0
    struct cmd_input* cmd_ptr;
    struct cmd_input* cmd_ptr_next;
    uint32_t unk_8;
    uint32_t unk_C;
    
    uint32_t dev_type_idx; // (1, 2, 3)
    sd_context_part_base* ctx; // pointer to custom context (sd_context_part_mmc*, sd_context_part_sd*, sd_context_part_wlanbt*)
    uint32_t voltages; // MMC_VDD_165_195, MMC_VDD_32_33, etc. Values seen: SDIF0 and SDIF2: 0x80, SDIF1 and SDIF3: 0x300000
    uint32_t unk_1C;

    uint32_t array_idx; // (0,1,2)
    uint8_t unk_24;
    uint8_t unk_25;
    uint8_t unk_26;
    uint8_t unk_27;
    cmd_input* cmd_28;
    cmd_input* cmd_2C;

    void** host_registers; // membase of SceSdif (0,1,2) memblock of size 0x1000
    uint32_t unk_34;
    uint8_t unk_38;
    uint8_t slow_mode; // relevant only for 2 read and 2 write functions
    uint8_t unk_3A;
    uint8_t unk_3B;

    SceUID host_registers_uid; // UID of SceSdif (0,1,2) memblock of size 0x1000

    SceUID evid; // event id SceSdif0, SceSdif1, SceSdif2 (SceSdif3 ?)
    void* sdif_fast_mutex;  //size is 0x40 - SceSdif0, SceSdif1, SceSdif2 (SceSdif3 ?) 

    //it looks like this chunk is separate structure since offset 0x2480 is used too often

    //offset 0x2484
    SceUID uid_10000; // UID of SceSdif (0,1,2) memblock of size 0x10000
    void* membase_10000; // membase of SceSdif (0,1,2) memblock of size 0x10000
    uint32_t unk_8C;

    uint32_t unk_90;
    int lockable_int;
    uint32_t unk_98;
    uint32_t unk_9C;

    uint32_t unk_A0;
    uint32_t unk_A4;
    uint32_t unk_A8;
    uint32_t unk_AC;

    uint32_t unk_B0;
    uint32_t unk_B4;
    uint32_t unk_B8;
    uint32_t unk_BC;
} sd_context_data;

typedef struct sd_context_part_mmc { // size is 0x398
   sd_context_part_base ctxb;
   
   uint8_t EXT_CSD[0x200]; // 0x30

   uint8_t data_230[0x160];
   
   void* unk_390;
   uint32_t unk_394;
} sd_context_part_mmc;

typedef struct sd_context_part_sd { // size is 0xC0
   sd_context_part_base ctxb;

   uint8_t data[0x90];
} sd_context_part_sd;

typedef struct sd_context_part_wlanbt { // size is 0x398
   struct sd_context_global* gctx_ptr;
   
   uint8_t data[0x394];
} sd_context_part_wlanbt;

typedef struct sd_context_global { // size is 0x24C0
    struct cmd_input commands[16];
    struct sd_context_data ctx_data;
} sd_context_global;

typedef struct bulk_transfer {
    uint32_t unk0;
    uint32_t unk1;
    uint32_t count;
    uint32_t unk2;
    uint32_t type; // 1: register access, 2: memory access
    uint32_t unk3;
    void * (*get_next)(void *); // callback to get next buffer
    uint32_t unk4;
} bulk_transfer;

void get_interupt_location();

#endif