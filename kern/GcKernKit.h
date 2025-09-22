/*
*	GcKernKit export headers
*/

int kResetGc();
int kClearCartSecret();
int kGetCartSecret(uint8_t* keys);

int kHasCmd20Captured();
int kGetLastCmd20KeyId();
int kGetLastCmd20Input(void* cmd20_input);
int kResetCmd20Input();

int kOpenDevice(const char* device, int permission);
int kReadDevice(int device_handle, uint8_t* data, int size);
int kWriteDevice(int device_handle, uint8_t* data, int size);
int kCloseDevice(int device_handle);
void kGetDeviceSize(int device_handle, uint64_t* device_size);

int kFormatDevice(const char* device);

int kGetCardId(int deviceIndex, void* cardId);
int kGetCardCsd(int deviceIndex, void* cardCsd);
int kGetCardExtCsd(int deviceIndex, void* cardExtCsd);

typedef struct SceSblSmCommGcData {
    int always1;
    int command;
    uint8_t data[2048];
    int key_id;
    int size;
    int always0;
} SceSblSmCommGcData;

typedef struct CommsData { 
    uint8_t packet6[32];
    uint8_t packet9[48];
    uint8_t packet17[32];
    uint8_t packet18[67];
    uint8_t packet19[16];
    uint8_t packet20[83];
} CommsData;

enum GcToolKitError {	
	SIZE_IS_ZERO = -9584,
	SIZE_NOT_MATCH = -9585,
	SIZE_NO_SPACE = -9686,
	POINTER_WAS_NULL = -1132,
	INVALID_MAGIC = -2329,
	SFO_TOO_MANY_ENTRIES = -253,
	KEYS_VERIFY_P18_FAILED = -9588,
	KEYS_VERIFY_P20_FAILED = -9589,
	KEYS_NOT_CAPTURED = -9587,
	DEVICE_KERNEL_MODULE_NOT_STARTED = -120,
	TOTAL_FILES_LESS_THAN_EQ_0 = -920,
	INTER_INFO_WAS_NULL = -1130,
	BRICK_PREVENTED_DEVICE_WHITELIST = -128,

};