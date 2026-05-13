/**
 * @FilePath     : main.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-27 14:25:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-27 14:39:05
 * @Description  :
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

/* 要监测的程序名配置 */
static const char *MONITOR_PROGRAMS[] = {
    "operation",
    "record",
    "stream",
    "upgrade",
    NULL /* 结束标记 */
};

/* 进程信息结构体 */
typedef struct
{
    int           pid;
    char          name[256];
    char          exe_name[256];   /* 实际执行文件名 */
    int           is_running;      /* 是否正在运行 */
    unsigned long vm_rss;          /* 物理内存占用 (kB) */
    unsigned long vm_size;         /* 虚拟内存占用 (kB) */
    double        cpu_usage;       /* CPU占用率 (%) */
    unsigned long utime;           /* 用户态时间 */
    unsigned long stime;           /* 内核态时间 */
    unsigned long last_total_time; /* 上次总时间 */
} ProcessInfo;

/* 网络接口信息结构体 */
typedef struct
{
    char          interface[32];
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    unsigned long rx_packets;
    unsigned long tx_packets;
    unsigned long last_rx_bytes;
    unsigned long last_tx_bytes;
    double        rx_rate; /* 接收速率 bytes/s */
    double        tx_rate; /* 发送速率 bytes/s */
} NetworkInfo;

/* 系统CPU信息结构体 */
typedef struct
{
    unsigned long user, nice, system, idle, iowait, irq, softirq;
    unsigned long total;
    unsigned long last_total;
    unsigned long last_idle;
    double        cpu_usage;
} SystemCPU;

/* 从/proc/pid/status读取内存信息 */
int read_process_memory(int pid, ProcessInfo *proc)
{
    char  path[256];
    FILE *file;
    char  line[256];

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (!file)
        return -1;

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "Name:", 5) == 0)
        {
            sscanf(line, "Name:\t%s", proc->name);
        }
        else if (strncmp(line, "VmRSS:", 6) == 0)
        {
            sscanf(line, "VmRSS:\t%lu kB", &proc->vm_rss);
        }
        else if (strncmp(line, "VmSize:", 7) == 0)
        {
            sscanf(line, "VmSize:\t%lu kB", &proc->vm_size);
        }
    }

    fclose(file);
    return 0;
}

/* 从/proc/pid/stat读取CPU时间信息 */
int read_process_cpu(int pid, ProcessInfo *proc)
{
    char          path[256];
    FILE         *file;
    char          comm[256];
    char          state;
    int           ppid, pgrp, session, tty_nr, tpgid;
    unsigned      flags;
    unsigned long minflt, cminflt, majflt, cmajflt;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (!file)
        return -1;

    /* 读取stat文件的前14个字段，获取utime和stime */
    fscanf(file,
           "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu",
           &proc->pid,
           comm,
           &state,
           &ppid,
           &pgrp,
           &session,
           &tty_nr,
           &tpgid,
           &flags,
           &minflt,
           &cminflt,
           &majflt,
           &cmajflt,
           &proc->utime,
           &proc->stime);

    fclose(file);
    return 0;
}

/* 计算进程CPU占用率 */
void calculate_process_cpu(ProcessInfo  *proc,
                           unsigned long system_total_time,
                           unsigned long last_system_total,
                           double        interval)
{
    unsigned long total_time = proc->utime + proc->stime;
    unsigned long delta_time = total_time - proc->last_total_time;
    unsigned long delta_system = system_total_time - last_system_total;

    if (delta_system > 0)
    {
        proc->cpu_usage = (double) delta_time * 100.0 / delta_system;
    }
    else
    {
        proc->cpu_usage = 0.0;
    }

    proc->last_total_time = total_time;
}

/* 读取系统CPU信息 */
int read_system_cpu(SystemCPU *cpu)
{
    FILE *file = fopen("/proc/stat", "r");
    if (!file)
        return -1;

    fscanf(file,
           "cpu %lu %lu %lu %lu %lu %lu %lu",
           &cpu->user,
           &cpu->nice,
           &cpu->system,
           &cpu->idle,
           &cpu->iowait,
           &cpu->irq,
           &cpu->softirq);

    cpu->total = cpu->user + cpu->nice + cpu->system + cpu->idle + cpu->iowait + cpu->irq + cpu->softirq;

    fclose(file);
    return 0;
}

