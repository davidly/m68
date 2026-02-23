g++ -DNDEBUG -DM68 -O3 -fno-builtin -I .  -ffunction-sections -fdata-sections m68.cxx m68000.cxx -o m68 -static  -Wl,--gc-sections
