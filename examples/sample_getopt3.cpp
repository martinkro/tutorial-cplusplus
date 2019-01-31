#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>


const char* VERSION = "2019.1.30";
const char* PROGRAM = "memdump";

void print_help(int exit_code);
void parse_opt(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    // printf("memdump version:%s\n", VERSION);
    parse_opt(argc,argv);
    return 0;
}

void print_help(int exit_code)
{
    printf("%s(%s) show working example\n",PROGRAM,VERSION);
    printf("%s [-h][-V][-f FILE][-o FILE]\n\n",PROGRAM);

    printf("  -h\t print this help and exit\n");
    printf("  -V\t print version and exit\n\n");
    printf("  -v\t set verbose flag\n");
    printf("  -f FILE\t set input file\n");
    printf("  -o FILE\t set output file\n");
    exit(exit_code);
}

void parse_opt(int argc, char* argv[])
{
    int opt;
    if (argc == 1){
        fprintf(stderr,"This program needs arguments...\n\n");
        print_help(1);
    }

    while((opt = getopt(argc,argv,"hVvf:o:")) != -1){
        switch(opt){
            case 'h':
                print_help(0);
                break;
            case 'V':
                printf("%s %s\n\n",PROGRAM,VERSION);
                exit(0);
                break;
            case 'v':
                printf("%s:Verbose option is set '%c'",PROGRAM,optopt);
                break;
            case 'f':
                printf("%s:Filename %s\n", PROGRAM,optarg);
                break;
            case 'o':
                printf("Output:%s\n",optarg);
                break;
            case ':':
                fprintf(stderr,"%s:Error -Option '%c' needs a value\n\n",PROGRAM,optopt);
                print_help(1);
                break;
            case '?':
                fprintf(stderr, "%s: Error - No such option: '%c'\n", PROGRAM,optopt);
                print_help(1);
                break;
            default:
                break;
        }
    }
}