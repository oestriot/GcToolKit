#ifndef PSV_H 
#define PSV_H

#define PSV_MAGIC "PSV\0"
#define PSV_VER (0x1)
#define PSV_FLAGS (0x0)

typedef struct PsvHeader {
	char magic[4];
	uint32_t version;
	uint32_t flags;
	uint8_t cart_secret[0x20];
	uint8_t packet20_sha1[0x14];
	uint8_t all_sectors_sha256[0x20];
	uint64_t image_size;
	uint64_t image_offset;
	uint8_t padding[0x190];
} __attribute__((packed)) PsvHeader;

#endif // PSV_H