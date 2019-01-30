#include <unistd.h>
#include <getopt.h>

#include <stdio.h>

int main(int argc, char * const argv[])
{
        int opt;
        while ((opt = getopt(argc, argv, "nb:o::t")) != -1) {
                printf("opt = %c, optarg = %s, optind = %d, argv[%d] = %s\n", 
                                opt, optarg, optind, optind-1, argv[optind - 1]);
        }
        return 0;
}

// test data
/**
$ ./a.exe -x -y -z
a: unknown option -- x
opt = ?, optarg = (null), optind = 2, argv[1] = -x
a: unknown option -- y
opt = ?, optarg = (null), optind = 3, argv[2] = -y
a: unknown option -- z
opt = ?, optarg = (null), optind = 4, argv[3] = -z

$ ./a.exe -n -b xzz -t
opt = n, optarg = (null), optind = 2, argv[1] = -n
opt = b, optarg = xzz, optind = 4, argv[3] = xzz
opt = t, optarg = (null), optind = 5, argv[4] = -t

$ ./a.exe -n -bxzz -t
opt = n, optarg = (null), optind = 2, argv[1] = -n
opt = b, optarg = xzz, optind = 3, argv[2] = -bxzz
opt = t, optarg = (null), optind = 4, argv[3] = -t

$ ./a.exe -b -t
opt = b, optarg = -t, optind = 3, argv[2] = -t


$ ./a.exe -t -b
opt = t, optarg = (null), optind = 2, argv[1] = -t
a: option requires an argument -- b
opt = ?, optarg = (null), optind = 3, argv[2] = -b

$ ./a.exe -t a -b xzz
opt = t, optarg = (null), optind = 2, argv[1] = -t

$ ./a.exe -t a -bxzz
opt = t, optarg = (null), optind = 2, argv[1] = -t

$ ./a.exe -t  -bxzz
opt = t, optarg = (null), optind = 2, argv[1] = -t
opt = b, optarg = xzz, optind = 3, argv[2] = -bxzz

$ ./a.exe -t a -bxzz
opt = t, optarg = (null), optind = 2, argv[1] = -t

$ ./a.exe -ntb xzz
opt = n, optarg = (null), optind = 1, argv[0] = ./a
opt = t, optarg = (null), optind = 1, argv[0] = ./a
opt = b, optarg = xzz, optind = 3, argv[2] = xzz
*/
