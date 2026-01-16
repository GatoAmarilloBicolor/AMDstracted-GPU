/*
 * PCI Enumeration Test
 * Lists all devices found
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>

int main() {
    printf("=== PCI Device Enumeration ===\n\n");
    
    DIR *dir = opendir("/sys/bus/pci/devices");
    if (!dir) {
        printf("Cannot open /sys/bus/pci/devices\n");
        return 1;
    }
    
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        char vendor_path[256], device_path[256], class_path[256];
        snprintf(vendor_path, sizeof(vendor_path), 
                "/sys/bus/pci/devices/%s/vendor", entry->d_name);
        snprintf(device_path, sizeof(device_path),
                "/sys/bus/pci/devices/%s/device", entry->d_name);
        snprintf(class_path, sizeof(class_path),
                "/sys/bus/pci/devices/%s/class", entry->d_name);
        
        FILE *vf = fopen(vendor_path, "r");
        FILE *df = fopen(device_path, "r");
        FILE *cf = fopen(class_path, "r");
        
        if (vf && df) {
            uint16_t vendor, device;
            uint32_t class;
            fscanf(vf, "%hx", &vendor);
            fscanf(df, "%hx", &device);
            if (cf) {
                fscanf(cf, "%x", &class);
                fclose(cf);
            }
            
            /* Decode class */
            const char *class_name = "Unknown";
            switch ((class >> 16) & 0xFF) {
                case 0x03: class_name = "Display Controller"; break;
                case 0x02: class_name = "Network Controller"; break;
                case 0x06: class_name = "Bridge"; break;
                case 0x01: class_name = "Mass Storage"; break;
            }
            
            printf("%s: %04x:%04x [%s]\n", entry->d_name, vendor, device, class_name);
            count++;
            
            fclose(vf);
            fclose(df);
        }
    }
    
    closedir(dir);
    printf("\nFound %d devices\n", count);
    return 0;
}
