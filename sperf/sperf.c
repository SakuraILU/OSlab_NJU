#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <assert.h>
#include <stdbool.h>

// int main(int argc, char *argv[])
// {
//   char *exec_argv[] = {
//       "strace",
//       "ls",
//       NULL,
//   };
//   char *exec_envp[] = {
//       "PATH=/bin",
//       NULL,
//   };
//   execve("strace", exec_argv, exec_envp);
//   perror(argv[0]);
//   exit(EXIT_FAILURE);
// }

#define SYSNAME_MSIZE 64
#define SYSTIME_MSIZE 32

extern char **environ;

typedef struct sysinfo
{
  char name[SYSNAME_MSIZE];
  float total_time;
  struct sysinfo *next;
} Sysinfo;

Sysinfo *head, *tail;

static __attribute__((constructor)) void init()
{
  tail = head = (Sysinfo *)malloc(sizeof(Sysinfo));
  memset(head, 0, sizeof(Sysinfo));
}

void add_sysinfo(char *sys_name, float sys_time)
{
  // printf("add sys %s time %f \n", sys_name, sys_time);
  Sysinfo *itr = head->next;
  while (itr != NULL)
  {
    if (strcmp(itr->name, sys_name) == 0)
    {
      itr->total_time += sys_time;
      return;
    }
    itr = itr->next;
  }

  tail->next = (Sysinfo *)malloc(sizeof(Sysinfo));
  tail = tail->next;
  strcpy(tail->name, sys_name);
  tail->total_time = sys_time;
  tail->next = NULL;
}

static void child(int argc, char *exec_argv[]);
static void parent();
void parse_sysinfo();

int fd[2];

int main(int argc, char *argv[])
{
  pipe(fd);

  int ret = fork();
  if (ret == 0)
  {
    close(fd[0]);

    dup2(fd[1], STDERR_FILENO);
    close(fd[1]);

    // close(STDOUT_FILENO);
    // fopen("/dev/null", "w");

    child(argc, argv);
  }
  else if (ret > 0)
  {
    close(fd[1]);
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);

    parent();
  }
  else
  {
    close(fd[0]);
    close(fd[1]);
    perror("fork");
    assert(0);
  }
}

static void child(int argc, char *exec_argv[])
{
  char *argv[2 + argc + 1];
  argv[0] = "strace";
  argv[1] = "--syscall-time";
  for (int i = 1; i < argc; ++i)
  {
    argv[i + 1] = exec_argv[i];
  }
  // argv[argc + 2] = NULL;
  printf("%s\n", argv[argc + 2]);

  char *envp[] = {
      "PATH=/:/bin:/usr/bin:/home/sakura/Code/Language/Python/Miniconda/bin/",
      NULL,
  };

  execve("/usr/bin/strace", argv, envp);

  perror(argv[0]);
  exit(EXIT_FAILURE);
}

static void parent()
{
  parse_sysinfo();
}

void parse_sysinfo()
{
  regex_t name_reg, time_reg;                 // 定义一个正则实例
  const char *name_pat = "^[a-z_][a-z0-9_]*"; // 定义模式串
  regcomp(&name_reg, name_pat, REG_EXTENDED); // 编译正则模式串
  const char *time_pat = "[0-9]+\\.[0-9]+>";  // 定义模式串
  regcomp(&time_reg, time_pat, REG_EXTENDED); // 编译正则模式串

  char *sysinfo = NULL;
  size_t len = 0;
  while (getline(&sysinfo, &len, stdin) != -1)
  {

    char sysname[SYSNAME_MSIZE] = {0};
    char systime_str[SYSTIME_MSIZE] = {0};
    float systime = 0;

    const size_t nmatch = 1; // 定义匹配结果最大允许数
    regmatch_t pmatch[1];    // 定义匹配结果在待匹配串中的下标范围

    // printf("%s", sysinfo);
    int status = regexec(&name_reg, sysinfo, nmatch, pmatch, 0); // 匹配他
    if (status == REG_NOMATCH)
      // 如果没匹配上
      continue;
    else if (status == 0)
      // 如果匹配上了
      strncpy(sysname, sysinfo + pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so);

    int offset = pmatch[0].rm_eo;
    status = regexec(&time_reg, sysinfo + offset, nmatch, pmatch, 0); // 匹配他
    if (status == 0)
      // 如果匹配上了
      systime = atof(strncpy(systime_str, sysinfo + offset + pmatch[0].rm_so + 1, pmatch[0].rm_eo - pmatch[0].rm_so - 2));
    else
    {
      while (getline(&sysinfo, &len, stdin) != -1)
      {
        status = regexec(&time_reg, sysinfo, nmatch, pmatch, 0); // 匹配他
        if (status == REG_NOMATCH)
          continue;

        else if (status == 0)
        { // 如果匹配上了
          systime = atof(strncpy(systime_str, sysinfo + pmatch[0].rm_so + 1, pmatch[0].rm_eo - pmatch[0].rm_so - 2));
          break;
        }
        else
          assert(false);
      }
    }

    add_sysinfo(sysname, systime);
  }
}

void sort_sysinfo()
{
}