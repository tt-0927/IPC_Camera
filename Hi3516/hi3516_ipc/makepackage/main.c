
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "md5lib.h"
#include "reach_os.h"
// #include "edukit_value.h"

/* 升级包ID索引 */
#define UPGRADE_ID_INDEX ("3852")

#pragma pack(1)
typedef struct package
{
	char id[4];		  // 用于标识正常的升级文件UPGRADE_ID_INDEX
	char version[48]; // 用于标识版本号
	char md5[33];	  // md5加密字符串
	long long int len;
	int reserves;
} Upgrade_Package_t;
#pragma pack()
#define CLIENT 1
#define SERVER 0

#define BUILD_TYPE_ITC "itc"
#define BUILD_TYPE "中性"

int tar_add_head(char *src, Upgrade_Package_t *head, char *bin_name)
{
	if ((NULL == src) || (NULL == head))
	{
		printf("tar_add_head param is err!!!\n");
	}

	int ret = 0;
	FILE *fp_src = NULL;
	FILE *fp_dest = NULL;
	char dest[128] = {0};
	strcpy(dest, bin_name);

	fp_src = fopen(src, "r");
	if (fp_src == NULL)
	{
		printf("ERROR,fopen the file %s is failed \n", src);
		return -1;
	}

	fp_dest = fopen(dest, "w+");
	if (fp_dest == NULL)
	{
		fclose(fp_src);
		fp_dest = NULL;
		printf("ERROR,fopen the file %s is failed \n", dest);
		return -1;
	}

	ret = fwrite((char *)head, 1, sizeof(Upgrade_Package_t), fp_dest);
	if (ret != sizeof(Upgrade_Package_t))
	{
		printf("ERROR,the file write first header is error\n");
		fclose(fp_src);
		fclose(fp_dest);
		fp_src = fp_dest = NULL;
		return -1;
	}

	char buff[1024 * 100] = {0};
	int read_len = 0;
	int write_len = 0;

	while (feof(fp_src) == 0)
	{
		memset(buff, 0, sizeof(buff));
		read_len = fread(buff, 1, sizeof(buff), fp_src);
		// printf("fread read_len = %d\n", read_len);
		write_len = fwrite(buff, 1, read_len, fp_dest);
		// printf("fwrite read_len = %d\n", write_len);
		if (read_len != write_len)
		{
			printf("read and write iserror\n");
			fclose(fp_src);
			fclose(fp_dest);
			fp_src = fp_dest = NULL;
			return -1;
		}
	}

	fclose(fp_src);
	fclose(fp_dest);

	return 0;
}

int bin_del_head(char *src_file, char *tg_file)
{
	int32_t ret = -1;
	FILE *targetFile;
	FILE *sourceFile;
	int8_t buffer[2050];
	int flag = 0;
	int write_size = 0;
	Upgrade_Package_t upgrade_pack;
	memset(&upgrade_pack, 0, sizeof(upgrade_pack));

	if (NULL == src_file)
	{
		return -1;
	}

	printf("before open the src_file :%s\n", src_file);
	sourceFile = fopen((char *)src_file, "rb");
	if (NULL == sourceFile)
	{
		printf("src_file open is err!!!\n");
		return -1;
	}

	targetFile = fopen((char *)tg_file, "w+");
	if (NULL == targetFile)
	{
		printf("open targetFile is err!!!\n");
		return -1;
	}

	while (1)
	{
		r_memset(buffer, 0x0, 2050);
		ret = fread((char *)buffer, 1, 2050, sourceFile);
		if (ret <= 0)
		{
			if (0 == ret)
			{
				printf("file end\n");
				break;
			}

			perror("fread");
			return -1;
		}

		if (flag == 0)
		{
			printf("flag is %d ret:%d head:%ld\n", flag, ret, sizeof(Upgrade_Package_t));
			if (ret > sizeof(Upgrade_Package_t))
			{
				memcpy(&upgrade_pack, buffer, sizeof(Upgrade_Package_t));
				printf("the pack->version:%s pack->len:%lld\n", upgrade_pack.version, upgrade_pack.len);
				if (0 == strcmp(upgrade_pack.id, UPGRADE_ID_INDEX))
				{
					write_size = fwrite((char *)buffer + sizeof(Upgrade_Package_t), 1, ret - sizeof(Upgrade_Package_t), targetFile);
					flag = 1;
					continue;
				}
				else
				{
					printf("file head UPGRADE_ID_INDEX is err!!!\n");
					return -1;
				}
			}
		}
		else
		{
			write_size = fwrite((char *)buffer, 1, ret, targetFile);
		}
	}

	fclose(sourceFile);
	fclose(targetFile);

	// char *md5_string = MDFile(tg_file);
	// printf("%s\n", md5_string);
	// unlink(src_file);
	printf("prepare_upgrade_package----[src_file : %s][tg_file : %s]\n", src_file, tg_file);
	return 0;
}

