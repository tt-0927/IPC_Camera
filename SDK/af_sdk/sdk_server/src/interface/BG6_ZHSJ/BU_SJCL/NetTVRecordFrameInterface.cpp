/*
 * @FilePath     : sdk_new/sdk_server/src/interface/BG6_ZHSJ/BU_SJCL/NetTVRecordFrameInterface.cpp
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : 录像帧流服务端 C 接口实现（BU_SJCL / NVR 侧）
 *                 从 Common/NetTVSDKServerInterface.cpp 迁出，独立业务不经 PIMPL，
 *                 直接调用 CRecordFrameServer 单例。
 *                 对外声明仍在 NetTVSDKServerInterface.h（共用头文件）。
 */

#include "NetTVSDKServerInterface.h"
#include "RecordFrameServer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 录像帧流 RecordFrame (服务端) ==================== */

/**
 * @brief 启动录像帧流 TCP 服务
 * @details 创建监听socket，开始接收客户端连接
 * @param [IN] dwPort TCP监听端口，建议使用9005
 * @return TRUE 成功，FALSE 失败
 * @note 服务端必须先启动此服务，客户端才能建立TCP连接接收帧数据
 */
NET_API BOOL STDCALL
NET_serverStartRecordFrameServer(IN UINT32 dwPort)
{
	return tvsdk::CRecordFrameServer::instance()->start(static_cast<int>(dwPort)) ? TRUE : FALSE;
}

/**
 * @brief 停止录像帧流 TCP 服务
 * @details 关闭监听socket，停止所有客户端连接，释放资源
 * @return TRUE 成功
 */
NET_API BOOL STDCALL
NET_serverStopRecordFrameServer(void)
{
	tvsdk::CRecordFrameServer::instance()->stop();
	return TRUE;
}

/**
 * @brief 注册录像帧流启动回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [IN] cb 宿主实现的启动回调函数指针，NULL表示取消注册
 * @param [IN] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return TRUE 成功
 * @note 为什么要这样写？
 *       1. 宿主程序使用C语言，回调是C风格函数指针，带有lpUserData参数
 *       2. SDK内部使用C++，RecordFrameServer期望的是C++风格的std::function
 *       3. 需要用Lambda做适配器，将C风格回调转换为C++风格回调
 *       4. cond参数做了拷贝（condCopy），因为C回调期望指针，防止引用失效
 */
NET_API BOOL STDCALL
NET_serverRegisterRecordFrameStartCb(IN NET_serverRecordFrameStartCallBack cb,
										 IN LPVOID lpUserData)
{
	if (!cb) {
		/* 取消注册，设置为空回调 */
		tvsdk::CRecordFrameServer::instance()->set_start_callback(nullptr);
		return TRUE;
	}

	/* 使用Lambda做适配器：C风格回调 → C++风格回调 */
	tvsdk::CRecordFrameServer::instance()->set_start_callback(
		[cb, lpUserData](const NET_RecordFrameStreamCond_S& cond,
						 NET_RecordFrameStreamInfo_S& info) -> NET_COMMON_ECODE_E {
			/* cond参数做拷贝，因为C回调期望指针，防止引用失效 */
			NET_RecordFrameStreamCond_S condCopy = cond;
			/* 调用宿主注册的C风格回调，传入lpUserData */
			return cb(&condCopy, &info, lpUserData);
		});
	return TRUE;
}

/**
 * @brief 注册录像帧读取回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [IN] cb 宿主实现的读取回调函数指针，NULL表示取消注册
 * @param [IN] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return TRUE 成功
 * @note 为什么要这样写？
 *       1. C回调参数：const CHAR* szStreamId, UINT32 dwBufferSize
 *       2. C++回调参数：const std::string& streamId, size_t bufferSize
 *       3. 需要做类型转换：std::string → const char*, size_t → UINT32
 *       4. Read回调会被持续循环调用，Lambda捕获保证cb和lpUserData不会失效
 */
NET_API BOOL STDCALL
NET_serverRegisterRecordFrameReadCb(IN NET_serverRecordFrameReadCallBack cb,
										IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::CRecordFrameServer::instance()->set_read_callback(nullptr);
		return TRUE;
	}

	tvsdk::CRecordFrameServer::instance()->set_read_callback(
		[cb, lpUserData](const std::string& streamId,
						 NET_RecordFrameInfo_S& frameInfo,
						 char* buffer,
						 size_t bufferSize) -> int {
			/* 类型转换：std::string → const char*, size_t → UINT32 */
			return cb(streamId.c_str(), &frameInfo, buffer, static_cast<UINT32>(bufferSize), lpUserData);
		});
	return TRUE;
}

/**
 * @brief 注册录像帧流停止回调
 * @details 将宿主程序实现的C风格回调转换为C++ Lambda，传递给RecordFrameServer
 *          Lambda捕获cb和lpUserData，在回调执行时恢复C风格函数调用
 * @param [IN] cb 宿主实现的停止回调函数指针，NULL表示取消注册
 * @param [IN] lpUserData 用户自定义数据，SDK会原样传回给宿主
 * @return TRUE 成功
 * @note 为什么要这样写？
 *       1. C回调参数：const CHAR* szStreamId
 *       2. C++回调参数：const std::string& streamId
 *       3. 需要做类型转换：std::string → const char*
 */
NET_API BOOL STDCALL
NET_serverRegisterRecordFrameStopCb(IN NET_serverRecordFrameStopCallBack cb,
										IN LPVOID lpUserData)
{
	if (!cb) {
		tvsdk::CRecordFrameServer::instance()->set_stop_callback(nullptr);
		return TRUE;
	}

	tvsdk::CRecordFrameServer::instance()->set_stop_callback(
		[cb, lpUserData](const std::string& streamId) -> NET_COMMON_ECODE_E {
			return cb(streamId.c_str(), lpUserData);
		});
	return TRUE;
}

#ifdef __cplusplus
}
#endif
