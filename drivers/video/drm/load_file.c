/*
 * load_file/load_file.c
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <malloc.h>
#include <asm/cache.h>
#include <sys_partition.h>
#include "load_file.h"
#include <command.h>
#include <linux/string.h>
#include <fs.h>
#include <ext4fs.h>

extern int sunxi_partition_get_partno_byname(const char *part_name);
extern int disp_fat_load(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_fat_size(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_ext4_size(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]);

extern int do_ext4_load(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]);

static int __unload_file(struct file_info_t *file)
{
	if (file) {
		free(file->name);
		free(file->path);
		free(file->file_addr);
		free(file);
		return 0;
	}
	return -1;
}

static int __print_file_info(struct file_info_t *file)
{
	pr_err("File name:%s\n", file->name);
	pr_err("Partition name:%s\n", file->path);
	pr_err("File size:%u\n", file->file_size);
	pr_err("File addr:%p\n", file->file_addr);
	return 0;
}

struct file_info_t *load_file(char *name, char *part_name)
{
	char *argv[6], file_addr[32];
	char size[32] = { 0 };
	char path[32] = { 0 };
	struct file_info_t *file = NULL;
	char cmd[16] = {0};
	char mmc_dev[16] = {0};
	int ret;

	if (!name || !part_name) {
		pr_err("NULL pointer! name:%p, part_name:%p\n", name,
		       part_name);
		goto OUT;
	}

	sprintf(path, "%s/%s", part_name, name);

	for(int i=2; i>=0; i-=2)
	{
		sprintf(mmc_dev, "%d:1", i);
		sprintf(cmd, "mmc dev %d", i);

		if (run_command(cmd, 0)) {
			pr_err("invalid mmc device\n");
			continue;
		} else {
			run_command("mmc part", 0);
		}
		
		struct blk_desc *dev_desc;
		dev_desc = blk_get_dev("mmc", i);
		if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
			pr_err("invalid mmc device\n");
			continue;
		} else {
			ret = fs_set_blk_dev_with_part(dev_desc, 1);
			if (ret < 0)
				pr_err("failed to set partition");
		}
	
		if (fs_set_blk_dev("mmc", simple_itoa(i), FS_TYPE_EXT) == 0) {
			printf("mmc %s is ext fstype\n", mmc_dev);
			argv[0] = "ext4size";
			argv[1] = "mmc";
			argv[2] = mmc_dev;
			argv[3] = path;
			argv[4] = NULL;
			argv[5] = NULL;


			if (!do_ext4_size(0, 0, 4, argv)) {
				file = (struct file_info_t *)malloc(sizeof(struct file_info_t));
				memset(file, 0, sizeof(struct file_info_t));
				file->file_size = env_get_hex("filesize", 0);
			} else {
				pr_err("get file(%s) size from mmc %s error\n", path, mmc_dev);
				continue;
			}

			file->name = (char *)malloc(strlen(name) + 1);
			strncpy(file->name, name, strlen(name) + 1);
			file->path = (char *)malloc(strlen(part_name) + 1);
			strncpy(file->path, name, strlen(name) + 1);
			file->file_addr =
				memalign(4096, file->file_size);

			sprintf(file_addr, "%lx", (unsigned long)file->file_addr);
			snprintf(size, 16, "%lx", (unsigned long)file->file_size);

			argv[0] = "ext4load";
			argv[3] = file_addr;
			argv[4] = path;
			argv[5] = size;

			if (do_ext4_load(0, 0, 6, argv)) {
				pr_err("Unable to open file %s from %s\n", path, mmc_dev);
				continue;
			} else {
				goto GOT_FILE;
			}
		} else if (fs_set_blk_dev("mmc", simple_itoa(i), FS_TYPE_FAT) == 0) {
			printf("mmc %s is fat fstype\n", mmc_dev);
			argv[0] = "fatsize";
			argv[1] = "mmc";
			argv[2] = mmc_dev;
			argv[3] = path;
			argv[4] = NULL;
			argv[5] = NULL;


			if (!do_fat_size(0, 0, 4, argv)) {
				file = (struct file_info_t *)malloc(sizeof(struct file_info_t));
				memset(file, 0, sizeof(struct file_info_t));
				file->file_size = env_get_hex("filesize", 0);
			} else {
				pr_err("get file(%s) size from mmc %s error\n", path, mmc_dev);
				continue;
			}

			file->name = (char *)malloc(strlen(name) + 1);
			strncpy(file->name, name, strlen(name) + 1);
			file->path = (char *)malloc(strlen(part_name) + 1);
			strncpy(file->path, name, strlen(name) + 1);
			file->file_addr =
				memalign(4096, file->file_size);

			sprintf(file_addr, "%lx", (unsigned long)file->file_addr);
			snprintf(size, 16, "%lx", (unsigned long)file->file_size);

			argv[0] = "fatload";
			argv[3] = file_addr;
			argv[4] = path;
			argv[5] = size;

			if (disp_fat_load(0, 0, 6, argv)) {
				pr_err("Unable to open file %s from %s\n", path, mmc_dev);
				continue;
			} else {
				goto GOT_FILE;
			}
		} else {
			printf("mmc %s not found\n", mmc_dev);
		}
	}
	goto FREE_FILE;

GOT_FILE:
#if !CONFIG_IS_ENABLED(SYS_DCACHE_OFF)
	flush_dcache_range((ulong)file->file_addr,
			   ALIGN((ulong)(file->file_addr + file->file_size),
				 CONFIG_SYS_CACHELINE_SIZE));
#endif

	file->unload_file = __unload_file;
	file->print_file_info = __print_file_info;

	return file;

FREE_FILE:
	free(file->name);
	free(file->path);
	free(file->file_addr);
	free(file);
OUT:
	return NULL;
}


int write_file(char *name, char *part_name, void *buf_addr, unsigned int buf_size)
{
	int partno = -1;
	char cmd[100] = {0};

	if (!name || !part_name || !buf_addr || !buf_size) {
		return -1;
	}

	partno = sunxi_partition_get_partno_byname(part_name);
	if (partno < 0) {
		pr_err("%s is not found!\n", part_name);
		goto OUT;
	}

	snprintf(cmd, 100, "fatwrite mmc 0:%x 0x%lx %s 0x%lx 0", partno, (unsigned long)buf_addr, name, (unsigned long)buf_size);

#ifdef CONFIG_FAT_WRITE
	if (run_command(cmd, 0))
		goto OUT;
#else
	pr_err("Please enable CONFIG_FAT_WRITE\n");
	goto OUT;

#endif
	return 0;
OUT:
	return -1;
}

