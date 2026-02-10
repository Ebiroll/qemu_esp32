#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
//
// qemu_toflash
// 
// Example (PlatformIO, ESP32):
// pio run -e lilygo-t-display
// esptool.py write_flash -z \
//   0x1000 bootloader.bin \
//   0x8000 partitions.bin \
//   0xE000 boot_app0.bin (optional, for OTA partition tables) \
//   0x10000 firmware.bin
//
// Options:
//   -base <flash.bin>  Use an existing flash image as the base (copied to esp32flash.bin)
//   -only-app          Only replace firmware at 0x10000 (requires -base)
//   -fw <firmware.bin> Override firmware path (defaults to PlatformIO firmware.bin)
//
// Default paths below are set for:
// examples/06_ttgo_display/FactoryTest/.pio/build/lilygo-t-display
//

// PlatformIO build output for the TTGO T-Display example.
#ifndef PIO_BUILD_DIR
#define PIO_BUILD_DIR "examples/06_ttgo_display/FactoryTest/.pio/build/lilygo-t-display"
#endif

static const char *select_pio_build_dir(void)
{
    if (access(".pio/build/lilygo-t-display/firmware.bin", R_OK) == 0) {
        return ".pio/build/lilygo-t-display";
    }
    if (access(PIO_BUILD_DIR "/firmware.bin", R_OK) == 0) {
        return PIO_BUILD_DIR;
    }
    return PIO_BUILD_DIR;
}

static void build_path(char *out, size_t out_sz, const char *dir, const char *file)
{
    snprintf(out, out_sz, "%s/%s", dir, file);
}

static bool file_exists(const char *path)
{
    return access(path, R_OK) == 0;
}

static int copy_file(const char *src, const char *dst)
{
    FILE *fin = fopen(src, "rb");
    if (fin == NULL) {
        printf("   Can't open '%s' for reading.\n", src);
        return -1;
    }
    FILE *fout = fopen(dst, "wb");
    if (fout == NULL) {
        printf("   Can't open '%s' for writing.\n", dst);
        fclose(fin);
        return -1;
    }
    unsigned char buf[4096];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0) {
        if (fwrite(buf, 1, n, fout) != n) {
            printf("   Short write to '%s'.\n", dst);
            fclose(fin);
            fclose(fout);
            return -1;
        }
    }
    if (ferror(fin)) {
        printf("   Read error from '%s'.\n", src);
        fclose(fin);
        fclose(fout);
        return -1;
    }
    fclose(fin);
    fclose(fout);
    return 0;
}

int merge_flash(const char *binfile, const char *flashfile, int flash_pos, int patch_hash)
{
    FILE *fbin;
    FILE *fflash;
    unsigned char *tmp_data;
    int j=0;

    int file_size=0;

    fbin = fopen(binfile, "rb");
    if (fbin == NULL) {
        printf("   Can't open '%s' for reading.\n", binfile);
		return -1;
	}

    if (fseek(fbin, 0 , SEEK_END) != 0) {
        printf("   Can't seek end of '%s'.\n", binfile);
        fclose(fbin);
        return -1;
    }
    file_size = ftell(fbin);

    if (fseek(fbin, 0 , SEEK_SET) != 0) {
        fclose(fbin);
        return -1;
    }

    fflash  = fopen(flashfile, "rb+");
    if (fflash == NULL) {
        printf("   Can't open '%s' for writing.\n", flashfile);
        fclose(fbin);
        return -1;
    }
    if (fseek(fflash, flash_pos, SEEK_SET) != 0) {
        printf("   Can't seek to 0x%x in '%s'.\n", flash_pos, flashfile);
        fclose(fbin);
        fclose(fflash);
        return -1;
    }

    if (file_size <= 0) {
        printf("   Not able to get file size '%s'.\n", binfile);
        fclose(fbin);
        fclose(fflash);
        return -1;
    }

    tmp_data=malloc((1+file_size)*sizeof(char));
    if (tmp_data == NULL) {
        printf("   Out of memory reading '%s'.\n", binfile);
        fclose(fbin);
        fclose(fflash);
        return -1;
    }

    int len_read=fread(tmp_data,sizeof(char),file_size,fbin);
    if (len_read != file_size) {
        printf("   Short read '%s': %d of %d bytes.\n", binfile, len_read, file_size);
        fclose(fbin);
        fclose(fflash);
        free(tmp_data);
        return -1;
    }

    if (patch_hash==1) {
        for (j=0;j<33;j++) {
            tmp_data[file_size-j]=0;
        }
    }

    
    int len_write=fwrite(tmp_data,sizeof(char),file_size,fflash);

    if (len_read!=len_write) {
        printf("Not able to merge %s, %d bytes read,%d to write,%d file_size\n",binfile,len_read,len_write,file_size);
        fclose(fbin);
        fclose(fflash);
        free(tmp_data);
        return -1;
    }

    fclose(fbin);

    if (fseek(fflash, 0x3E8000*4 , SEEK_SET) != 0) {
    }

    fclose(fflash);

    free(tmp_data);
    return 0;
}


