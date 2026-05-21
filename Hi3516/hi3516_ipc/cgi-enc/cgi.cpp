/**
 * @FilePath     : cgi.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-17 11:14:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-20 09:52:51
 * @Description  : web通信接口封装
 */

#include "cgi.hpp"
#include "fcgi_stdio.h"
#include "http_sdk_gateway.hpp"
#include "share_os.h"
#include "share_port.h"

// 全局变量
extern char *cgiRemoteAddr;

namespace WebCGI
{
	namespace
	{
		/**
		 * @brief 获取CGI运行环境变量
		 * @param pKey 环境变量名称
		 * @return 返回环境变量值，未配置时返回空字符串
		 */
		std::string getEnvString(const char *pKey)
		{
			const char *pValue = getenv(pKey);
			return pValue ? std::string(pValue) : std::string();
		}
	}

	JsonPtr JsonHelper::parseJson(const std::string &strJson)
	{
		if (strJson.empty())
		{
			throw CGIException("空的 JSON 字符串", ERR_PARAM_NULL);
		}

		cJSON *json = cJSON_Parse(strJson.c_str());
		if (!json)
		{
			throw CGIException("无效的 JSON 格式", ERR_PARSE);
		}

		return JsonPtr(json, cJSON_Delete);
	}

	std::string JsonHelper::createJsonResponse(const JsonResponse_S &stResponse)
	{
		auto jsonRoot = JsonPtr(cJSON_CreateObject(), cJSON_Delete);
		if (!jsonRoot)
		{
			throw CGIException("无法创建 JSON 对象");
		}

		cJSON_AddItemToObject(jsonRoot.get(), "ActionCode", cJSON_CreateNumber(stResponse.nActionCode));
		cJSON_AddItemToObject(jsonRoot.get(), "DeviceName", cJSON_CreateString(stResponse.strDeviceName.c_str()));
		cJSON_AddItemToObject(jsonRoot.get(), "UserName", cJSON_CreateString(stResponse.strUserName.c_str()));
		cJSON_AddItemToObject(jsonRoot.get(), "Return", cJSON_CreateNumber(stResponse.nReturnCode));

		/*处理Data字段*/
		cJSON *pDataJson = nullptr;
		if (!stResponse.strData.empty())
		{
			pDataJson = cJSON_Parse(stResponse.strData.c_str());
		}

		if (!pDataJson)
		{
			pDataJson = cJSON_CreateObject();
		}

		cJSON_AddItemToObject(jsonRoot.get(), "Data", pDataJson);

		char *pJsonStr = cJSON_Print(jsonRoot.get());
		if (!pJsonStr)
		{
			throw CGIException("Failed to serialize JSON");
		}

		std::string result(pJsonStr);
		cJSON_free(pJsonStr);
		return result;
	}

	int JsonHelper::extractActionCode(const std::string &strJson)
	{
		auto json = parseJson(strJson);

		cJSON *actionCodeItem = cJSON_GetObjectItem(json.get(), "ActionCode");
		if (!actionCodeItem || !cJSON_IsNumber(actionCodeItem))
		{
			throw CGIException("Missing or invalid ActionCode", ERR_PARSE);
		}

		return static_cast<int>(actionCodeItem->valuedouble);
	}

	bool JsonHelper::validateJsonStructure(const std::string &strJson)
	{
		try
		{
			auto json = parseJson(strJson);

			/*检查必要字段*/
			if (!cJSON_GetObjectItem(json.get(), "ActionCode"))
			{
				return false;
			}

			return true;
		}
		catch (const CGIException &)
		{
			return false;
		}
	}

	// LoginCommandHandler实现
	int LoginCommandHandler::execute(ShortCallbackMsg_t *pMsg)
	{
		if (!pMsg || !pMsg->value)
		{
			throw CGIException("Invalid message parameters", ERR_PARAM_NULL);
		}

		char *xmlData = static_cast<char *>(pMsg->value);

		// 设置响应头
		fprintf(cgiOut, "Content-type: text/html\n\n");
		fprintf(cgiOut, "%s\n\n", xmlData);

		return OK;
	}

