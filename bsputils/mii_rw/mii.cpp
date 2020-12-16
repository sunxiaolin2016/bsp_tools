#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#ifndef __GLIBC__
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#endif
#include "mii.h"

#define dbg(fmt,...) \
	do {\
		fprintf(stdout, fmt, ##__VA_ARGS__);\
	} while (0)

#define err(fmt,...) \
	do {\
		fprintf(stderr, fmt, ##__VA_ARGS__);\
	} while (0)

static struct ifreq ifr;
static int mdio_read(int skfd, int location)
{
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
	mii->reg_num = location;
	if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
		fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
		return -1;
	}
	return mii->val_out;
}

static int mdio_write(int skfd, int location, int value)
{
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
	mii->reg_num = location;
	mii->val_in = value;
	if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
		fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
		return -1;
	}
	return 0;
}

struct phyaddress_interface_map {
	int phyaddr;	//phy addres
	char* intf;	//interface name
} g_sja1105p_phy_intf_maps[] = {
	{0, "SJA1105P_p1"},
	{4, "SJA1105P_p1"},
	{6, "SJA1105P_p2"}
};
static char* phyaddr_to_ifname(int phyaddr)
{
	int i;
	for (i = 0; i < sizeof(g_sja1105p_phy_intf_maps)
			/ sizeof(g_sja1105p_phy_intf_maps[0]); i++) {
		if (g_sja1105p_phy_intf_maps[i].phyaddr == phyaddr)
			return g_sja1105p_phy_intf_maps[i].intf;
	}
	return "Unkown";
}
#define MII_ACT_RD	(0x1)
#define MII_ACT_WR	(0x2)

/*
usage:
	//do read phy(address 0x6 - indentified by -D)'s reg 0x0
	mii -D 0x6 -R 0x0
	mii -d 6 -r 0
	//do write 0x1 to reg 0x0 of phy(address 0x6 -identified by -D)
	mii -D 0x6 -W 0x0 -V 0x1
	mii -d 6 -w 0 -v 1
*/

static void usage(char* base)
{
	fprintf(stdout, "%s -D|-d  phyaddr -R|-r reg -W|-w reg -V|-v reg_val\n"\
		"-D/-R/-W/-V require 0x prefix, which means hexadecimal\n"\
		"-d/-r/-w/-v require none 0x prefix, which means decimal.\n",
		base);
	exit(0);
}


static void parse_args(int argc, char* argv[],
	int *phyaddr, int *act, int *reg, int *val)
{
	int opt;
	extern char *optarg;

	while ((opt = getopt(argc, argv, "D:R:W:V:d:r:w:v")) != -1) {
		switch (opt) {
		case 'D':
			*phyaddr = strtol(optarg, NULL, 16);
			break;
		case 'd':
			*phyaddr = atoi(optarg); 
			break;
		case 'R':
			*reg = strtol(optarg, NULL, 16);
			*act = MII_ACT_RD;
			break;
		case 'r':
			*reg = atoi(optarg);
			*act = MII_ACT_RD;
			break;
		case 'W':
			*reg = strtol(optarg, NULL, 16);
			*act = MII_ACT_WR;
			break;
		case 'w':
			*reg = atoi(optarg); 
			*act = MII_ACT_WR;
			break;
		case 'V':
			*val = strtol(optarg, NULL, 16);
			break;
		case 'v':
			*val = atoi(optarg); 
			break;
		default:
			usage(argv[0]);
		}
	}
}
static int check_input(int phyaddr, int act, int reg, int val)
{
	if (phyaddr != 0 && phyaddr != 6 && phyaddr != 4) {
		err("phyaddr:0x%x invalid\n", phyaddr);
		goto out;
	}
	if (act != MII_ACT_WR && act != MII_ACT_RD) {
		err("act(%d) err!\n", act);
		goto out;
	}
	if (reg < 0 || reg > 0xff) {
		err("register(%d) should be [0, 255]\n", reg);
		goto out;
	}
	if (act == MII_ACT_WR  && (val < 0 || reg > 0xffff)) {
		err("value(%d) should be [0, 0xffff], 16bit width!\n", val);
		goto out;
	}
	return 0;
out:
	return -1;
} 
int do_mii(int act, int phyaddr, int reg, int val)
{
	int fd, rc = 0;
	char* ifname = phyaddr_to_ifname(phyaddr);
	/* Open a basic socket. */
	if ((fd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		perror("socket");
		exit(-1);
	}

	/* Get the vitals from the interface. */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(fd, SIOCGMIIPHY, &ifr) < 0) {
		fprintf(stderr, "SIOCGMIIPHY on '%s' failed: %s\n",
				ifname, strerror(errno));
		goto out;
	}

	if (act == MII_ACT_RD)	{
		val = mdio_read(fd, reg);
		if (val < 0) {
			fprintf(stderr, "phy: 0x%x read 0x%02x Fail!\n",
					phyaddr, reg);
			goto out;
		}
		fprintf(stdout, "phy: 0x%x read 0x%02x = 0x%02x\n",
			phyaddr, reg, val);
	} else {
		rc = mdio_write(fd, reg, val);
		if (rc < 0) {
			fprintf(stderr, "phy: 0x%x write 0x%02x = 0x%x Fail!\n",
					phyaddr, reg, val);
			perror("write fail:");
			goto out;
		}
		fprintf(stdout, "phy: 0x%x write 0x%02x = 0x%02x\n",
			phyaddr, reg, val);
	}
out:
	close(fd);
	return rc;
}
int main(int argc, char* argv[])
{
	int phyaddr = -1 , act = -1 , reg = -1 , val = -1;
	int rc;	
	parse_args(argc, argv, &phyaddr, &act, &reg, &val);

	rc = check_input(phyaddr, act, reg, val);
	if (rc)
		usage(argv[0]);

	if (act == MII_ACT_RD)
		dbg("request read 0x%x on phy 0x%x(%s)\n",
			reg, phyaddr,
			phyaddr_to_ifname(phyaddr));
	else 
		dbg("request write 0x%x to 0x%x on phy 0x%x(%s)\n",
			val, reg, phyaddr,
			phyaddr_to_ifname(phyaddr));

	return do_mii(act, phyaddr, reg, val);
}
