#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"

typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void pstest_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void new_command(int, char**);
void malloctest_command(int, char **);
void _command(int, char **);
int fibonacci(int);
int stringToInt(char []);

#define StackSize 512
#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function"),
	MKCL(new, "create new tasks"),
	MKCL(malloctest, "test pvPortMalloc() function"),
	MKCL(pstest, "routine to create task and implements semihosting"),
	MKCL(, ""),
};

/*Pointer to the task entry function. Tasks must be implemented 
to never return (i.e. continuous loop) */
void task_func(void *pvParameters){

	while(1);
}

int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

	return count;
}

void ls_command(int n, char *argv[]){
    fio_printf(1,"\r\n"); 

	fio_printf(1,"Hello , this is ls command !!\r\n");

    int dir;
    if(n == 0){
        dir = fs_opendir("");
    }else if(n == 1){
        dir = fs_opendir(argv[1]);
        //if(dir == )
    }else{
        fio_printf(1, "Too many argument!\r\n");
        return;
    }

(void)dir;   // Use dir
}

int filedump(const char *filename){
	char buf[128];

	int fd=fs_open(filename, 0, O_RDONLY);

	if( fd == -2 || fd == -1)
		return fd;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0){
		fio_write(1, buf, count);
    }
	
    fio_printf(1, "\r");

	fio_close(fd);
	return 1;
}

void ps_command(int n, char *argv[]){
	signed char buf[1024];
	vTaskList(buf);
        fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
        fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	

}

/* New a task and implements semihosting. Send ps message to host. */
void pstest_command(int n, char *argv[]){
	signed char buf[1024];
	char psMessage[100];
	int handle, error;

	fio_printf(1, "\r\n");
	xTaskCreate(task_func, 
		(signed portCHAR *) "neal",
		StackSize /*stack size*/, NULL, tskIDLE_PRIORITY + 1, NULL);	// tskIDLE_PRIORITY initial is 0

	vTaskList(buf);
    handle = host_action(SYS_SYSTEM, "mkdir -p output");
	handle = host_action(SYS_SYSTEM, "touch output/syslog");
	handle = host_action(SYS_OPEN, "output/syslog", 8);
	if(handle == -1){
		fio_printf(1, "open file error !!\r\n");
	}

	sprintf(psMessage, (char *) "\n\rCreate %d bytes task!\n\rName	State	Priority	Stack	Num\n\r************************", StackSize*4);
//	fio_printf(1, psMessage);
	error = host_action(SYS_WRITE, handle, psMessage, strlen(psMessage));
	error = host_action(SYS_WRITE, handle, (void *)buf, strlen((const void *)buf));
	if(error!=0){
		fio_printf(1,"write file error!!\r\n");
		host_action(SYS_CLOSE, handle);
		return;
	}
	host_action(SYS_CLOSE, handle);
}

void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");
		return;
	}

    int dump_status = filedump(argv[1]);
	if(dump_status == -1){
		fio_printf(2, "\r\n%s : no such file or directory.\r\n", argv[1]);
    }else if(dump_status == -2){
		fio_printf(2, "\r\nFile system not registered.\r\n", argv[1]);
    }
}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

    int dump_status = filedump(buf);
	if(dump_status < 0)
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i = 0;i < sizeof(cl)/sizeof(cl[0]) - 1; ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}


void new_command(int n, char *argv[]){

	int taskNum = 1;

    fio_printf(1, "\r\n");
	taskNum = stringToInt(argv[1]);
//	fio_printf(1, "argv[1] = %d\r\n",taskNum);
	for(int i = 0 ; i < taskNum ; i++){
		xTaskCreate(task_func, 
					(signed portCHAR *) "neal",
					StackSize /*stack size*/, NULL, tskIDLE_PRIORITY + 1, NULL);	// tskIDLE_PRIORITY initial is 0
	}

}

void malloctest_command(int n, char *argv[]){
	int size;
	int count = 0;
	char *p;

	fio_printf(2, "\r\n");
	while(1){
		size = StackSize;
		fio_printf(1, "try to allocate %d bytes\r\n", size);
		p = (char *)pvPortMalloc(size);
		fio_printf(1, "malloc return 0x%x\r\n", p);
		if(p == NULL){
			fio_printf(1, "Can not allocate more !! count = %d\r\n", count);
			size *= count;
			fio_printf(1, "The total Stack size is %d\r\n", size);
			return;
		}
		count++;
	}
}

void test_command(int n, char *argv[]) {
    int handle;
    int error;
	int fib, result;

    fio_printf(1, "\r\n");

	fib = stringToInt(argv[1]);
//	fib = atoi(argv[1]);
	fio_printf(1, "input fib = %d\r\n", fib);
//	for(int i=0 ; i<fib ; i++){
		result = fibonacci(fib);
		fio_printf(1, "fibonacci is %d. \r\n", result);
//	}

    handle = host_action(SYS_SYSTEM, "mkdir -p output");
    handle = host_action(SYS_SYSTEM, "touch output/syslog");

    handle = host_action(SYS_OPEN, "output/syslog", 8);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n\r");
        return;
    }

    char *buffer = "Test host_write function which can write data to output/syslog\n";
    error = host_action(SYS_WRITE, handle, (void *)buffer, strlen(buffer));
    if(error != 0) {
        fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
        host_action(SYS_CLOSE, handle);
        return;
    }

    host_action(SYS_CLOSE, handle);
}

void _command(int n, char *argv[]){
    (void)n; (void)argv;
    fio_printf(1, "\r\n");
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}

int fibonacci(int x){
	int previous = -1;
	int result = 1;
	int i = 0, sum = 0;

	for(i = 0 ; i <= x ; i++){
		sum = result + previous;
		previous = result;
		result = sum;
	}

	return result;
}

int stringToInt(char a[]){

	int c, sign, offset, n = 0;

	if(a[0] == '-'){		// Handle negative integers
		sign = -1;
	}

	if(sign == -1){
		offset = 1;			// Set starting position to convert
	}
	else
		offset = 0;

	for(c = offset ; a[c] != '\0' ; c++){
		n = n*10 + a[c]-'0';
	}

	if(sign == -1){
		n = -n;
	}

	return n;
}



