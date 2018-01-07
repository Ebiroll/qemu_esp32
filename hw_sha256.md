# To emulate hw sha256 calculation


```
  Component config  --->    
       mbedTLS  --->
          [ ] Enable hardware SHA acceleration                                     

So now the toflash.c program will overwrite the checksum with 00000
If you insist on using an older esp-idf try commenting out this, as
older bootloader relies on proper hash of the image

    if (patch_hash==1) {
      for (j=0;j<33;j++)
	{
          tmp_data[file_size-j]=0;
	}
    }


D (22) esp_image: Calculated hash: 00000000
D (22) bootloader_flash: mmu set paddr=00030000 count=1
E (22) esp_image: Image hash failed - image is corrupt
D (22) esp_image: Expected hash: 1bb917b3
E (22) boot: Factory app partition is not bootable
D (22) boot: Can't boot from zero-length partition
E (22) boot: No bootable app partitions in the partition table
user code done


Here are some usehul registers...

ets_sha_enable();  
io read 1c 
io write 1c,2 
io read 20 
io write 20,0 



while(REG_READ(SHA_256_BUSY_REG) != 0) { }
io read 309c

for (int i = 0; i < copy_words; i++) {
  sha_text_reg[block_count + i] = __builtin_bswap32(w[i]);
}                                                                        

io write 3000,e9070210 
io write 3004,2c0e0840 
io write 3008,ee000000
io write 300c,0 
io write 3010,0 
io write 3014,1 

io write 3000,2000000 
io write 3004,8c3b0840 
io write 3008,3000000 
io write 300c,8c3b0840 
io write 3010,4000000 
io write 3014,8c3b0840 
io write 3018,5000000 
io write 301c,8c3b0840 
io write 3020,6000000 
io write 3024,8c3b0840 
io write 3028,7000000 
io write 302c,8c3b0840 
io write 3030,8000000 
io write 3034,8c3b0840 
io write 3038,9000000 
io write 303c,8c3b0840 



io write 3030,80000000 
io write 3034,0 
io write 3038,0




Set a breakopint in 
process_segment()


REG_WRITE(SHA_256_CONTINUE_REG, 1);
io write 3094,1


REG_WRITE(SHA_256_LOAD_REG, 1);
io write 3098,1


for (int i = 0; i < DIGEST_WORDS; i++) {
  digest_words[i] = __builtin_bswap32(sha_text_reg[i]);
}
io read 309c 
io read 3000 
io read 3004 
io read 3008 
io read 300c 
io read 3010 
io read 3014 
io read 3018 
io read 301c 





```