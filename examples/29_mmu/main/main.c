#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <esp_spi_flash.h>
#include <soc/dport_reg.h>
#include <soc/cpu.h>
#include <rom/cache.h>


// MMU Table Entry

// Test #1
int MMUTableEntry = 96;  // 0x4020 0000
volatile uint32_t *b_instructions = (volatile uint32_t)0x40200000;

// Test #2
//int MMUTableEntry = 128; // 0x4040 0000
//volatile uint32_t *b_instructions = (volatile uint32_t)0x40400000;

// Test #3
//int MMUTableEntry = 192; // 0x4080 0000
//volatile uint32_t *b_instructions = (volatile uint32_t)0x40800000;

extern void spi_flash_disable_interrupts_caches_and_other_cpu();

extern void spi_flash_enable_interrupts_caches_and_other_cpu();



void IRAM_ATTR mapFlash() {

	printf("Setting the flash mapping\n");
	spi_flash_disable_interrupts_caches_and_other_cpu();
	
	
	DPORT_PRO_FLASH_MMU_TABLE[MMUTableEntry] = 48;
	DPORT_APP_FLASH_MMU_TABLE[MMUTableEntry] = 48;

	Cache_Flush(0);
	Cache_Flush(1);
	spi_flash_enable_interrupts_caches_and_other_cpu();
}

void app_main(void)
{
  // https://github.com/espressif/esp-idf/issues/1667

  /* When the DPORT_PRO_SINGLE_IRAM_ENA bit of register DPORT_PRO_CACHE_CTRL_REG is 1, the MMU enters this special mode for PRO_CPU memory accesses. Similarily, when the DPORT_APP_SINGLE_IRAM_ENA bit of register DPORT_APP_CACHE_CTRL_REG is 1, the APP_CPU accesses memory using this special mode. In this mode, the process and virtual address page supported by each configuration entry of MMU are different. For details please see Table 104 and 105. As shown in these tables, in this special mode V Addr2 and V Addr3 cannot be used to access External Flash.
   */
																  
  
      printf("DPORT_PRO_SINGLE_IRAM_ENA: %ld\n", DPORT_PRO_CACHE_CTRL_REG & DPORT_PRO_SINGLE_IRAM_ENA);
      printf("DPORT_APP_SINGLE_IRAM_ENA: %ld\n", DPORT_APP_CACHE_CTRL_REG & DPORT_APP_SINGLE_IRAM_ENA);
  
	mapFlash();
	printf("%08x %08x %08x %08x\n", b_instructions[0], b_instructions[1], b_instructions[2], b_instructions[3]);
}
