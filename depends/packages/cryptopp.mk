package=cryptopp
$(package)_version=5.6.5
$(package)_download_path=https://github.com/weidai11/cryptopp/archive
$(package)_file_name=bccc6443c4d4d611066c2de4c17109380cf97704.tar.gz
$(package)_sha256_hash=f1fddacadd2a0873f795d5614a85fecd5b6ff1d1c6e21dedc251703c54ce63aa

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