/* 计算系统CPU占用率 */
void calculate_system_cpu(SystemCPU *cpu)
{
    unsigned long total_diff = cpu->total - cpu->last_total;
    unsigned long idle_diff = cpu->idle - cpu->last_idle;

    if (total_diff > 0)
    {
        cpu->cpu_usage = (double) (total_diff - idle_diff) * 100.0 / total_diff;
    }
    else
    {
        cpu->cpu_usage = 0.0;
    }

    cpu->last_total = cpu->total;
    cpu->last_idle = cpu->idle;
}

/* 读取网络接口信息 */
int read_network_stats(NetworkInfo *networks, int max_interfaces)
{
    FILE *file = fopen("/proc/net/dev", "r");
    if (!file)
        return -1;

    char line[512];
    int  count = 0;

    /* 跳过前两行标题 */
    fgets(line, sizeof(line), file);
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) && count < max_interfaces)
    {
        char *colon = strchr(line, ':');
        if (!colon)
            continue;

        *colon = '\0';
        char *interface = line;
        char *stats = colon + 1;

        /* 去除接口名前后空格 */
        while (*interface == ' ' || *interface == '\t')
            interface++;

        unsigned long rx_bytes, rx_packets, tx_bytes, tx_packets;
        unsigned long dummy[12]; /* 其他不需要的统计项 */

        if (sscanf(stats,
                   "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &rx_bytes,
                   &rx_packets,
                   &dummy[0],
                   &dummy[1],
                   &dummy[2],
                   &dummy[3],
                   &dummy[4],
                   &dummy[5],
                   &tx_bytes,
                   &tx_packets,
                   &dummy[6],
                   &dummy[7],
                   &dummy[8],
                   &dummy[9],
                   &dummy[10],
                   &dummy[11]) == 16)
        {

            strncpy(networks[count].interface, interface, sizeof(networks[count].interface) - 1);
            networks[count].rx_bytes = rx_bytes;
            networks[count].tx_bytes = tx_bytes;
            networks[count].rx_packets = rx_packets;
            networks[count].tx_packets = tx_packets;
            count++;
        }
    }

    fclose(file);
    return count;
}

/* 计算网络速率 */
void calculate_network_rate(NetworkInfo *networks, int count, double interval)
{
    for (int i = 0; i < count; i++)
    {
        if (interval > 0)
        {
            networks[i].rx_rate = (double) (networks[i].rx_bytes - networks[i].last_rx_bytes) / interval;
            networks[i].tx_rate = (double) (networks[i].tx_bytes - networks[i].last_tx_bytes) / interval;
        }
        else
        {
            networks[i].rx_rate = 0.0;
            networks[i].tx_rate = 0.0;
        }

        networks[i].last_rx_bytes = networks[i].rx_bytes;
        networks[i].last_tx_bytes = networks[i].tx_bytes;
    }
}

/* 根据进程名查找所有匹配的PID */
int find_pids_by_name(const char *process_name, int *pids, int max_pids)
{
    DIR           *proc_dir;
    struct dirent *entry;
    FILE          *status_file;
    char           path[512];
    char           line[256];
    char           name[256];
    int            pid;
    int            count = 0;

    proc_dir = opendir("/proc");
    if (proc_dir == NULL)
    {
        perror("无法打开 /proc 目录");
        return 0;
    }

    while ((entry = readdir(proc_dir)) != NULL && count < max_pids)
    {
        /* 检查是否是数字目录（PID） */
        pid = atoi(entry->d_name);
        if (pid <= 0)
            continue;

        /* 读取进程名 */
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        status_file = fopen(path, "r");
        if (status_file == NULL)
            continue;

        if (fgets(line, sizeof(line), status_file))
        {
            if (sscanf(line, "Name:\t%s", name) == 1)
            {
                /* 检查进程名是否匹配 */
                if (strcmp(name, process_name) == 0)
                {
                    pids[count++] = pid;
                }
            }
        }

        fclose(status_file);
    }

    closedir(proc_dir);
    return count;
}

/* 初始化要监控的进程列表 */
int initialize_processes(ProcessInfo *processes, int max_processes)
{
    int total_count = 0;

    /* 遍历配置的程序名 */
    for (int i = 0; MONITOR_PROGRAMS[i] != NULL && total_count < max_processes; i++)
    {
        int pids[32]; /* 每个程序名最多找32个实例 */
        int pid_count = find_pids_by_name(MONITOR_PROGRAMS[i], pids, 32);

        printf("查找程序 '%s': 找到 %d 个实例\n", MONITOR_PROGRAMS[i], pid_count);

        /* 为每个找到的PID创建ProcessInfo */
        for (int j = 0; j < pid_count && total_count < max_processes; j++)
        {
            processes[total_count].pid = pids[j];
            strncpy(processes[total_count].exe_name, MONITOR_PROGRAMS[i], sizeof(processes[total_count].exe_name) - 1);
            processes[total_count].is_running = 1;
            processes[total_count].last_total_time = 0;

            /* 读取初始信息 */
            if (read_process_memory(pids[j], &processes[total_count]) == 0 &&
                read_process_cpu(pids[j], &processes[total_count]) == 0)
            {
                printf("  PID %d: %s\n", pids[j], processes[total_count].name);
                total_count++;
            }
        }
    }

    return total_count;
}

