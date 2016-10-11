#include <stdio.h>


void save_rom(char *infile,char *outfile)
{
    FILE *fdump;
    FILE *fbin;
    char ptr[100];
    unsigned int data[10];
    unsigned int sum;
    unsigned int calc_sum=0;

    int lineno=0;

    fdump = fopen(infile, "r");
    if (fdump == NULL) {
        printf("   Can't open '%s' for reading.\n", infile);
		return;
	}
    fbin  = fopen(outfile, "w");
    if (fbin == NULL) {
        printf("   Can't open '%s' for writing.\n", outfile);
        return;
    }

    while(fgets(ptr, 99, fdump)) {
        //printf("line=%s \n",ptr);

        if (!sscanf(ptr, "%x,%x,%x,%x,%x,%x,%x,%x,:%x:", &data[0],&data[1],&data[2],&data[3],&data[4],&data[5],&data[6],&data[7],&sum))
        {
                printf("Failed read line %d\n",lineno);
        }
        lineno++;
        for (int j=0;j<8;j++) {
                calc_sum = calc_sum ^ data[j];
                fwrite(&data[j], sizeof(unsigned int), 1, fbin);
                //printf("%08X ",data[j]);
        }
        if (calc_sum!=sum)
        {
                printf("Check error line %d,%08X:%08X\n",lineno,calc_sum,sum);
        }
        calc_sum=0;
    }
}


int main(int argc,char *argv[])
{

    // Parse test.log and put in rom.bin
    save_rom("test.log","rom.bin");

}

