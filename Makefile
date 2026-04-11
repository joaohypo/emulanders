
TARGET_TRIPLE := aarch64-nintendo-switch-freestanding
PROGRAM_ID := 0100000000000366

.PHONY: all dev clean emulanders emulanders-dev sysmodule sysmodule-dev overlay emulandgen dist dist-dev emulanders-clean emulandgen-clean

all: emulanders emulandgen

dev: emulanders-dev emulandgen

clean: emulanders-clean emulandgen-clean

emulanders: sysmodule overlay dist

emulanders-dev: sysmodule-dev overlay dist-dev

sysmodule:
	@cd emulanders && cargo update && cargo nx build --release

sysmodule-dev:
	@cd emulanders && cargo update && cargo nx build

overlay:
	@$(MAKE) -C overlay/

dist: sysmodule overlay
	@rm -rf $(CURDIR)/SdOut
	@mkdir -p $(CURDIR)/SdOut/atmosphere/contents/$(PROGRAM_ID)/flags
	@touch $(CURDIR)/SdOut/atmosphere/contents/$(PROGRAM_ID)/flags/boot2.flag
	@cp $(CURDIR)/emulanders/target/$(TARGET_TRIPLE)/release/emulanders.nsp $(CURDIR)/SdOut/atmosphere/contents/$(PROGRAM_ID)/exefs.nsp
	@cp $(CURDIR)/emulanders/toolbox.json $(CURDIR)/SdOut/atmosphere/contents/$(PROGRAM_ID)/toolbox.json
	@mkdir -p $(CURDIR)/SdOut/switch/.overlays
	@cp $(CURDIR)/overlay/emulanders.ovl $(CURDIR)/SdOut/switch/.overlays/emulanders.ovl
	@mkdir -p $(CURDIR)/SdOut/emulanders/overlay
	@mkdir -p $(CURDIR)/SdOut/emulanders/figures
	@cp -r $(CURDIR)/overlay/lang $(CURDIR)/SdOut/emulanders/overlay/
	# @zip -r $(CURDIR)/emulanders.zip SdOut 
	@echo "Output created at $(CURDIR)/SdOut"

dist-dev: sysmodule-dev overlay
	@rm -rf $(CURDIR)/SdOut
	@mkdir -p $(CURDIR)/SdOut/atmosphere/contents/$(PROGRAM_ID)/flags
	@touch $(CURDIR)/SdOut/atmosphere/contents/$(PROGRAM_ID)/flags/boot2.flag
	@cp $(CURDIR)/emulanders/target/$(TARGET_TRIPLE)/debug/emulanders.nsp $(CURDIR)/SdOut/atmosphere/contents/$(PROGRAM_ID)/exefs.nsp
	@cp $(CURDIR)/emulanders/toolbox.json $(CURDIR)/SdOut/atmosphere/contents/$(PROGRAM_ID)/toolbox.json
	@mkdir -p $(CURDIR)/SdOut/switch/.overlays
	@cp $(CURDIR)/overlay/emulanders.ovl $(CURDIR)/SdOut/switch/.overlays/emulanders.ovl
	@mkdir -p $(CURDIR)/SdOut/emulanders/overlay
	@mkdir -p $(CURDIR)/SdOut/emulanders/figures
	@cp -r $(CURDIR)/overlay/lang $(CURDIR)/SdOut/emulanders/overlay/
	@echo "Output (dev) created at $(CURDIR)/SdOut"

emulandgen:
	@cd emulandgen && mvn package

emulanders-clean:
	@rm -rf $(CURDIR)/SdOut
	@cd emulanders && cargo clean
	@$(MAKE) clean -C overlay/
n:
	@cd emulandgen && mvn clean
