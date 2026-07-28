if [[ "$(uname -s)" == "Darwin" ]]; then
    g++ -O2 -I . -I ../.. symto68k.cxx -o symto68k
else
    g++ -O3 -fno-builtin -I . -I ../.. symto68k.cxx -o symto68k -static
fi