/* 更新进程列表，检查新进程和已退出进程 */
int update_process_list(ProcessInfo *processes, int current_count, int max_processes)
{
    /* 标记所有进程为未找到 */
    for (int i = 0; i < current_count; i++)
    {
        processes[i].is_running = 0;
    }

    /* 重新扫描所有配置的程序 */
    for (int i = 0; MONITOR_PROGRAMS[i] != NULL; i++)
    {
        int pids[32];
        int pid_count = find_pids_by_name(MONITOR_PROGRAMS[i], pids, 32);

        for (int j = 0; j < pid_count; j++)
        {
            int found = 0;
            /* 检查是否已存在于列表中 */
            for (int k = 0; k < current_count; k++)
            {
                if (processes[k].pid == pids[j])
                {
                    processes[k].is_running = 1;
                    found = 1;
                    break;
                }
            }

            /* 如果是新进程且还有空间，添加到列表 */
            if (!found && current_count < max_processes)
            {
                processes[current_count].pid = pids[j];
                strncpy(processes[current_count].exe_name,
                        MONITOR_PROGRAMS[i],
                        sizeof(processes[current_count].exe_name) - 1);
                processes[current_count].is_running = 1;
                processes[current_count].last_total_time = 0;

                if (read_process_memory(pids[j], &processes[current_count]) == 0 &&
                    read_process_cpu(pids[j], &processes[current_count]) == 0)
                {
                    printf("发现新进程: PID %d (%s)\n", pids[j], MONITOR_PROGRAMS[i]);
                    current_count++;
                }
            }
        }
    }

    return current_count;
}

/* 格式化字节数显示 */
void format_bytes(double bytes, char *output, int size)
{
    const char *units[] = { "B", "KB", "MB", "GB" };
    int         unit = 0;

    while (bytes >= 1024.0 && unit < 3)
    {
        bytes /= 1024.0;
        unit++;
    }

    snprintf(output, size, "%.2f%s", bytes, units[unit]);
}

/* 显示帮助信息 */
void show_help(const char *program_name)
{
    printf("系统监控程序\n");
    printf("使用方法: %s [选项]\n", program_name);
    printf("选项:\n");
    printf("  -i <间隔>    监控间隔秒数 (默认: 2)\n");
    printf("  -c <次数>    监控次数 (默认: 无限)\n");
    printf("  -h           显示此帮助信息\n");
    printf("\n");
    printf("预配置监控的程序:\n");
    for (int i = 0; MONITOR_PROGRAMS[i] != NULL; i++)
    {
        printf("  - %s\n", MONITOR_PROGRAMS[i]);
    }
    printf("\n");
    printf("示例:\n");
    printf("  %s            使用默认设置监控\n", program_name);
    printf("  %s -i 5       每5秒监控一次\n", program_name);
    printf("  %s -c 10      监控10次后退出\n", program_name);
}

