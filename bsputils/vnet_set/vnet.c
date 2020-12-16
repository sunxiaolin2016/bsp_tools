#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cutils/log.h>
//#include <utils/Log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cutils/properties.h>

#define MAX_CMD_LEN 80

int main (void)
{
	char vnet_address[PROPERTY_VALUE_MAX] = "000";
	char ptp_add[20],ip_add[20];
	bool ptp = false;
//	pid_t id = 0;
	int status;
	int i_num , temp = 0, ret = 0 ,len = 0;
//	char* cmd_argv[] = {"ifconfig", "vnet0", ip_add, NULL};
//	char* cmd_argv1[] = {"ifconfig","vnet0", "pointopoint", ptp_add, NULL};
        char cmd[MAX_CMD_LEN];
	if(property_get("ro.boot.vnet", vnet_address, NULL) != 0) {
		printf("ro.boot.vnet = %s\n", vnet_address);
	}
	len = strlen(vnet_address);
	for(i_num = 0; i_num < len; i_num++) {
		if(vnet_address[i_num] == ',') {
			ptp = true;
			ip_add[i_num] = '\0';
			temp = i_num;
			continue;
		}

		if(ptp) {
			ptp_add[i_num-temp -1] = vnet_address[i_num];
		}
		else {
			ip_add[i_num] = vnet_address[i_num];
		}
	}
	ptp_add[i_num-temp-1] = '\0';

	printf("len:%ld, ip_add:%s\n", strlen(ip_add), ip_add);
	printf("len:%ld, ptp_add:%s\n", strlen(ptp_add), ptp_add);
        
        snprintf(cmd, MAX_CMD_LEN, "/system/bin/ip-wrapper-1.0 addr add %s/32 peer %s/32 dev vnet0", ip_add, ptp_add);
        printf("exec %s\n", cmd);
        system(cmd);
        system("/system/bin/ip-wrapper-1.0 link set vnet0 up");

//	id = fork();
//	if(id < 0) {
//		printf("error in fork!\n");
//	}
//	else if(id == 0) {
//		//printf("child runing, process id is %d\n",getpid());
//		execv("/system/bin/ifconfig", cmd_argv);
//		exit(0);
//	}
//	else {
//		wait(&status);
//		//printf("father runing, process id is %d\n",getpid());
//		execv("/system/bin/ifconfig", cmd_argv1);
//	}

    return 0;
}
