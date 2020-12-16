#define LOG_TAG "memcheck"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
//#include <sys/stat.h>
#include <dirent.h>
#include <utils/Log.h>

enum {
    MEMCHECK_MEMINFO = 0,
    MEMCHECK_ZONEINFO,
    MEMCHECK_SLABINFO,
    MEMCHECK_GPUMEMINFO,
    MEMCHECK_GPUVIDMEM,
    MEMCHECK_CMAUSED,
    MEMCHECK_NUMBER
};

struct discrete_buffer {
    struct discrete_buffer *next;
    struct discrete_buffer *prev;

    size_t size;
    size_t used;
    char data[0];
};

#define DISCRETE_BUFFER_SIZE    2000
#define SHOW_LINE_SIZE          256

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

struct memcheck_entry {
    const char *path;
    int fd;
    int err;

   struct discrete_buffer head;
};

struct process_status {
    struct process_status *next;
    int pid;
    char path[32];
    char name[32];
    char nothing;
    unsigned long rss;
    unsigned long rss_anon;
    unsigned long rss_file;
    unsigned long rss_shmem;
    struct memcheck_entry entry;
};

static bool use_logcat = false;

static struct memcheck_entry entry[MEMCHECK_NUMBER] = {
    [MEMCHECK_MEMINFO] = {
        .path   = "/proc/meminfo",
        .fd     = -1,
        .err    = 0,
    },

    [MEMCHECK_ZONEINFO] = {
        .path   = "/proc/zoneinfo",
        .fd     = -1,
        .err    = 0,
    },

    [MEMCHECK_SLABINFO] = {
        .path   =  "/proc/slabinfo",
        .fd     = -1,
        .err    = 0,
    },

    [MEMCHECK_GPUMEMINFO] = {
        .path   = "/d/gc/meminfo",
        .fd     = -1,
        .err    = 0,
    },

    [MEMCHECK_GPUVIDMEM] = {
        .path   = "/d/gc/vidmem",
        .fd     = -1,
        .err    = 0,
    },

    [MEMCHECK_CMAUSED] = {
        .path   = "/d/cma/cma-reserved/used",
        .fd     = -1,
        .err    = 0,
    },
};

static bool memcheck_enable[MEMCHECK_NUMBER] = {
    [MEMCHECK_MEMINFO]    = false,
    [MEMCHECK_ZONEINFO]   = false,
    [MEMCHECK_SLABINFO]   = false,
    [MEMCHECK_GPUMEMINFO] = false,
    [MEMCHECK_GPUVIDMEM]  = false,
    [MEMCHECK_CMAUSED]    = false
};

#define MLOGE(...)  do  \
    { \
        if (use_logcat) \
            ALOGE(__VA_ARGS__); \
        else { \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
        } \
    } while(0)

#define MLOGW(...)  do  \
    { \
        if (use_logcat) \
            ALOGW(__VA_ARGS__); \
        else { \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
        } \
    } while(0)

#define MLOGV(...)  do  \
    { \
        if (use_logcat) \
            ALOGV(__VA_ARGS__); \
        else { \
            fprintf(stdout, __VA_ARGS__); \
            fprintf(stdout, "\n"); \
        } \
    } while(0)
#define MLOGI(...)  do  \
    { \
        if (use_logcat) \
            ALOGI(__VA_ARGS__); \
        else { \
            fprintf(stdout, __VA_ARGS__); \
            fprintf(stdout, "\n"); \
        } \
    } while(0)
#define MLOGD(...)  do  \
    { \
        if (use_logcat) \
            ALOGD(__VA_ARGS__); \
        else { \
            fprintf(stdout, __VA_ARGS__); \
            fprintf(stdout, "\n"); \
        } \
    } while(0)

static struct discrete_buffer *discrete_buffer_new(void)
{
    struct discrete_buffer *b;

    b = malloc(DISCRETE_BUFFER_SIZE);

    if (b) {
        b->size = DISCRETE_BUFFER_SIZE - sizeof(*b);
        b->used = 0;
        b->next = NULL;
    }

    return b;
}

