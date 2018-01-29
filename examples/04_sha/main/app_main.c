#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "qemu_sha.h"
#include "sha256.h"

#define MAX_ALLOCS 20


#include <stdio.h>


void print_hash(unsigned char hash[])
{
   int idx;
   for (idx=0; idx < 32; idx++)
      printf("%02x",hash[idx]);
   printf("\n");
}

/*
Output should be:
88d4266fd4e6338d13b845fcf289579d209c897823b9217da3e161936f031589
248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
f48d00cb89d8e79ada420b8bc839ee6c9d61b5b36b90e91b076d0fa641892c3e
*/
const unsigned char text1[]={"abcd"};
const unsigned char text2[]={"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"};
const unsigned char text3[]={"aaaaaaaaaaaa"};


const unsigned char text0[32*8];

void test_main(void *param) {
   unsigned char hash[32];
   int idx;
   SHA256_CTX ctx;

   memset(text0,0,sizeof(text0));
   strcpy(text0,"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl");
   tsha256_init(&ctx);
   tsha256_update(&ctx,text0,60);
   tsha256_final(&ctx,hash);
   print_hash(hash);

   // Hash one
   tsha256_init(&ctx);
   tsha256_update(&ctx,text1,strlen((char *)text1));
   tsha256_final(&ctx,hash);
   print_hash(hash);

   // Hash two
   tsha256_init(&ctx);
   tsha256_update(&ctx,text2,strlen((char *)text2));
   tsha256_final(&ctx,hash);
   print_hash(hash);

   // Hash three
   tsha256_init(&ctx);
   for (idx=0; idx < 100000; ++idx)
      tsha256_update(&ctx,text3,strlen((char *)text3));
   tsha256_final(&ctx,hash);
   print_hash(hash);


   vTaskDelete(NULL);
}
void app_main(void)
{
    SHA256_CTX ctx;
    unsigned char hash[32];
    nvs_flash_init();
    int idx;

    int *quemu_test=(int *) 0x3ff005f2;
    // Turn on wardware sha
    *quemu_test=0x01;
    
    printf("starting sha test\n");

    xTaskCreatePinnedToCore(&test_main, "test_main", 4096, NULL, 20, NULL, 0);


    printf("HW accelerated hash\n");

    // Compare to local sha256
    //tsha256_init(&ctx);
    //tsha256_update(&ctx,text0,60);
    //tsha256_final(&ctx,hash);
    //print_hash(hash);


    qemu_sha256_handle_t handle=qemu_sha256_start();
    qemu_sha256_data(handle,text0,60);
    qemu_sha256_finish(handle,hash);
    print_hash(hash);

    //handle=qemu_sha256_start();
    //qemu_sha256_data(handle,text0,8);
    //qemu_sha256_finish(handle,hash);
    //print_hash(hash);


    handle=qemu_sha256_start();
    qemu_sha256_data(handle,text1,strlen((char *)text1));
    qemu_sha256_finish(handle,hash);
    print_hash(hash);

    handle=qemu_sha256_start();
    qemu_sha256_data(handle,text2,strlen((char *)text2));
    qemu_sha256_finish(handle,hash);
    print_hash(hash);

    handle=qemu_sha256_start();
    for (idx=0; idx < 100000; ++idx)
      qemu_sha256_data(handle,text3,strlen((char *)text3));

    //qemu_sha256_data(handle,text3,strlen((char *)text3));
    qemu_sha256_finish(handle,hash);
    print_hash(hash);

}
