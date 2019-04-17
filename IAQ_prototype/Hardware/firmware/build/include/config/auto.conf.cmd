deps_config := \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/app_trace/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/aws_iot/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/bt/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/driver/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/esp32/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/esp_adc_cal/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/esp_http_client/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/ethernet/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/fatfs/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/freertos/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/heap/Kconfig \
	/home/alois/Documents/projects/IAQ_cookstove_monitoring/IAQ_prototype/Hardware/firmware/components/libesphttpd/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/libsodium/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/log/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/lwip/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/mbedtls/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/nvs_flash/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/openssl/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/pthread/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/spi_flash/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/spiffs/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/tcpip_adapter/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/vfs/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/wear_levelling/Kconfig \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/alois/Documents/build_systems/espressif/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/alois/Documents/build_systems/espressif/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
