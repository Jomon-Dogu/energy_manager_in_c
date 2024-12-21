#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CSV_FILE "system_data.csv"
#define INTERVAL 1  // Zeitintervall in Sekunden
#define NUM_ENTRIES 10000  // Anzahl der zu sammelnden Datensätze

// Funktion zum Parsen der Systemdaten
void parse_system_data(FILE *csv_file) {
    char buffer[1024];
    long cpu_user, cpu_system, cpu_idle;
    long memory_total, memory_free, swap_total, swap_free;
    long disk_read, disk_write;
    long network_rx, network_tx;
    float load_1min, load_5min, load_15min;
    long cpu_frequency;
    int cpu_temperature;

    FILE *proc_file = fopen("/proc/read_system_data", "r");
    if (!proc_file) {
        perror("Failed to open /proc/read_system_data");
        return;
    }

    // Initialisieren der Variablen
    cpu_user = cpu_system = cpu_idle = 0;
    memory_total = memory_free = swap_total = swap_free = 0;
    disk_read = disk_write = network_rx = network_tx = 0;
    load_1min = load_5min = load_15min = 0.0;
    cpu_frequency = 0;
    cpu_temperature = 0;

    // Zeilenweise durch die Datei lesen
    while (fgets(buffer, sizeof(buffer), proc_file)) {
        if (sscanf(buffer, "CPU User: %ld", &cpu_user) == 1)
            continue;
        if (sscanf(buffer, "CPU System: %ld", &cpu_system) == 1)
            continue;
        if (sscanf(buffer, "CPU Idle: %ld", &cpu_idle) == 1)
            continue;
        if (sscanf(buffer, "Memory Total: %ld kB", &memory_total) == 1)
            continue;
        if (sscanf(buffer, "Memory Free: %ld kB", &memory_free) == 1)
            continue;
        if (sscanf(buffer, "Swap Total: %ld kB", &swap_total) == 1)
            continue;
        if (sscanf(buffer, "Swap Free: %ld kB", &swap_free) == 1)
            continue;
        if (sscanf(buffer, "Disk Read: %ld", &disk_read) == 1)
            continue;
        if (sscanf(buffer, "Disk Write: %ld", &disk_write) == 1)
            continue;
        if (sscanf(buffer, "Network RX: %ld", &network_rx) == 1)
            continue;
        if (sscanf(buffer, "Network TX: %ld", &network_tx) == 1)
            continue;
        if (sscanf(buffer, "Load 1min: %f", &load_1min) == 1)
            continue;
        if (sscanf(buffer, "Load 5min: %f", &load_5min) == 1)
            continue;
        if (sscanf(buffer, "Load 15min: %f", &load_15min) == 1)
            continue;
        if (sscanf(buffer, "CPU Frequency: %ld kHz", &cpu_frequency) == 1)
            continue;
        if (sscanf(buffer, "CPU Temperature: %d°C", &cpu_temperature) == 1)
            continue;
    }

    fclose(proc_file);


    

    // In CSV-Datei im richtigen Format speichern
    fprintf(csv_file, "%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %.2f, %.2f, %.2f, %ld, %d\n",
            cpu_user, cpu_system, cpu_idle, memory_total, memory_free, swap_total, swap_free,
            disk_read, disk_write, network_rx, network_tx, load_1min, load_5min, load_15min,
            cpu_frequency, cpu_temperature);
}

int main() {
    FILE *csv_file = fopen(CSV_FILE, "w");
    if (!csv_file) {
        perror("Failed to open CSV file");
        return 1;
    }

    // CSV-Dateikopf
    fprintf(csv_file, "CPU User, CPU System, CPU Idle, Memory Total, Memory Free, Swap Total, Swap Free, Disk Read, Disk Write, Network RX, Network TX, Load 1min, Load 5min, Load 15min, CPU Frequency, CPU Temperature\n");

    for (int i = 0; i < NUM_ENTRIES; i++) {
        printf("Collecting data entry %d\n", i + 1);
        parse_system_data(csv_file);
        sleep(INTERVAL);  // Warte INTERVAL Sekunden, bevor der nächste Datensatz gesammelt wird
    }

    fclose(csv_file);
    printf("Data collection completed. Results are saved in %s\n", CSV_FILE);
    return 0;
}
