mkdir switch_out
/opt/devkitpro/tools/bin/nacptool.exe --create "Minetest" "Many People" "1.0" control.nacp
/opt/devkitpro/tools/bin/elf2nro.exe ./src/minetest switch_out/minetest.nro --nacp=control.nacp --icon=./icon.jpg