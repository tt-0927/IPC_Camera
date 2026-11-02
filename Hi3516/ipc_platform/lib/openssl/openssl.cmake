# openssl.cmake

# 注意：openssl 必须保持动态链接。curl/websockets/ffmpeg 等动态库的 .so 带
# 符号@OPENSSL_3.0.0 版本引用，静态归档无法提供带版本号的符号定义，链接必然
# 失败；且这些 .so 运行时仍依赖 libssl.so.3，静态化只会让进程里同时存在两份
# OpenSSL。若未来要静态化，需先把 TLS 消费方全部静态化。
set(OPENSSL_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")
set(OPENSSL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

link_directories(${OPENSSL_LIBRARY_DIR})

# 使用 file(GLOB ...) 查找所有库文件
file(GLOB OPENSSL_LIBRARY
	"${OPENSSL_LIBRARY_DIR}/lib*.so*"
)

# 创建一个命名的 target（目标），方便在 CMake 中使用
add_library(libopenssl INTERFACE)
target_link_libraries(libopenssl INTERFACE ${OPENSSL_LIBRARY})
target_include_directories(libopenssl INTERFACE ${OPENSSL_INCLUDE_DIR})