static void discrete_buffer_insert(struct discrete_buffer *head,
                                   struct discrete_buffer *b)
{
    struct discrete_buffer *last = head->prev;

    b->next = head;
    b->prev = last;

    last->next = b;
    head->prev = b;
}

#define for_each_discrete_buffer(_pos, _head) \
    for (_pos = (_head)->next; \
         _pos != _head; \
         _pos = _pos->next)

#define for_each_discrete_buffer_safe(_pos, _tmp, _head) \
    for (_pos = (_head)->next, \
         _tmp = (_pos)->next; \
         _pos != _head; \
         _pos = _tmp, \
         _tmp = _tmp->next)

#define empty_discrete_buffer(_head) ((_head)->next == (_head))

#define first_discrete_buffer(_head) (empty_discrete_buffer(_head) ? NULL : (_head)->next)

static void discrete_buffer_delete(struct discrete_buffer *b)
{
    b->next->prev = b->prev;
    b->prev->next = b->next;
}

static void memcheck_entry_init(struct memcheck_entry *entry)
{
    //struct stat buf;
    int rc;

    entry->head.next = &entry->head;
    entry->head.prev = &entry->head;
#if 0
    rc = stat(entry->path, &buf);

    if (rc == -1) {
        MLOGE("stat %s %s", entry->path, strerror(errno));
        entry->err = errno;
        return;
    }

    if(!S_ISREG(buf.st_mode)) {
        MLOGE("%s isn't a reg", entry->path);
        entry->err = ENOENT;
        return;
    }
#endif
    rc = open(entry->path, O_RDONLY);

    if (rc < 0) {
        MLOGE("open %s %s", entry->path, strerror(errno));
        entry->err = errno;
        return;
    }

    entry->fd = rc;
}

static void memcheck_entry_fini(struct memcheck_entry *entry)
{
    struct discrete_buffer *b, *tmp;
    if (entry->err)
        return;

    if (entry->fd >= 0) {
        close(entry->fd);
        entry->fd = -1;
    }

    for_each_discrete_buffer_safe(b, tmp, &entry->head) {
        discrete_buffer_delete(b);
        free(b);
    }
}

static void memcheck_entry_grab(struct memcheck_entry *entry)
{
    if (entry->err)
        return;

    for (;;) {
        int rc;
        struct discrete_buffer *b = discrete_buffer_new();

        if (!b) {
            entry->err = ENOMEM;
            MLOGE("alloc buffer %s %s", entry->path, strerror(entry->err));
            break;
        }

        rc = read(entry->fd, b->data, b->size);

        if (rc <= 0) {
            if (rc < 0)
                MLOGE("read %s %s rc %d", entry->path, strerror(errno), rc);

            free(b);
            break;
        }

        b->used = rc;

        discrete_buffer_insert(&entry->head, b);
        if (rc < (int)b->size)
            break;
    }
}

static void memcheck_entry_show(struct memcheck_entry *entry)
{
    struct discrete_buffer *b;
    char line[SHOW_LINE_SIZE + 1];
    long offset = 0;
    bool discard_tail = false;
    bool no_line_break = false;
    char *lstart, *lend, *lguard;

    for_each_discrete_buffer(b, &entry->head) {
        lstart = lend = b->data;
        lguard = b->data + b->used;

        /* for every line */
        while (lend < lguard) {
            size_t copy;

            for(; *lend && *lend != '\n' && lend < lguard; lend++);

            no_line_break = (lend == lguard);

            if (discard_tail) {
                if (!no_line_break)
                    discard_tail = false;

                lstart = ++ lend;
                continue;
            }

            copy = min(SHOW_LINE_SIZE - offset, lend - lstart);
            strncpy(line + offset, lstart, copy);

            line[offset + copy] = 0;

            discard_tail = (offset + copy == SHOW_LINE_SIZE && no_line_break);

            lstart = ++ lend;
            offset += copy;

            if (!no_line_break || discard_tail) {
                offset = 0;
                MLOGI("%s", line);
            }
        }
    }

    MLOGI("\n");
}

static struct process_status* process_status_new(int pid)
{
    struct process_status *ps = malloc(sizeof(*ps));