	/**
	 * @brief 执行通用JSON命令处理逻辑
	 * @param pMsg 短链接回调消息指针，包含后端返回JSON数据
	 * @return 返回JSON命令处理结果码
	 */
	int JsonCommandHandler::execute(ShortCallbackMsg_t *pMsg)
	{
		if (!pMsg || !pMsg->value)
		{
			throw CGIException("Invalid message parameters", ERR_PARAM_NULL);
		}

		fprintf(cgiOut, "Content-Type: application/json\r\n\r\n");
		fprintf(cgiOut, "%s\n", pMsg->value);
		return OK;
	}

	// NetworkClient实现
	int NetworkClient::sendRequest(const ShortLink_Send_t &stRequest)
	{
		return shortLink_creat_netClient(const_cast<ShortLink_Send_t *>(&stRequest));
	}

	// CGIProcessor实现
	CGIProcessor::CGIProcessor()
	{
		registerHandlers();
		if (cgiRemoteAddr)
		{
			remoteAddr = std::string(cgiRemoteAddr);
		}
	}

	void CGIProcessor::registerHandlers()
	{
		commandHandlers[AC_LOGIN] = std::make_unique<LoginCommandHandler>();

		/* 人脸抓拍配置 */
		commandHandlers[AC_GET_FACE_CAPTURE_INFO] = std::make_unique<JsonCommandHandler>(AC_GET_FACE_CAPTURE_INFO);
		commandHandlers[AC_SET_FACE_CAPTURE_INFO] = std::make_unique<JsonCommandHandler>(AC_SET_FACE_CAPTURE_INFO);

		/* 人脸比对配置 */
		commandHandlers[AC_SET_FACE_COMPARE_INFO] = std::make_unique<JsonCommandHandler>(AC_SET_FACE_COMPARE_INFO);

		/* 目标库管理 */
		commandHandlers[AC_ADD_TARGET_LIB] = std::make_unique<JsonCommandHandler>(AC_ADD_TARGET_LIB);
		commandHandlers[AC_DEL_TARGET_LIB] = std::make_unique<JsonCommandHandler>(AC_DEL_TARGET_LIB);
		commandHandlers[AC_SET_TARGET_LIB] = std::make_unique<JsonCommandHandler>(AC_SET_TARGET_LIB);
		commandHandlers[AC_GET_TARGET_LIB] = std::make_unique<JsonCommandHandler>(AC_GET_TARGET_LIB);

		/* 人脸人员管理 */
		commandHandlers[AC_ADD_FACE_INFO] = std::make_unique<JsonCommandHandler>(AC_ADD_FACE_INFO);
		commandHandlers[AC_DEL_FACE_INFO] = std::make_unique<JsonCommandHandler>(AC_DEL_FACE_INFO);
		commandHandlers[AC_SET_FACE_INFO] = std::make_unique<JsonCommandHandler>(AC_SET_FACE_INFO);
		commandHandlers[AC_GET_FACE_INFO] = std::make_unique<JsonCommandHandler>(AC_GET_FACE_INFO);
	}

	void CGIProcessor::setupCORSHeaders()
	{
		fprintf(cgiOut, "Access-Control-Allow-Origin: *\n");
		fprintf(cgiOut, "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\n");
		fprintf(cgiOut, "Access-Control-Allow-Headers: Content-Type, Authorization\n");
	}

	void CGIProcessor::sendErrorResponse(int nErrorCode, const std::string &strMessage, int nActionCode)
	{
		JsonResponse_S stResponse(nActionCode, nErrorCode, "");

		try
		{
			std::string strJson = JsonHelper::createJsonResponse(stResponse);
			setupCORSHeaders();
			fprintf(cgiOut, "Content-Type: application/json\r\n\r\n");
			fprintf(cgiOut, "%s\n", strJson.c_str());
		}
		catch (const CGIException &e)
		{
			setupCORSHeaders();
			fprintf(cgiOut, "Content-type: text/plain\r\n\r\n");
			fprintf(cgiOut, "{\"result\": %d, \"message\": \"%s\"}", nErrorCode, strMessage.c_str());
		}
	}

