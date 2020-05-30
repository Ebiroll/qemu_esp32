#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
// s2rompatch
// 
// 
//




void merge_rom(const unsigned char *data,char *flashfile,int flash_pos,int size)
{
    FILE *fflash;
    int j=0;

    int flash_size=0;

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


    int len_write=fwrite(data,sizeof(char),size,fflash);

    fclose(fflash);

}


int main(int argc,char *argv[])
{
 static const unsigned char patch_ret[] = {
                 0x06,0x03,0x00,  // Jump
                 0x3d,0xf0,   // NOP 
                 0x3d,0xf0,   // NOP
                 0xff,        // To get next instruction correctly
                 0x91, 0xfc, 0xff, 0x81 , 0xfc , 0xff , 0x99, 0x08 , 0xc, 0x2, 0x1d, 0xf0
            };


  

    // Add patch Cache_Set_Default_Mode
    merge_rom(patch_ret,"s2rom.bin",0x0001810c+3,sizeof(patch_ret));

    //Cache_Invalidate_ICache_Items
    merge_rom(patch_ret,"s2rom.bin",0x000181b8+3,sizeof(patch_ret));

    // Cache_Suspend_ICache = 0x40018ca4 
    merge_rom(patch_ret,"s2rom.bin",0x00018ca4+3,sizeof(patch_ret));

    // SPIUnlock
    merge_rom(patch_ret,"s2rom.bin",0x00016e88+3,sizeof(patch_ret));    

    
}

