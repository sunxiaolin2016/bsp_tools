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

int main(int argc,char *argv[ ])
{
	char gpio_patch[2048];
	int gpio_id;
	int i, ret, gpio_num, len;
	bool gpio_output = false;
	char *src, gpio_base[4], gpio_number[10], set_gpio[12] = "gpio";
	char gpio_path[125] = "/sys/class/gpio/";
	char gpio_direct[125], gpio_value[125], value[124], direction[124];
	//printf("These are the %d command- line arguments passed to main:\n", argc);
	//for(i=0; i < argc; i++)
	//printf("argv[%d]:%s\n", i, argv[i]);

	if(!strncmp(argv[3], "out", 3)) {
		gpio_output = true;
	}

	if(gpio_output) {
		if(argc != 5) {
			//printf("the parameter error！\n");
			printf("config_gpio <group> <pin> <direction> <value>\n");
			return -1;
		}
	}

	if(strncmp(argv[3],"out", 3) && strncmp(argv[3], "in", 2)) {
		printf("direction error!\n");
		return -1;
	}

	//parse gpio base number
	gpio_id = open("/sys/kernel/debug/gpio",O_RDONLY);
	if(gpio_id == -1) {
		printf("open /sys/kernel/debug/gpio err\n");
		return -1;
	}

	ret = read(gpio_id, gpio_patch, sizeof(gpio_patch));
	if(ret < 0) {
		printf("read /sys/kernel/debug/gpio err!\n");
		return -1;
	}

	//printf("gpio_patch:\nlen:%d\n%s\n\n",strlen(gpio_patch),gpio_patch);

	if(!strcmp(argv[1], "gpio0"))
		src = "5d080000";
	else if(!strcmp(argv[1], "gpio1"))
		src = "5d090000";
	else if(!strcmp(argv[1], "gpio2"))
		src = "5d0a0000";
	else if(!strcmp(argv[1], "gpio3"))
		src = "5d0b0000";
	else if(!strcmp(argv[1], "gpio4"))
		src = "5d0c0000";
	else if(!strcmp(argv[1], "gpio5"))
		src = "5d0d0000";
	else if(!strcmp(argv[1], "gpio6"))
		src = "5d0e0000";
	else if(!strcmp(argv[1], "gpio7"))
		src = "5d0f0000";
	else {
		printf("no gpio!\n");
		return -1;
	}

	//printf("src:\n%s\n",src);
	//lookup first "gpiochipx"
	src = strstr(gpio_patch, src);
	len = strlen(gpio_patch) - strlen(src);
	gpio_patch[len] = '\0';
	//printf("lookup first gpio_patch:\nlen: %d\n%s\n",strlen(src),src);

	src = strrchr(gpio_patch, 's');
	//printf("after src:\n%s\n",src);
	//lookup first " "
	src = strstr(src, " ");
	//printf("src:\n%s\n", src);

	for(i = 0; i < 3; i++) {
		gpio_base[i] = src[i + 1];
	}
	gpio_base[3] = '\0';

	//char to int
	gpio_num = atoi(gpio_base) + atoi(argv[2]);

	//int to char
	sprintf(gpio_number,"%d",gpio_num);
	printf("gpio_number:%s\n", gpio_number);

	//unexport gpio
	gpio_id = open("/sys/class/gpio/unexport", O_WRONLY);
	if(gpio_id == -1) {
		printf("open /sys/class/gpio/unexport err!\n");
		return -1;
	}
	//echo xx > /sys/class/gpio/unexport
	ret = write(gpio_id, gpio_number, strlen(gpio_number));

	//export gpio
	gpio_id = open("/sys/class/gpio/export", O_WRONLY);
	if(gpio_id == -1) {
		printf("open /sys/class/gpio/export err!\n");
		return -1;
	}
	//echo xx > /sys/class/gpio/export
	ret = write(gpio_id, gpio_number, strlen(gpio_number));
	if(ret < 0) {
		printf("echo %s > /sys/class/gpio/export err!\n",gpio_number);
		return -1;
	}

	//set_gpio
	strcat(set_gpio, gpio_number);
	strcat(gpio_path, set_gpio);

	//direction
	strncpy(gpio_direct, gpio_path, strlen(gpio_path));
	strcat(gpio_direct, "/direction");
	gpio_id = open(gpio_direct, O_WRONLY);
	if(gpio_id == -1) {
		printf("open %s err\n", gpio_direct);
		return -1;
	}
	//echo xx > /sys/class/gpio/gpioxx/direction
	ret = write(gpio_id, argv[3], strlen(argv[3]));
	if(ret < 0) {
		printf("echo %s > %s err!\n",argv[3], gpio_direct);
		return -1;
	}

	//value
	strncpy(gpio_value, gpio_path, strlen(gpio_path));
	strcat(gpio_value, "/value");
	gpio_id = open(gpio_value, O_WRONLY);
	if(gpio_id == -1) {
		printf("open %s err!\n",gpio_value);
		return -1;
	}
	if(gpio_output) {
			//echo xx > /sys/class/gpio/gpioxx/value
		ret = write(gpio_id, argv[4], strlen(argv[4]));
		if(ret < 0) {
			printf("echo %s > %s err!\n",argv[4], gpio_path);
			return -1;
		}
	}

	//unexport gpio
	gpio_id = open("/sys/class/gpio/unexport", O_WRONLY);
	if(gpio_id == -1) {
		printf("open /sys/class/gpio/unexport err!\n");
		return -1;
	}
	//echo xx > /sys/class/gpio/unexport
	ret = write(gpio_id, gpio_number, strlen(gpio_number));
	if(ret < 0) {
		printf("echo %s > /sys/class/gpio/unexport err!\n",gpio_number);
		return -1;
	}
	printf("unexport end");

    return 0;
}
