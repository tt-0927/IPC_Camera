/***
 * @FilePath     : upnp_example.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2025-03-20 14:16:09
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-14 14:41:31
 * @Description  :upnp测试程序
 */

#include <iostream>
#include <thread>
#include <atomic>

#include "upnp_manage.h"
#include "network_define.h"

std::atomic<bool> g_running(true);
Network::PortMapConfig_S g_newConfig;

void input_handler()
{
	while (g_running)
	{
		std::cout << "\n命令选项: \n"
				  << "  s - 显示状态\n"
				  << "  e - 启用UPnP映射\n"
				  << "  a - 启用自动UPnP映射\n"
				  << "  d - 禁用UPnP映射\n"
				  << "  q - 退出程序\n"
				  << "请输入命令: ";

		char cmd = ' ';
		std::cin >> cmd;

		switch (cmd)
		{
		case 's': // 显示状态
			CUpnpManage::instance()->print_status();
			break;

		case 'e': // 启用UPnP
		{

			g_newConfig.bEnablePortMap = true;
			CUpnpManage::instance()->set_port_map(g_newConfig);
			break;
		}
		case 'a': // 启用UPnP自动映射
		{

			g_newConfig.nMapType = 1; /* 自动映射 */
			g_newConfig.bEnablePortMap = true;
			CUpnpManage::instance()->set_port_map(g_newConfig);
			break;
		}
		case 'd': // 禁用UPnP
		{
			g_newConfig.bEnablePortMap = false;
			CUpnpManage::instance()->set_port_map(g_newConfig);
			break;
		}

		case 'q': // 退出程序
			g_running = false;
			break;

		default:
			std::cout << "未知命令，请重新输入" << std::endl;
		}
	}
}

int main()
{
	initLog("logger", "./upnp.log");
	g_newConfig.nMapType = 0;

	g_newConfig.portMap =
		{
			{0, 80, "0.0.0.0", 80, 0},	  // HTTP (80→80)
			{1, 554, "0.0.0.0", 554, 0},  // RTSP (554→554)
			{2, 443, "0.0.0.0", 443, 0},  // HTTPS (443→443)
			{3, 8000, "0.0.0.0", 8000, 0} // 自定义服务 (8000→8000)
		};
	CUpnpManage::instance()->init();

	std::thread input_thread(input_handler);

	while (g_running)
	{
		// 主循环保持运行
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	input_thread.join();
	// CUpnpManage::instance()->deinit();

	return 0;
}