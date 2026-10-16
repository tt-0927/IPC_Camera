set(BLE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(BLE_INCLUDE_DIR "${BLE_ROOT_DIR}/include")
set(BLE_LIBRARY_DIR "${BLE_ROOT_DIR}/lib")

add_library(libble_gdbus INTERFACE)

target_include_directories(libble_gdbus INTERFACE
    "${BLE_INCLUDE_DIR}/glib-2.0"
    "${BLE_LIBRARY_DIR}/glib-2.0/include"
)

target_link_libraries(libble_gdbus INTERFACE
    "${BLE_LIBRARY_DIR}/libgio-2.0.so"
    "${BLE_LIBRARY_DIR}/libgobject-2.0.so"
    "${BLE_LIBRARY_DIR}/libgmodule-2.0.so"
    "${BLE_LIBRARY_DIR}/libglib-2.0.so"
    "${BLE_LIBRARY_DIR}/libffi.so"
    "${BLE_LIBRARY_DIR}/libpcre2-8.so"
)

target_link_options(libble_gdbus INTERFACE
    "-Wl,-rpath-link,${BLE_LIBRARY_DIR}"
)