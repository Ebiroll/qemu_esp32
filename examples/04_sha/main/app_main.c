#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "qemu_sha.h"
#include "sha256.h"

#define MAX_ALLOCS 20


#include <stdio.h>

/*
Output should be:
ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0
*/

void print_hash(unsigned char hash[])
{
   int idx;
   for (idx=0; idx < 32; idx++)
      printf("%02x",hash[idx]);
   printf("\n");
}

int test_main()
{
   const unsigned char text1[]={"abc"},
                 text2[]={"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"},
                 text3[]={"aaaaaaaaaa"},
                 hash[32];
   int idx;
   SHA256_CTX ctx;

   // Hash one
   sha256_init(&ctx);
   sha256_update(&ctx,text1,strlen((char)text1));
   sha256_final(&ctx,hash);
   print_hash(hash);

   // Hash two
   sha256_init(&ctx);
   sha256_update(&ctx,text2,strlen((char)text2));
   sha256_final(&ctx,hash);
   print_hash(hash);

   // Hash three
   sha256_init(&ctx);
   for (idx=0; idx < 100000; ++idx)
      sha256_update(&ctx,text3,strlen((char)text3));
   sha256_final(&ctx,hash);
   print_hash(hash);

   getchar();
   return 0;
}
void app_main(void)
{
    nvs_flash_init();
    
    printf("starting sha test\n");

    test_main();

    //qemu_sha256_start();
    //qemu_sha256_data(qemu_sha256_handle_t handle, const void *data, size_t data_len);
}
