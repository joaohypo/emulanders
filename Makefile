
TARGET_TRIPLE := aarch64-nintendo-switch-freestanding
PROGRAM_ID := 420000000000E311

.PHONY: all dev clean emulanders emulanders-dev sysmodule sysmodule-dev overlay dist dist-dev emulanders-clean

all: emulanders

dev: emulanders-dev

clean: emulanders-clean

emulanders: sysmodule overlay portal dist

emulanders-dev: sysmodule-dev overlay portal dist-dev

sysmodule:
	@cd emulanders && cargo update && cargo nx build --release

sysmodule-dev:
	@cd emulanders && cargo update && cargo nx build

overlay:
	@$(MAKE) -C overlay/

portal:
	@$(MAKE) -C EmulandersPortal/

dist: sysmodule overlay portal
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
	@mkdir -p $(CURDIR)/SdOut/emulanders/res
	@cp $(CURDIR)/res/logo.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_dumper.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_manager.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_editor.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_dumpdl.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_asset.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_settings.png $(CURDIR)/SdOut/emulanders/res/
	@mkdir -p $(CURDIR)/SdOut/switch/EmulandersPortal
	@cp $(CURDIR)/EmulandersPortal/EmulandersPortal.nro $(CURDIR)/SdOut/switch/EmulandersPortal/
	@echo "Creating Release Zips in dist/..."
	@mkdir -p $(CURDIR)/dist
	@rm -f $(CURDIR)/dist/*.zip
	@cd $(CURDIR)/SdOut && zip -rq $(CURDIR)/dist/Emulanders-Bundle-1.0.0.zip .
	@cd $(CURDIR)/SdOut && zip -rq $(CURDIR)/dist/Emulanders-Core-1.0.0.zip atmosphere switch/.overlays emulanders
	@cd $(CURDIR)/SdOut && zip -rq $(CURDIR)/dist/Emulanders-Portal-1.0.0.zip switch/EmulandersPortal
	@echo "Output created at $(CURDIR)/SdOut and release bundles at $(CURDIR)/dist"

dist-dev: sysmodule-dev overlay portal
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
	@mkdir -p $(CURDIR)/SdOut/emulanders/res
	@cp $(CURDIR)/res/logo.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_dumper.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_manager.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_editor.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_dumpdl.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_asset.png $(CURDIR)/SdOut/emulanders/res/
	@cp $(CURDIR)/res/icon_settings.png $(CURDIR)/SdOut/emulanders/res/
	@mkdir -p $(CURDIR)/SdOut/switch/EmulandersPortal
	@cp $(CURDIR)/EmulandersPortal/EmulandersPortal.nro $(CURDIR)/SdOut/switch/EmulandersPortal/
	@echo "Output (dev) created at $(CURDIR)/SdOut"

emulandgen:
	@cd emulandgen && mvn package

emulanders-clean:
	@rm -rf $(CURDIR)/SdOut
	@cd emulanders && cargo clean
	@$(MAKE) clean -C overlay/
	@$(MAKE) clean -C EmulandersPortal/