int main(int argc, char *argv[])
{
    int interval = 2;  /* 默认监控间隔2秒 */
    int max_count = 0; /* 0表示无限循环 */
    int count = 0;
    int process_count = 0;

    /* 解析命令行参数 */
    int opt;
    while ((opt = getopt(argc, argv, "i:c:h")) != -1)
    {
        switch (opt)
        {
        case 'i':
            interval = atoi(optarg);
            if (interval <= 0)
            {
                fprintf(stderr, "错误: 监控间隔必须大于0\n");
                return 1;
            }
            break;
        case 'c':
            max_count = atoi(optarg);
            if (max_count <= 0)
            {
                fprintf(stderr, "错误: 监控次数必须大于0\n");
                return 1;
            }
            break;
        case 'h':
            show_help(argv[0]);
            return 0;
        default:
            show_help(argv[0]);
            return 1;
        }
    }

    ProcessInfo processes[64]; /* 最多监控64个进程实例 */
    NetworkInfo networks[16];
    SystemCPU   system_cpu = { 0 };

    printf("系统监控程序启动 (监控间隔: %d秒)\n", interval);
    printf("配置的监控程序: ");
    for (int i = 0; MONITOR_PROGRAMS[i] != NULL; i++)
    {
        printf("%s ", MONITOR_PROGRAMS[i]);
    }
    printf("\n\n");

    /* 初始化要监控的进程 */
    process_count = initialize_processes(processes, 64);

    if (process_count == 0)
    {
        printf("警告: 没有找到任何配置的程序正在运行\n");
    }

    read_system_cpu(&system_cpu);
    int net_count = read_network_stats(networks, 16);

    /* 等待一个间隔，获取初始数据 */
    sleep(1);

    printf("\n按Ctrl+C退出\n\n");

    while (max_count == 0 || count < max_count)
    {
        time_t now = time(NULL);
        printf("\n=== %s", ctime(&now));

        /* 更新进程列表（检查新进程和已退出进程） */
        process_count = update_process_list(processes, process_count, 64);

        /* 读取系统CPU */
        SystemCPU current_cpu;
        read_system_cpu(&current_cpu);
        calculate_system_cpu(&current_cpu);

        printf("系统CPU使用率: %.2f%%\n", current_cpu.cpu_usage);

        /* 监控进程 */
        printf("\n进程监控:\n");
        printf("%-8s %-16s %-12s %10s %10s %8s\n", "PID", "程序名", "进程名", "物理内存", "虚拟内存", "CPU%");
        printf("----------------------------------------------------------------------\n");

        int running_count = 0;
        for (int i = 0; i < process_count; i++)
        {
            if (!processes[i].is_running)
            {
                printf("%-8d %-13s %-9s %6s %10s %8s\n",
                       processes[i].pid,
                       processes[i].exe_name,
                       "已退出",
                       "-",
                       "-",
                       "-");
                continue;
            }

            ProcessInfo current_proc = processes[i];
            if (read_process_memory(processes[i].pid, &current_proc) != 0 ||
                read_process_cpu(processes[i].pid, &current_proc) != 0)
            {
                processes[i].is_running = 0;
                printf("%-8d %-13s %-9s %4s %10s %8s\n",
                       processes[i].pid,
                       processes[i].exe_name,
                       "已退出",
                       "-",
                       "-",
                       "-");
                continue;
            }

            calculate_process_cpu(&current_proc, current_cpu.total, system_cpu.last_total, interval);

            char rss_str[32], size_str[32];
            format_bytes(current_proc.vm_rss * 1024.0, rss_str, sizeof(rss_str));
            format_bytes(current_proc.vm_size * 1024.0, size_str, sizeof(size_str));

            printf("%-8d %-13s %-9s %4s %10s %7.2f%%\n",
                   current_proc.pid,
                   processes[i].exe_name,
                   current_proc.name,
                   rss_str,
                   size_str,
                   current_proc.cpu_usage);

            processes[i] = current_proc;
            running_count++;
        }

        printf("当前运行进程数: %d/%d\n", running_count, process_count);

        /* 网络监控 */
        printf("\n网络接口监控:\n");
        printf("%-12s %11s %11s %10s %10s\n", "接口", "RX速率", "TX速率", "RX包数", "TX包数");
        printf("------------------------------------------------------------\n");

        NetworkInfo current_networks[16];
        int         current_net_count = read_network_stats(current_networks, 16);

        for (int i = 0; i < current_net_count; i++)
        {
            /* 查找对应的历史数据 */
            for (int j = 0; j < net_count; j++)
            {
                if (strcmp(current_networks[i].interface, networks[j].interface) == 0)
                {
                    current_networks[i].last_rx_bytes = networks[j].rx_bytes;
                    current_networks[i].last_tx_bytes = networks[j].tx_bytes;
                    break;
                }
            }
        }

        calculate_network_rate(current_networks, current_net_count, interval);

        for (int i = 0; i < current_net_count; i++)
        {
            char rx_rate_str[32], tx_rate_str[32];
            format_bytes(current_networks[i].rx_rate, rx_rate_str, sizeof(rx_rate_str));
            format_bytes(current_networks[i].tx_rate, tx_rate_str, sizeof(tx_rate_str));

            printf("%-12s %6s/s %10s/s %10lu %10lu\n",
                   current_networks[i].interface,
                   rx_rate_str,
                   tx_rate_str,
                   current_networks[i].rx_packets,
                   current_networks[i].tx_packets);
        }

        /* 更新历史数据 */
        system_cpu = current_cpu;
        memcpy(networks, current_networks, sizeof(NetworkInfo) * current_net_count);
        net_count = current_net_count;

        count++;

        if (max_count == 0 || count < max_count)
        {
            sleep(interval);
        }
    }

    printf("\n监控结束\n");
    return 0;
}