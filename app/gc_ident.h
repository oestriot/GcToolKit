#ifndef GC_IDENT_H 
#define GC_IDENT_H 1

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <GcToolKit.h>
#include "psv.h"
#include "vci.h"

#define CARD_ID_SIZE 0x10
#define CARD_DESCRIPTOR_SIZE 0x10

typedef struct EmmcCardId {
	uint8_t unused : 1;
	uint8_t crc7 : 7;
	uint8_t manufacture_date : 8;
	uint32_t serial_number : 32;
	uint8_t revision : 8;
	char product_name[6];
	uint8_t oem_id : 8;
	uint8_t device_type : 2;
	uint8_t reserved : 6;
	uint8_t vendor : 8;
} __attribute__((packed)) EmmcCardId;
static_assert(sizeof(EmmcCardId) == CARD_ID_SIZE);

typedef struct EmmcCardDescriptor {	
	uint16_t unused : 1;
	uint16_t crc7 : 7;
	uint16_t ecc_code : 2;
	uint16_t file_format : 2;
	uint16_t tmp_write_prot : 1;
	uint16_t perm_write_prot : 1;
	uint16_t copy : 1;
	uint16_t file_format_group : 1;
	uint16_t content_protection_application : 1;
	uint16_t reserved1 : 4;
	uint16_t partial_write_block_allowed : 1;
	uint16_t max_write_data_blk_length : 4;
	uint16_t write_speed_factor : 3;
	uint16_t manufacturer_default_ecc : 2;
	uint16_t write_prot_group_enable : 1;
	
	uint16_t write_prot_group_size : 5;
	uint16_t erase_group_size_mult : 5;
	uint16_t erase_group_size : 5;
	uint16_t device_size_mult : 3;

	uint16_t vdd_w_cur_max : 3;
	uint16_t vdd_w_cur_min : 3;
	uint16_t vdd_r_cur_max : 3;
	uint16_t vdd_r_cur_min : 3;

	uint16_t device_size : 12;
	uint16_t reserved2 : 2;

	uint16_t dsr_implemented : 1;
	uint16_t read_blk_misalignment : 1;
	uint16_t write_blk_misalignment : 1;
	
	uint16_t partial_block_for_read_allowed : 1;
	uint16_t max_read_data_block_length : 4;
	
	uint16_t device_command_classes : 12;
	uint16_t max_bus_clk_frequency : 8;
	uint16_t data_read_access_time_in_2_clk_cycles : 8;
	uint16_t data_read_access_time_1 : 8;

	uint16_t reserved3 : 2;
	
	uint16_t system_specification_version : 4;
	uint16_t csd_structure : 2;
} __attribute__((packed)) EmmcCardDescriptor;
static_assert(sizeof(EmmcCardDescriptor) == CARD_DESCRIPTOR_SIZE);

typedef struct EmmcExtendedCardDescriptor {	
	uint8_t reserved[15];
	uint8_t command_queue_mode_enable;
	uint8_t secure_removal_type;
	uint8_t product_state_awareness_enablement;
	uint32_t max_preloading_data_size;
	uint32_t preloading_data_size;
	uint8_t ffu_status;
	uint8_t reserved2[2];
	uint8_t mode_operation_codes;
	uint8_t mode_config;
	uint8_t barrier_control;
	uint8_t flush_cache;
	uint8_t cache_control;
	uint8_t power_off_notification;
	uint8_t packed_failure_index;
	uint8_t packed_command_status;
	uint8_t context_configuration[15];
	uint16_t extended_partitions_attribute;
	uint16_t exception_events_status;
	uint16_t exception_events_control;
	uint8_t dyncap_needed;
	uint8_t class_6_ctrl;
	uint8_t ini_timeout_emu;
	uint8_t data_sector_size;
	uint8_t use_native_sector;
	uint8_t native_sector_size;
	uint8_t vendor_specific_feild[64];
	uint8_t program_cid_csd_ddr_support;
	uint8_t periodic_wakeup;
	uint8_t case_is_temperature_controlled;
	uint8_t production_state_awareness;
	uint8_t bad_block_management_mode;
	uint8_t reverved3;
	uint32_t enchanged_user_data_start_address;
	uint32_t enchanged_user_data_area_size : 24;
	uint8_t general_purpose_partition_size[12];
	uint8_t partitioning_setting;
	uint8_t partitions_attribute;
	uint32_t max_enchanted_area_size : 24;
	uint8_t partitioning_support;
	uint8_t hpi_management;
	uint8_t hw_reset_function;
	uint8_t bg_operations_handshake_enabled;
	uint8_t manual_start_bg_operations;
	uint8_t start_sanitize_operations;
	uint8_t write_reliability_parameter_register;
	uint8_t write_reliability_setting_register;
	uint8_t rpmb_size;
	uint8_t fw_configuration;
	uint8_t reverved4;
	uint8_t user_area_write_protect_register;
	uint8_t reverved5;
	uint8_t boot_bus_conditions;
	uint8_t boot_bus_protection;
	uint8_t partition_configuration;
	uint8_t erased_memory_content;
	uint8_t reserved6;
	uint8_t bus_width_mode;
	uint8_t strobe_support;
	uint8_t high_speed_interface_timing;
	uint8_t reserved7;
	uint8_t power_class;
	uint8_t reserved8;
	uint8_t command_set_revision;
	uint8_t reserved9;
	uint8_t command_set;
	// modes segment
	uint8_t extended_csd_revision;
	uint8_t reserved10;
	uint8_t csd_structure;
	uint8_t reserved11;
	uint8_t device_type;
	uint8_t driver_strength;
	uint8_t out_of_interupt_time;
	uint8_t partition_switch_time;


	
	
} __attribute__((packed)) EmmcExtendedCardDescriptor;

typedef struct GcInfo {
	EmmcCardId card_id; 
	EmmcCardDescriptor card_descriptor; 
	uint8_t extra_card_descriptor[0x200];
	uint8_t extra_card_descriptor_revision;

	uint16_t month;
	uint16_t year;
	
	uint16_t key_id;
	GcCmd56Keys key_set;
	GcNpdrmKeys npdrm_keys;
} GcInfo;

void mmc_datetime_from_byte(uint8_t rev, uint8_t mdt, uint16_t* year, uint16_t* month);
const char* mmc_vendor_id_to_manufacturer(uint8_t vendor_id);
const char* keyid_to_keygroup(uint16_t key_id);
const char* mmc_device_type_to_string(uint8_t device_type);

void get_gc_info(GcInfo* info);

int dump_report(const char* output_file);

#endif // GC_IDENT_H