    if (ps) {
        memset(ps, 0, sizeof(*ps));
        ps->next = NULL;
        ps->pid  = pid;
        ps->rss  = 0;
        sprintf(ps->path, "/proc/%d/status", pid);
        ps->entry.fd = -1;
        ps->entry.path = ps->path;
        memcheck_entry_init(&ps->entry);

        if (ps->entry.err) {
            memcheck_entry_fini(&ps->entry);
            free(ps);
            ps = NULL;
        }
    }

    return ps;
}

static void process_status_name(struct process_status *ps)
{
    int fd, rc;
    char cmdline[32];

    rc = snprintf(cmdline, sizeof(cmdline), "/proc/%u/cmdline", ps->pid);
    cmdline[rc] = 0;

    fd = open(cmdline, O_RDONLY);

    if (fd < 0)
        return;

    read(fd, ps->name, sizeof(ps->name));
    close(fd);
}

static unsigned long process_status_parse(struct process_status *ps, const char *pattern)
{
    char line[SHOW_LINE_SIZE + 1];
    char *lstart, *lend, *lguard;
    struct discrete_buffer *b = first_discrete_buffer(&ps->entry.head);
    size_t pattern_len = strlen(pattern);
    size_t copy;
    bool found = false;
    unsigned long value = 0;

    if (!b)
        return 0;

    lstart = b->data;
    lguard = b->data + b->used;

    for (; lstart + pattern_len < lguard; lstart ++) {
        if (!strncmp(lstart, pattern, pattern_len)) {
            found = true;
            break;
        }
    }

    if (!found)
        return 0;

    found = false;
    for (lend = lstart; lend < lguard; lend ++) {
        if (*lend == '\n') {
            found = true;
            break;
        }
    }

    if (!found)
        return 0;

    copy = lend - lstart;
    copy = min(copy, SHOW_LINE_SIZE);
    memcpy(line, lstart, copy);
    line[copy] = 0;

    if (1 == sscanf(line + pattern_len, "\t%8lu kB", &value))
        return value;

    return 0;
}

static void process_status_sort(struct process_status **head, bool descend)
{
    struct process_status *sort_head = NULL;
    struct process_status *unsort_head = *head;
    struct process_status *ps, *candidate;

    if (!head)
        return;

    while (unsort_head) {
        /* first. find the candidate */
        for (ps = candidate = unsort_head; ps; ps = ps->next) {
            if (descend && ps->rss < candidate->rss)
                candidate = ps;
            else if (!descend && ps->rss > candidate->rss)
                candidate = ps;
        }

        /* second, delete the candidate from head */
        if (unsort_head == candidate)
            unsort_head = candidate->next;
        else {
            for (ps = unsort_head; ps->next != candidate; ps = ps->next);
            ps->next = candidate->next;
        }

        /* third, link candidate into sort_head */
        candidate->next = sort_head;
        sort_head = candidate;
    }

    *head = sort_head;
}

static void memcheck_ps_show(struct process_status *ps)
{
    if (!ps->rss)
        return;

    MLOGI("Name:%40s\t"
          "Pid:%6u\t"
          "Rss:%8lu kB\t"
          "RssAnon:%8lu kB\t"
          "RssShmem:%8lu kB\t"
          "RssFile:%8lu kB",
          ps->name[0]?ps->name:"null",
          ps->pid,
          ps->rss,
          ps->rss_anon,
          ps->rss_shmem,
          ps->rss_file);
}

static void memcheck_system(void)
{
    int i;

    for (i = 0; i < MEMCHECK_NUMBER; i ++) {
        if (!memcheck_enable[i])
            continue;

        memcheck_entry_init(&entry[i]);
        memcheck_entry_grab(&entry[i]);
        memcheck_entry_show(&entry[i]);
        memcheck_entry_fini(&entry[i]);
    }
}

/* A simple function to tranfer string to digit */
static bool simple_str2int(const char *str, int *result)
{
    int digit = 0;
    bool rc = true;
    const char *p;

    for (p = str; p && *p; p ++) {
        if (isdigit(*p))
            digit = digit * 10 + *p - '0';
        else {
            rc = false;
            break;
        }
    }

    if (rc)
        *result = digit;

    return rc;
}

