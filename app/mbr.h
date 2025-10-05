#ifndef MBR_H 
#define MBR_H

#define SECTOR_SIZE (0x200)

enum ScePartitionCode {
	ScePartitionCode_EMPTY = 0x00,
	ScePartitionCode_IDSTORAGE = 0x01,
	ScePartitionCode_SLB2 = 0x02,
	ScePartitionCode_OS0 = 0x03,
	ScePartitionCode_VS0 = 0x04,
	ScePartitionCode_VD0 = 0x05,
	ScePartitionCode_TM0 = 0x06,
	ScePartitionCode_UR0 = 0x07,
	ScePartitionCode_UX0 = 0x08,
	ScePartitionCode_GRO0 = 0x09,
	ScePartitionCode_GRW0 = 0x0A,
	ScePartitionCode_UD0 = 0x0B,
	ScePartitionCode_SA0 = 0x0C,
	ScePartitionCode_MEDIAID = 0x0D,
	ScePartitionCode_PD0 = 0x0E,
	ScePartitionCode_UNUSED = 0x0F
};

enum ScePartitionType {
	ScePartitionType_FAT16 = 0x06,
	ScePartitionType_EXFAT = 0x07,
	ScePartitionType_RAW = 0xDA
};


typedef struct ScePartiton {
	uint32_t off;
	uint32_t sz;
	uint8_t code;
	uint8_t type;
	uint8_t active;
	uint32_t flags;
	uint16_t unk;
} __attribute__((packed)) ScePartiton;

typedef struct SceMbr {
	char magic[0x20];
	uint32_t version;
	uint32_t devicesize;
	char unk1[0x28];
	ScePartiton partitions[0x10];
	char unk2[0x5e];
	char unk3[0x10 * 4];
	uint16_t sig;
} __attribute__((packed)) SceMbr;


#endif // MBR_H