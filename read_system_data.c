#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/types.h>

#define PROC_FILENAME "read_system_data"

// Globale Variablen zum Speichern der Systemdaten
static long cpu_user = 0, cpu_system = 0, cpu_idle = 0;
static long mem_total = 0, mem_free = 0;
static long swap_total = 0, swap_free = 0;
static long disk_read = 0, disk_write = 0;
static long network_rx = 0, network_tx = 0;
static unsigned int cpu_freq = 0;
static long cpu_temp = 0;
static long load_1min = 0, load_5min = 0, load_15min = 0;

// Funktion zum Lesen der CPU-Auslastung
static void read_cpu_usage(void) {
    struct file *file;
    char buffer[128];
    loff_t pos = 0;
    
    file = filp_open("/proc/stat", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /proc/stat\n");
        return;
    }

    kernel_read(file, buffer, sizeof(buffer) - 1, &pos);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-Terminierung sicherstellen
    filp_close(file, NULL);

    if (sscanf(buffer, "cpu  %ld %ld %ld %*ld", &cpu_user, &cpu_system, &cpu_idle) != 3) {
        pr_err("Failed to parse /proc/stat\n");
    }
}

// Funktion zum Lesen der CPU-Frequenz
static unsigned int read_cpu_freq(void) {
    struct file *file;
    char buffer[128];
    loff_t pos = 0;

    file = filp_open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq\n");
        return 0;
    }

    kernel_read(file, buffer, sizeof(buffer) - 1, &pos);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-Terminierung sicherstellen
    filp_close(file, NULL);

    if (sscanf(buffer, "%u", &cpu_freq) != 1) {
        pr_err("Failed to parse CPU frequency\n");
        return 0;
    }

    return cpu_freq;
}

// Funktion zum Lesen der CPU-Temperatur
static void read_cpu_temp(void) {
    struct file *file;
    char buffer[128];
    loff_t pos = 0;

    file = filp_open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /sys/class/thermal/thermal_zone0/temp\n");
        return;
    }

    kernel_read(file, buffer, sizeof(buffer) - 1, &pos);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-Terminierung sicherstellen
    filp_close(file, NULL);

    if (sscanf(buffer, "%ld", &cpu_temp) != 1) {
        pr_err("Failed to parse CPU temperature\n");
        cpu_temp = 0;
        return;
    }

    cpu_temp = cpu_temp / 1000; // Umrechnung in Grad Celsius
}

// Funktion zum Lesen der Load-Averages (1, 5, 15 Minuten)
static void read_load_avg(void) {
    struct file *file;
    char buffer[128];
    loff_t pos = 0;

    file = filp_open("/proc/loadavg", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /proc/loadavg\n");
        return;
    }

    kernel_read(file, buffer, sizeof(buffer) - 1, &pos);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-Terminierung sicherstellen
    filp_close(file, NULL);

    if (sscanf(buffer, "%ld %ld %ld", &load_1min, &load_5min, &load_15min) != 3) {
        pr_err("Failed to parse /proc/loadavg\n");
    }
}

// Funktion zum Lesen der Speicherstatistiken
static void read_memory_stats(void) {
    struct file *file;
    char buffer[256];
    loff_t pos = 0;

    file = filp_open("/proc/meminfo", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /proc/meminfo\n");
        return;
    }

    kernel_read(file, buffer, sizeof(buffer) - 1, &pos);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-Terminierung sicherstellen
    filp_close(file, NULL);

    if (sscanf(buffer, "MemTotal: %ld kB\nMemFree: %ld kB", &mem_total, &mem_free) != 2) {
        pr_err("Failed to parse /proc/meminfo for memory\n");
    }
}

// Funktion zum Lesen der Swap-Statistiken
static void read_swap_stats(void) {
    struct file *file;
    char buffer[256];
    loff_t pos = 0;

    file = filp_open("/proc/meminfo", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /proc/meminfo\n");
        return;
    }

    kernel_read(file, buffer, sizeof(buffer) - 1, &pos);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-Terminierung sicherstellen
    filp_close(file, NULL);

    if (sscanf(buffer, "%*[^S]SwapTotal: %ld kB\nSwapFree: %ld kB", &swap_total, &swap_free) != 2) {
        pr_err("Failed to parse swap stats\n");
    }
}

// Funktion zum Lesen der Disk-Statistiken
static void read_disk_stats(void) {
    disk_read = 1000; // Beispielwert
    disk_write = 2000; // Beispielwert
}

// Funktion zum Lesen der Netzwerk-Statistiken
static void read_network_stats(void) {
    struct file *file;
    char buffer[256];
    loff_t pos = 0;

    file = filp_open("/proc/net/dev", O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open /proc/net/dev\n");
        return;
    }

    kernel_read(file, buffer, sizeof(buffer) - 1, &pos);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-Terminierung sicherstellen
    filp_close(file, NULL);

    if (sscanf(buffer, "%*[^e]eth0: %ld %*[^0-9] %ld", &network_rx, &network_tx) != 2) {
        pr_err("Failed to parse /proc/net/dev for network stats\n");
    }
}

// Funktion, die die Daten im /proc-Dateisystem anzeigt
static int read_proc(struct seq_file *m, void *v) {
    read_cpu_usage();
    read_memory_stats();
    read_swap_stats();
    read_disk_stats();
    read_network_stats();
    read_load_avg();
    read_cpu_freq();
    read_cpu_temp();

    seq_printf(m, "CPU User: %ld\n", cpu_user);
    seq_printf(m, "CPU System: %ld\n", cpu_system);
    seq_printf(m, "CPU Idle: %ld\n", cpu_idle);
    seq_printf(m, "Memory Total: %ld kB\n", mem_total);
    seq_printf(m, "Memory Free: %ld kB\n", mem_free);
    seq_printf(m, "Swap Total: %ld kB\n", swap_total);
    seq_printf(m, "Swap Free: %ld kB\n", swap_free);
    seq_printf(m, "Disk Read: %ld\n", disk_read);
    seq_printf(m, "Disk Write: %ld\n", disk_write);
    seq_printf(m, "Network RX: %ld\n", network_rx);
    seq_printf(m, "Network TX: %ld\n", network_tx);
    seq_printf(m, "Load 1min: %ld\n", load_1min);  // Verwendet jetzt Ganzzahlen
    seq_printf(m, "Load 5min: %ld\n", load_5min);  // Verwendet jetzt Ganzzahlen
    seq_printf(m, "Load 15min: %ld\n", load_15min);  // Verwendet jetzt Ganzzahlen
    seq_printf(m, "CPU Frequency: %u kHz\n", cpu_freq);
    seq_printf(m, "CPU Temperature: %ld°C\n", cpu_temp);

    return 0;
}

// Funktion für das Öffnen des /proc-Dateisystems
static int my_module_open(struct inode *inode, struct file *file) {
    return single_open(file, read_proc, NULL);
}

// Funktion für das Schließen des /proc-Dateisystems
static int my_module_release(struct inode *inode, struct file *file) {
    return single_release(inode, file);
}

// Definieren der proc_ops Struktur
static const struct proc_ops proc_fops = {
    .proc_open = my_module_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = my_module_release,
};

// Initialisierung des /proc-Eintrags
static int __init read_system_data_init(void) {
    proc_create(PROC_FILENAME, 0, NULL, &proc_fops);
    return 0;
}

static void __exit read_system_data_exit(void) {
    remove_proc_entry(PROC_FILENAME, NULL);
}

module_init(read_system_data_init);
module_exit(read_system_data_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dein Name");
MODULE_DESCRIPTION("Ein Modul, das Systemdaten über /proc/read_system_data bereitstellt");