int main(int argc,char *argv[])
{
    const char *pio_dir = select_pio_build_dir();
    char bootloader_path[PATH_MAX];
    char partitions_path[PATH_MAX];
    char firmware_path[PATH_MAX];
    char boot_app0_path[PATH_MAX];
    const char *base_path = NULL;
    const char *firmware_override = NULL;
    bool only_app = false;
    bool have_boot_app0 = false;

    build_path(bootloader_path, sizeof(bootloader_path), pio_dir, "bootloader.bin");
    build_path(partitions_path, sizeof(partitions_path), pio_dir, "partitions.bin");
    build_path(firmware_path, sizeof(firmware_path), pio_dir, "firmware.bin");
    build_path(boot_app0_path, sizeof(boot_app0_path), pio_dir, "boot_app0.bin");

    if (file_exists(boot_app0_path)) {
        have_boot_app0 = true;
    } else {
        const char *home = getenv("HOME");
        if (home != NULL) {
            snprintf(boot_app0_path, sizeof(boot_app0_path),
                     "%s/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin",
                     home);
            if (file_exists(boot_app0_path)) {
                have_boot_app0 = true;
            }
        }
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-bl") == 0) {
            printf("Overwrite bootloader only \n");
            if (!file_exists(bootloader_path)) {
                printf("Missing bootloader: %s\n", bootloader_path);
                return 1;
            }
            if (merge_flash(bootloader_path,"esp32flash.bin",0x1000,0) != 0) {
                return 1;
            }
            system("cp esp32flash.bin ~/qemu_esp32");
            return 0;
        } else if (strcmp(argv[i], "-base") == 0 && i + 1 < argc) {
            base_path = argv[++i];
        } else if (strcmp(argv[i], "-fw") == 0 && i + 1 < argc) {
            firmware_override = argv[++i];
        } else if (strcmp(argv[i], "-only-app") == 0) {
            only_app = true;
        } else if (strcmp(argv[i], "-base") == 0) {
            printf("Option -base requires a file path.\n");
            return 1;
        } else if (strcmp(argv[i], "-fw") == 0) {
            printf("Option -fw requires a file path.\n");
            return 1;
        } else if (argv[i][0] == '-') {
            printf("Unknown option: %s\n", argv[i]);
            return 1;
        } else {
            firmware_override = argv[i];
        }
    }

    // Overwrites esp32flash.bin file

    if (only_app && base_path == NULL) {
        printf("Option -only-app requires -base <flash.bin>.\n");
        return 1;
    }

    if (base_path != NULL) {
        if (!file_exists(base_path)) {
            printf("Missing base image: %s\n", base_path);
            return 1;
        }
        if (copy_file(base_path, "esp32flash.bin") != 0) {
            return 1;
        }
    } else {
        system("dd if=/dev/zero bs=1M count=4  | tr \"\\000\" \"\\377\" >  esp32flash.bin");
    }

    if (!only_app) {
        if (!file_exists(bootloader_path)) {
            printf("Missing bootloader: %s\n", bootloader_path);
            return 1;
        }
        if (!file_exists(partitions_path)) {
            printf("Missing partitions: %s\n", partitions_path);
            return 1;
        }
        if (merge_flash(bootloader_path,"esp32flash.bin",0x1000,0) != 0) {
            return 1;
        }
        // Optional boot_app0 (used by OTA partition tables)
        if (have_boot_app0) {
            if (merge_flash(boot_app0_path,"esp32flash.bin",0xE000,0) != 0) {
                return 1;
            }
        }
        // Add partitions, 
        if (merge_flash(partitions_path,"esp32flash.bin",0x8000,0) != 0) {
            return 1;
        }
    }

    // Add application
    if (firmware_override != NULL) {
        if (!file_exists(firmware_override)) {
            printf("Missing firmware: %s\n", firmware_override);
            return 1;
        }
        if (merge_flash(firmware_override,"esp32flash.bin",0x10000,0) != 0) {
            return 1;
        }
    } else {
        if (!file_exists(firmware_path)) {
            printf("Missing firmware: %s\n", firmware_path);
            return 1;
        }
        if (merge_flash(firmware_path,"esp32flash.bin",0x10000,0) != 0) {
            return 1;
        }
    }
    system("cp esp32flash.bin ~/qemu_esp32");
}
