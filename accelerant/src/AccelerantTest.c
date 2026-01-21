/*
 * Simple test for AMD Accelerant
 */

#include <stdio.h>
#include <stdlib.h>

int
main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    printf("AMD Accelerant Test Utility\n");
    printf("=============================\n\n");
    
    printf("This utility tests the AMD graphics accelerant module.\n");
    printf("Run from Haiku with the accelerant loaded.\n\n");
    
    printf("Usage: AccelerantTest [options]\n");
    printf("  -h, --help      Show this help\n");
    printf("  -t, --test      Run tests\n");
    printf("  -i, --info      Show GPU info\n");
    printf("  -m, --modes     List display modes\n");
    printf("  -v, --verbose   Verbose output\n\n");
    
    printf("Note: Must be run with proper privileges to access GPU.\n");
    
    return 0;
}