	std::string CGIProcessor::readRequestBody()
	{
		const char *pContentLengthStr = getenv("CONTENT_LENGTH");
		if (!pContentLengthStr)
		{
			return "{}";
		}

		int nContentLength = atoi(pContentLengthStr);
		if (nContentLength <= 0)
		{
			return "{}";
		}

		/*限制最大请求体大小，防止内存溢出*/
		constexpr int MAX_CONTENT_LENGTH = 2 * 1024 * 1024; // 2MB
		if (nContentLength > MAX_CONTENT_LENGTH)
		{
			throw CGIException("Request body too large", ERR_PARAM_NULL);
		}

		std::vector<char> vBuffer(nContentLength + 1, 0);
		size_t bytesRead = fread(vBuffer.data(), 1, nContentLength, stdin);

		DEBUG_LOG("nContentLength:%d bytesRead:%d\nbuffer.data():%s\n", nContentLength, bytesRead, vBuffer.data());

		if (static_cast<int>(bytesRead) != nContentLength)
		{
			throw CGIException("无法读取完整的请求正文", ERR);
		}

		return std::string(vBuffer.data(), bytesRead);
	}

	/**
	 * @brief 构造后端任务报文
	 * @param strRequestBody HTTP请求体
	 * @param nActionCode 输出内部动作码
	 * @return 返回后端可识别的JSON报文
	 * @note 旧ActionCode JSON原样透传；HTTP-SDK命令由CHttpSdkGateway转换为ActionCode JSON。
	 */
	std::string CGIProcessor::buildBackendJson(const std::string &strRequestBody, int &nActionCode)
	{
		if (JsonHelper::validateJsonStructure(strRequestBody))
		{
			nActionCode = JsonHelper::extractActionCode(strRequestBody);
			return strRequestBody;
		}

		const std::string strMethod = getEnvString("REQUEST_METHOD");
		const std::string strUri = getEnvString("REQUEST_URI");
		if (CHttpSdkGateway::isGatewayRequest(strMethod, strUri))
		{
			return CHttpSdkGateway::buildBackendJson(strRequestBody, nActionCode);
		}

		throw CGIException("Invalid JSON structure", ERR_PARSE);
	}

	int CGIProcessor::processCommand(int nActionCode, const std::string &strJsonData)
	{
		auto it = commandHandlers.find(nActionCode);
		if (it == commandHandlers.end())
		{
			throw CGIException("Unsupported action code: " + std::to_string(nActionCode), ERR);
		}

		return sendToBackend(nActionCode, strJsonData);
	}

	int CGIProcessor::sendToBackend(int nActionCode, const std::string &strJsonData)
	{
		ShortLink_Send_t shortHandle;
		memset(&shortHandle, 0, sizeof(ShortLink_Send_t));

		shortHandle.code = nActionCode;
		shortHandle.dealcmd = [](ShortCallbackMsg_t *pMsg) -> int
		{
			if (!pMsg || !pMsg->sOperHandle || !pMsg->value)
			{
				return -1;
			}

			// 设置CORS头
			fprintf(cgiOut, "Access-Control-Allow-Origin: *\n");
			fprintf(cgiOut, "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\n");
			fprintf(cgiOut, "Access-Control-Allow-Headers: Content-Type, Authorization\n");

			// 根据命令类型处理
			CGIProcessor processor;
			auto it = processor.commandHandlers.find(pMsg->Code);
			if (it != processor.commandHandlers.end())
			{
				return it->second->execute(pMsg);
			}

			return ERR;
		};

		getlocalip(shortHandle.iP);
		shortHandle.port = WEB_COMMUNTICATION_PORT;

		// 创建完整的消息（包含远程IP信息）
		std::string fullMessage = strJsonData + "|" + remoteAddr;
		shortHandle.message = const_cast<char *>(fullMessage.c_str());
		shortHandle.nLen = fullMessage.length();

		return NetworkClient::sendRequest(shortHandle);
	}

