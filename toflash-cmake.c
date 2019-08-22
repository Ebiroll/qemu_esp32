#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
// qemu_toflash
// 
// python /home/olas/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 
// 0x1000  /home/olas/esp/esp-idf/examples/system/ota/build/bootloader/bootloader.bin  
// 0x10000 /home/olas/esp/esp-idf/examples/system/ota/build/ota.bin 
// 0x8000 /home/olas/esp/esp-idf/examples/system/ota/build/partitions_two_ota.bin
//

void merge_flash(char *binfile,char *flashfile,int flash_pos,int patch_hash)
{
    FILE *fbin;
    FILE *fflash;
    unsigned char *tmp_data;
    int j=0;

    int file_size=0;
    int flash_size=0;

    fbin = fopen(binfile, "rb");
    if (fbin == NULL) {
        printf("   Can't open '%s' for reading.\n", binfile);
		return;
	}

    if (fseek(fbin, 0 , SEEK_END) != 0) {
        printf("   Can't seek end of '%s'.\n", binfile);
       /* Handle Error */
    }
    file_size = ftell(fbin);

    if (fseek(fbin, 0 , SEEK_SET) != 0) {
      /* Handle Error */
    }

    fflash  = fopen(flashfile, "rb+");
    if (fflash == NULL) {
        printf("   Can't open '%s' for writing.\n", flashfile);
        return;
    }
    if (fseek(fflash, 0 , SEEK_END) != 0) {
          printf("   Can't seek end of '%s'.\n", flashfile);
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
      printf("Not able to merge %s, %d bytes read,%d to write,%d file_size\n",binfile,len_read,len_write,file_size);
      //while()


    }

    fclose(fbin);

    if (fseek(fflash, 0x3E8000*4 , SEEK_SET) != 0) {
    }

    fclose(fflash);

    free(tmp_data);
}


int main(int argc,char *argv[])
{

    if (argc>1) {
      if (strcmp(argv[1],"-bl")==0)  {
        printf("Overwrite bootloader only \n");
        merge_flash("build/bootloader/bootloader.bin","esp32flash.bin",0x1000,0);
        system("cp esp32flash.bin ~/qemu_esp32");
        exit(0);

      }
    }
    // Overwrites esp32flash.bin file

    system("dd if=/dev/zero bs=1M count=4  | tr \"\\000\" \"\\377\" >  esp32flash.bin");

    // Add bootloader
    merge_flash("build/bootloader/bootloader.bin","esp32flash.bin",0x1000,0);
    // Add partitions, 
    merge_flash("build/partition_table/partition-table.bin","esp32flash.bin",0x8000,0);
    // Add application
    if (argc>1) {
      merge_flash(argv[1],"esp32flash.bin",0x10000,0);
    } else {
      merge_flash("build/app-template.bin","esp32flash.bin",0x10000,0);
    }
    system("cp esp32flash.bin ~/qemu_esp32");
}