static void memcheck_processes(void)
{
    const char *proc = "/proc/";
    DIR *dir;
    struct dirent *de;
    struct process_status *ps_head = NULL;
    struct process_status *ps;
    unsigned long all_rss = 0;
    unsigned long all_rss_anon = 0;

    dir = opendir(proc);

    if (!dir) {
        MLOGE("open %s %s", proc, strerror(errno));
        return;
    }

    while ((de = readdir(dir)) != NULL) {
        int pid;

        if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        if(de->d_type != DT_DIR || !simple_str2int(de->d_name, &pid))
            continue;

        ps = process_status_new(pid);

        if (!ps)
            continue;

        ps->next = ps_head;
        ps_head  = ps;
    }

    closedir(dir);

    for (ps = ps_head; ps; ps = ps->next) {
        memcheck_entry_grab(&ps->entry);
        process_status_name(ps);
        ps->rss       = process_status_parse(ps, "VmRSS:");
        ps->rss_anon  = process_status_parse(ps, "RssAnon:");
        ps->rss_file  = process_status_parse(ps, "RssFile:");
        ps->rss_shmem = process_status_parse(ps, "RssShmem:");

        all_rss += ps->rss;
        all_rss_anon += ps->rss_anon;
    }

    process_status_sort(&ps_head, true);

    for (ps = ps_head; ps; ps = ps->next) {
        memcheck_ps_show(ps);
        memcheck_entry_fini(&ps->entry);
    }

    MLOGI("\n");
    MLOGI("All Rss:\t%8lu kB\t\tAll Rss Anon:\t%8lu kB", all_rss, all_rss_anon);
    MLOGI("\n");

    ps = ps_head;

    while (ps) {
        struct process_status *to_free;
        to_free = ps;
        ps = ps->next;
        free(to_free);
    }
}

static void usage(const char *name)
{
    fprintf(stderr, "usage:  %s [-m|-z|-s|-g|-v|-c|-p] [-l] [-h]\n\n"
         "Check the memory used infomation.\n"
         "If -m is specified, show meminfo.\n"
         "If -z is specified, show zoneinfo.\n"
         "If -s is specified, show slabinfo.\n"
         "If -g is specified, show gpu memory pre-allocation.\n"
         "If -v is specified, show gpu memory used information for each process.\n"
         "If -c is specified, show cma used information.\n"
         "If -p is specified, show memory used for each process.\n\n"

         "If -l is specified, %s output information to android logd.\n"
         "Otherwise, output information to terminal\n\n"

         "If -h is specified, output this.\n\n",
         name, name);
}

int main(int argc, char **argv)
{
    int c;
    bool enable_sys = false;
    bool enable_ps  = false;

    while ((c = getopt(argc, argv, "mzsgvcplh")) != -1) {
        switch (c) {
        case 'm':
            enable_sys = true;
            memcheck_enable[MEMCHECK_MEMINFO] = true;
            break;

        case 'z':
            enable_sys = true;
            memcheck_enable[MEMCHECK_ZONEINFO] = true;
            break;

        case 's':
            enable_sys = true;
            memcheck_enable[MEMCHECK_SLABINFO] = true;
            break;

        case 'g':
            enable_sys = true;
            memcheck_enable[MEMCHECK_GPUMEMINFO] = true;
            break;

        case 'v':
            enable_sys = true;
            memcheck_enable[MEMCHECK_GPUVIDMEM] = true;
            break;

        case 'c':
            enable_sys = true;
            memcheck_enable[MEMCHECK_CMAUSED] = true;
            break;

        case 'p':
            enable_ps = true;
            break;

        case 'l':
            use_logcat = true;
            break;

        case 'h':
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if (!enable_sys && !enable_ps) {
        memcheck_enable[MEMCHECK_MEMINFO]  = true;
        memcheck_enable[MEMCHECK_ZONEINFO] = true;
    }

    memcheck_system();

    if (enable_ps)
        memcheck_processes();
}