int upgrade_makePacket(char *path, char *out_name, char *version, char *buildType, char *upgrade_id_index, int type)
{
	int nRet;
	DIR *pDir = NULL;
	int8_t cmd[1024] = {0};
	char pack_name[128] = {0};
	char bin_name[128] = {0};

	// if (SERVER == type)
	// {
	// 	sprintf(pack_name, "%s%s.tar.gz", out_name, version);
	// 	sprintf(bin_name, "%s%s.bin", out_name, version);
	// }
	// else if (CLIENT == type)
	// {
	// 	sprintf(pack_name, "%s%s.tar.gz", out_name, version);
	// 	sprintf(bin_name, "%s%s.bin", out_name, version);
	// }

	if (NULL == buildType)
	{
		sprintf(pack_name, "%s.tar.gz", out_name);
		sprintf(bin_name, "%s.bin", out_name);
	}

	pDir = opendir(path);
	if (NULL == pDir)
	{
		printf("Cannot open directory: %s\n", path);
		return 0;
	}
	closedir(pDir);

	char *md5_string = NULL;
#if 0
	char *pt;
	char *pt_tmp = NULL;
	pt = strtok(binDir,"/");
    while (pt)
	{
		pt_tmp = pt;
		pt = strtok(NULL,"/");
	}
	
	printf("%s\n", pt_tmp);
#endif
	// if ()

#if 1
	memset(cmd, 0, 1024);
	sprintf(cmd, "cd %s; tar -zcvf %s ./*", path, pack_name);
	printf("%s---------->\n", cmd);
	if (0 != system(cmd))
	{
		printf("tar update.tar is failed!!!\n");
		return 0;
	}

	Upgrade_Package_t pack = {0};
	memset(&pack, 0, sizeof(pack));
	strncpy(pack.version, version, sizeof(pack.version) - 1);
	pack.version[sizeof(pack.version) - 1] = '\0';
	sprintf(cmd, "%s/%s", path, pack_name);
	pack.len = get_file_size(cmd);
	md5_string = MDFile(cmd);
	printf("the md5_string is %s\n", md5_string);
	if (NULL != md5_string)
	{
		strcpy(pack.md5, md5_string);
		printf("the md5_string is %s\n", md5_string);
	}

	strcpy(pack.id, upgrade_id_index);
	printf("the pack.version=[%s], pack.len=[%lld], pack.id=[%s]\n", version, pack.len, pack.id);

	nRet = tar_add_head(cmd, &pack, bin_name);
	if (0 != nRet)
	{
		printf("tar_add_head is err!!!\n");
	}

	printf("create upgrade file update.bin is success!!\n");
#if 0
	nRet = bin_del_head("./update.bin", "./update.tar.gz");
	if (0 != nRet)
	{
		printf("bin del head is err!!!!\n");
	}
#endif
#endif
	sprintf(cmd, "mv %s/%s ./", path, pack_name);
	system(cmd);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 6)
	{
		printf("用法: %s  <升级文件根目录><软件包名称><版本号 eg: V1.01><构建类型(itc/中性)><升级ID索引(6124 or 204 or 304)><类型(客户端为:1 服务端:0)>\n", argv[0]);
		// printf("Usage: %s <path><out_name><version><buildType(itc or not)><upgrade_id_index(6124 or 204 or 304)><type(client is 1 else 0)>\n", argv[0]);
		return 0;
	}

	int type = 0;
	if (7 == argc)
	{
		type = atoi(argv[6]);
		upgrade_makePacket(argv[1], argv[2], argv[3], argv[4], argv[5], type);
	}
	else if (6 == argc)
	{
		type = atoi(argv[5]);
		upgrade_makePacket(argv[1], argv[2], argv[3], NULL, argv[4], type);
	}

	return 0;
}