	int CGIProcessor::processRequest()
	{
		try
		{
			const std::string strMethod = getEnvString("REQUEST_METHOD");
			if (strMethod == "OPTIONS")
			{
				setupCORSHeaders();
				fprintf(cgiOut, "\r\n");
				return OK;
			}

			/*读取请求体*/
			std::string strJsonData = readRequestBody();

			/*兼容旧ActionCode报文和HTTP-SDK转发命令*/
			int nActionCode = 0;
			strJsonData = buildBackendJson(strJsonData, nActionCode);

			/*验证ActionCode*/
			if (!isValidActionCode(nActionCode))
			{
				sendErrorResponse(ERR_PARAM_NULL, "Invalid action code", nActionCode);
				return ERR_PARAM_NULL;
			}

			/*处理命令*/
			setupCORSHeaders();
			return processCommand(nActionCode, strJsonData);
		}
		catch (const CGIException &e)
		{
			sendErrorResponse(e.getErrorCode(), e.what());
			return e.getErrorCode();
		}
		catch (const std::exception &e)
		{
			sendErrorResponse(ERR, e.what());
			return ERR;
		}
		catch (...)
		{
			sendErrorResponse(ERR, "Unknown error occurred");
			return ERR;
		}
	}

	bool CGIProcessor::isValidActionCode(int nActionCode)
	{
		return commandHandlers.find(nActionCode) != commandHandlers.end();
	}

	// Utils命名空间实现
	namespace Utils
	{
		void redirectStdioToLog()
		{
			int logFileDescriptor = open("/tmp/cgi_log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (logFileDescriptor != -1)
			{
				dup2(logFileDescriptor, STDOUT_FILENO);
				dup2(logFileDescriptor, STDERR_FILENO);
				close(logFileDescriptor);
				setvbuf(stderr, nullptr, _IONBF, 0);
			}
			else
			{
				perror("打开日志文件失败");
			}
		}

		void debugLog(const char *pFunction, int nLine, const char *pFormat, ...)
		{
			char aBuffer[LOG_BUFFER_SIZE];
			int offset = snprintf(aBuffer, sizeof(aBuffer), "[Func]:%s [Line]:%d ", pFunction, nLine);

			va_list args;
			va_start(args, pFormat);
			int length = vsnprintf(aBuffer + offset, sizeof(aBuffer) - offset, pFormat, args);
			va_end(args);

			if (length > 0)
			{
				int totalLength = offset + length;
				if (totalLength > static_cast<int>(sizeof(aBuffer)))
				{
					totalLength = sizeof(aBuffer);
				}
				write(STDERR_FILENO, aBuffer, totalLength);
			}
		}


		std::string getCurrentTimestamp()
		{
			time_t currentTime = time(nullptr);
			char aBuffer[100];
			strftime(aBuffer, sizeof(aBuffer), "%Y-%m-%d %H:%M:%S", localtime(&currentTime));
			return std::string(aBuffer);
		}



		std::string sanitizeInputString(const std::string &strInput)
		{
			std::string strResult = strInput;
			const std::string dangerousChars = "<>\"'&";

			for (char dangerousChar : dangerousChars)
			{
				strResult.erase(
					std::remove(strResult.begin(), strResult.end(), dangerousChar),
					strResult.end());
			}
			return strResult;
		}
	}
}

/*C风格的主入口函数*/
extern "C" int cgiMain(void)
{
	WebCGI::Utils::redirectStdioToLog();
    
    WebCGI::CGIProcessor processor;
    auto result = processor.processRequest();
    
    return result;
}
