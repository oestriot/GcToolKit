#include <vitasdk.h>
#include <string.h>
#include <GcKernKit.h>

static char tmp_error[0x20];
const char* get_error_msg(int error) {	
	switch(error) {
		case SIZE_IS_ZERO:
			return "Size is zero bytes";
		case SIZE_NOT_MATCH:
			return "Size does not match expected size";
		case SIZE_NO_SPACE:
			return "No space left";
		case POINTER_WAS_NULL:
			return "A pointer specified was NULL";
		case INVALID_MAGIC:
			return "Invalid Magic Number";
		case SFO_TOO_MANY_ENTRIES:
			return "SFO file has too many entries";
		case KEYS_VERIFY_P18_FAILED:
			return "Failed to verify Packet 18 Key";
		case KEYS_VERIFY_P20_FAILED:
			return "Failed to verify Packet 20 Key";
		case KEYS_NOT_CAPTURED:
			return "CMD56 Keys have not yet been captured.";
		case KERNEL_MODULE_FAILED_START:
			return "Kernel module failed to start";
		case TOTAL_FILES_LESS_THAN_EQ_0:
			return "Directory contained no files.";
		case DEVICE_WHITELIST_FAILED:
			return "Destination device is not a gamecard, aborting to prevent a brick.";
		default:
			_sceErrorGetExternalString(tmp_error, error);
			return (const char*)tmp_error;
	}
}