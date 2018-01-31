#include <stdio.h>
#include <stdlib.h>
//
// qemu_toflash
// 
// python /home/olas/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 
// 0x1000  /home/olas/esp/esp-idf/examples/system/ota/build/bootloader/bootloader.bin  
// 0x10000 /home/olas/esp/esp-idf/examples/system/ota/build/ota.bin 
// 0x8000 /home/olas/esp/esp-idf/examples/system/ota/build/partitions_two_ota.bin
//
/*
 0xe000 /home/olas/esp/qemu_esp32/examples/35_gps/components/arduino/tools/partitions/boot_app0.bin 
0x1000 /home/olas/esp/qemu_esp32/examples/35_gps/build/bootloader/bootloader.bin 
0xf000 /home/olas/esp/qemu_esp32/examples/35_gps/build/phy_init_data.bin 
0x10000 /home/olas/esp/qemu_esp32/examples/35_gps/build/app-template.bin 
0x8000 /home/olas/esp/qemu_esp32/examples/35_gps/build/default.bin
*/


void merge_flash(char *binfile,char *flashfile,int flash_pos,int patch_hash)
{
    FILE *fbin;
    FILE *fflash;
    unsigned char *tmp_data;
    int j=0;

    int file_size=0;
    int flash_size=0;

    fbin = fopen(binfile, "r");
    if (fbin == NULL) {
        printf("   Can't open '%s' for reading.\n", binfile);
		return;
	}

    if (fseek(fbin, 0 , SEEK_END) != 0) {
       /* Handle Error */
    }
    file_size = ftell(fbin);

    if (fseek(fbin, 0 , SEEK_SET) != 0) {
      /* Handle Error */
    }

    fflash  = fopen(flashfile, "r+");
    if (fflash == NULL) {
        printf("   Can't open '%s' for writing.\n", flashfile);
        return;
    }
    if (fseek(fflash, 0 , SEEK_END) != 0) {
       /* Handle Error */
    }
    flash_size = ftell(fflash);
    rewind(fflash);
    fseek(fflash,flash_pos,SEEK_SET);


    tmp_data=malloc((1+file_size)*sizeof(char));

    if (file_size<=0) {
      printf("Not able to get file size %s",binfile);
    }

    int len_read=fread(tmp_data,sizeof(char),file_size,fbin);

    if (patch_hash==1) {
      for (j=0;j<33;j++)
	{
          tmp_data[file_size-j]=0;
	}
    }

    
    int len_write=fwrite(tmp_data,sizeof(char),file_size,fflash);

    if (len_read!=len_write) {
      printf("Not able to merge %s",binfile);
    }

    fclose(fbin);

    if (fseek(fflash, 0x3E8000*4 , SEEK_SET) != 0) {
    }

    fclose(fflash);

    free(tmp_data);
}


int main(int argc,char *argv[])
{

    // Overwrites esp32flash.bin file
    system("dd if=/dev/zero bs=1M count=4  | tr \"\\000\" \"\\377\" >  esp32flash.bin");

    // Add bootloader
    merge_flash("build/bootloader/bootloader.bin","esp32flash.bin",0x1000,0);
    // Add partitions,
    merge_flash("components/arduino/tools/partitions/boot_app0.bin","esp32flash.bin",0xe000,0);
    //merge_flash("build/phy_init_data.bin","esp32flash.bin",0xf000,0);
    merge_flash("build/default.bin","esp32flash.bin",0x8000,0);

    // Add application
    merge_flash(argv[1],"esp32flash.bin",0x10000,0);

    system("cp esp32flash.bin ~/qemu_esp32");
}

