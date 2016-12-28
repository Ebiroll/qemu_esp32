#Test of mmap function

Note that when running the program the bootloader will be overwritten.
Running with qemu this should not be a problem.

This is just for testing of the spi_flash_mmap function.


http://iot-bits.com/esp32/esp32-spi-tutorial-part-1/



ESP-IDF uses partition table to maintain information about various regions of SPI flash memory (bootloader, various application binaries, data, filesystems). More information about partition tables can be found in docs/partition_tables.rst.

This component provides APIs to enumerate partitions found in the partition table and perform operations on them. These functions are declared in esp_partition.h:

    esp_partition_find used to search partition table for entries with specific type, returns an opaque iterator
    esp_partition_get returns a structure describing the partition, for the given iterator
    esp_partition_next advances iterator to the next partition found
    esp_partition_iterator_release releases iterator returned by esp_partition_find
    esp_partition_find_first is a convenience function which returns structure describing the first partition found by esp_partition_find
    esp_partition_read, esp_partition_write, esp_partition_erase_range are equivalent to spi_flash_read, spi_flash_write, spi_flash_erase_range, but operate within partition boundaries

Most application code should use esp_partition_* APIs instead of lower level spi_flash_* APIs. Partition APIs do bounds checking and calculate correct offsets in flash based on data stored in partition table.

#Memory mapping APIs

ESP32 features memory hardware which allows regions of flash memory to be mapped into instruction and data address spaces. This mapping works only for read operations, it is not possible to modify contents of flash memory by writing to mapped memory region. Mapping happens in 64KB pages. Memory mapping hardware can map up to 4 megabytes of flash into data address space, and up to 16 megabytes of flash into instruction address space. See the technical reference manual for more details about memory mapping hardware.

Note that some number of 64KB pages is used to map the application itself into memory, so the actual number of available 64KB pages may be less.

Reading data from flash using a memory mapped region is the only way to decrypt contents of flash when flash encryption is enabled. Decryption is performed at hardware level.

Memory mapping APIs are declared in esp_spi_flash.h and esp_partition.h:

    spi_flash_mmap maps a region of physical flash addresses into instruction space or data space of the CPU
    spi_flash_munmap unmaps previously mapped region
    esp_partition_mmap maps part of a partition into the instruction space or data space of the CPU

Differences between spi_flash_mmap and esp_partition_mmap are as follows:

    spi_flash_mmap must be given a 64KB aligned physical address
    esp_partition_mmap may be given an arbitrary offset within the partition, it will adjust returned pointer to mapped memory as necessary

Note that because memory mapping happens in 64KB blocks, it may be possible to read data outside of the partition provided to esp_partition_mmap.
Implementation notes

In order to perform some flash operations, we need to make sure both CPUs are not running any code from flash for the duration of the flash operation. In a single-core setup this is easy: we disable interrupts/scheduler and do the flash operation. In the dual-core setup this is slightly more complicated. We need to make sure that the other CPU doesn’t run any code from flash.

When SPI flash API is called on CPU A (can be PRO or APP), we start spi_flash_op_block_func function on CPU B using esp_ipc_call API. This API wakes up high priority task on CPU B and tells it to execute given function, in this case spi_flash_op_block_func. This function disables cache on CPU B and signals that cache is disabled by setting s_flash_op_can_start flag. Then the task on CPU A disables cache as well, and proceeds to execute flash operation.

While flash operation is running, interrupts can still run on CPUs A and B. We assume that all interrupt code is placed into RAM. Once interrupt allocation API is added, we should add a flag to request interrupt to be disabled for the duration of flash operations.

Once flash operation is complete, function on CPU A sets another flag, s_flash_op_complete, to let the task on CPU B know that it can re-enable cache and release the CPU. Then the function on CPU A re-enables the cache on CPU A as well and returns control to the calling code.

Additionally, all API functions are protected with a mutex (s_flash_op_mutex).

In a single core environment (CONFIG_FREERTOS_UNICORE enabled), we simply disable both caches, no inter-CPU communication takes place.
Application Example

Instructions


#debugging the esp_spi_flash functions with qemu,


#Important to remember

#define SPI_FLASH_SEC_SIZE  4096    /**< SPI Flash sector size */



spi_flash_unlock();


SPIUnlock();


Wait_SPI_Idle(){

 while((REG_READ(SPI_EXT2_REG(SPI_IDX)) & SPI_ST)) {                                             
           }                                                    

  > 0 esp32_spi_read: +0xf8: 0x00000000

    while(REG_READ(SPI_EXT2_REG(OTH_IDX)) & SPI_ST) {                                               
            }  

  > 1 esp32_spi_read: +0xf8: 0x00000000


}


SPIUnlock(void)
{
 REG_WRITE(SPI_CMD_REG(SPI_IDX), SPI_FLASH_WREN); 
>   0 esp32_spi_write: +0x00 = 0x40000000

    while(REG_READ(SPI_CMD_REG(SPI_IDX)) != 0) {  
>   0 esp32_spi_read: +0x00: 0x00000000

   }

  SET_PERI_REG_MASK(SPI_CTRL_REG(SPI_IDX), SPI_WRSR_2B);  
  0 esp32_spi_read: +0x08: 0x00208000
  0 esp32_spi_write: +0x08 = 0x00608000
 

}

SPIEraseSector(sector=0x200)
{

 0 esp32_spi_read: +0x1c: read 0x00000000
 0 esp32_spi_write: +0x1c = 0x00000000
 written 0x0000001c
 0 esp32_spi_read: +0x20: read 0x00000000
 0 esp32_spi_write: +0x20 = 0x5c000000     // USER1
 written 0x00000020



    0x40062d02      quou   a4, a8, a4                                                                         
    0x40062d05      bltu   a2, a4, 0x40062d0c                                                                 
    0x40062d08      movi.n a2, 1                                                                              
    0x40062d0a      retw.n                                                                                    
  > 0x40062d0c      mov.n  a10, a3           




}



 spi_flash_write(base_addr, g_wbuf, sizeof(g_wbuf)); 

  0x200000,  0x3ffb162c  , 1024
  
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 3f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 


 


             deviceid, chip_size ,         ,sector_size,status_mask
SPIParamCfg(0x1540ef, 4*1024*1024, 64*1024, 4096, 256, 0xffff);

    // Set flash chip size
SPIParamCfg(g_rom_flashchip.deviceId, size * 0x100000, 0x10000, 0x1000, 0x100, 0xffff);


Get flash chip size, as set in binary image header

size_t spi_flash_get_chip_size();
{
 return (g_rom_flashchip.chip_size);
}

 p g_rom_flashchip

  0x400000
$2 = {deviceId = 1392879, chip_size = 4194304, block_size = 65536, sector_size = 4096, page_size = 256,
  status_mask = 65535}
