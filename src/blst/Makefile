###############################################################################
# Configuration
###############################################################################

# Settings for blst.
BLST_DIR = .
BLST_BUILDSCRIPT = ./build.sh
BLST_BUILDSCRIPT_FLAGS = -D__BLST_PORTABLE__

###############################################################################
# Core
###############################################################################

# This will build blst without condition.
.PHONY: build_blst
build_blst: $(BLST_BUILDSCRIPT)
	@echo "[+] building blst"
	@cd $(dir $(BLST_BUILDSCRIPT)) && \
	./$(notdir $(BLST_BUILDSCRIPT)) $(BLST_BUILDSCRIPT_FLAGS)

###############################################################################
# Cleanup
###############################################################################

.PHONY: clean
clean:
	@echo "[+] cleaning"
	@rm -f *.o *.a
