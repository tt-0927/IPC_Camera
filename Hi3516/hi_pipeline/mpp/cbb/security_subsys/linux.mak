default: km_ko_build km_lib_build otp_ko_build otp_lib_build cipher_ko_build cipher_lib_build mbedtls_harden_build hardware_cryptodev_build

clean: km_ko_clean km_lib_clean otp_ko_clean otp_lib_clean cipher_ko_clean cipher_lib_clean mbedtls_harden_clean hardware_cryptodev_clean

ifdef CONFIG_OT_CIPHER_SUPPORT
cipher_ko_build:
	@cd cipher/mkp && make
cipher_ko_clean:
	@cd cipher/mkp && make clean
cipher_lib_build: cipher_ko_build
	@cd cipher/mpi && make
cipher_lib_clean:
	@cd cipher/mpi && make clean
else
cipher_ko_build:
cipher_ko_clean:
cipher_lib_build:
cipher_lib_clean:
endif

ifdef CONFIG_OT_KM_SUPPORT
km_ko_build: cipher_ko_build
	@cd km/mkp && make
km_lib_build: cipher_ko_build
	@cd km/mpi && make
km_ko_clean:
	@cd km/mkp && make clean
km_lib_clean:
	@cd km/mpi && make clean
else
km_ko_build:
km_lib_build:
km_ko_clean:
km_lib_clean:
endif


ifdef CONFIG_OT_OTP_SUPPORT
otp_ko_build: cipher_ko_build
	@cd otp/mkp && make
otp_lib_build: cipher_ko_build
	@cd otp/mpi && make
otp_ko_clean:
	@cd otp/mkp && make clean
otp_lib_clean:
	@cd otp/mpi && make clean
else
otp_ko_build:
otp_lib_build:
otp_ko_clean:
otp_lib_clean:
endif

ifdef CONFIG_OT_MBEDTLS_HARDEN_SUPPORT
mbedtls_harden_build: cipher_ko_build
	@cd mbedtls_harden_adapt && make
mbedtls_harden_clean:
	@cd mbedtls_harden_adapt && make clean
else
mbedtls_harden_build:
mbedtls_harden_clean:
endif

ifdef CONFIG_OT_CRYPTODEV_HARDEN_SUPPORT
hardware_cryptodev_build:
	@cd hardware_cryptodev && make -j
hardware_cryptodev_clean:
	@cd hardware_cryptodev && make clean
else
hardware_cryptodev_build:
hardware_cryptodev_clean:
endif