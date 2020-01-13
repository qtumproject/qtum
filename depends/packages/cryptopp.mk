package=cryptopp
$(package)_version=5.6.5
$(package)_download_path=https://github.com/weidai11/cryptopp/releases
$(package)_file_name=CRYPTOPP_8_2_0.tar.gz
$(package)_sha256_hash=e3bcd48a62739ad179ad8064b523346abb53767bcbefc01fe37303412292343e

define $(package)_set_vars
endef

define $(package)_config_cmds
endef

define $(package)_build_cmds
  $(MAKE) shared static CXXFLAGS="-DNDEBUG -O2 -fPIC"
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

