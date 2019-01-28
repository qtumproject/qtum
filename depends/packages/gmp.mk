package=gmp
$(package)_version=6.1.2
$(package)_sha256_hash=87b565e89a9a684fe4ebeeddb8399dce2599f9c9049854ca8c0dfbdea0e21912
$(package)_build_subdir=$(package)-$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_download_path=https://gmplib.org/download/$(package)

define $(package)_config_cmds
  ../configure --enable-static=yes --enable-shared=no --enable-cxx -without-readline --host=$(host) --prefix=$(host_prefix)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install 
endef
