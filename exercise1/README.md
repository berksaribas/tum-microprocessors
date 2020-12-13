# Exercise 1
- Cakir, Yunus Can
- Lönnbäck, Otto Knut Mikael
- Saribas, Berk

# How to compile
- You need nasm to assemble asm
> nasm -f elf64 toupper_avx2.asm

> nasm -f nasm -f elf64 toupper_avx2_pthread.asm

- We have included pre-assembled object files as well.
- Simply run make to compile all different 8 variants.

# Variants
- Variant 1: GCC O0
- Variant 2: GCC O1
- Variant 3: GCC O2
- Variant 4: GCC O3
- Variant 5: CLANG O0
- Variant 6: CLANG O1
- Variant 7: CLANG O2
- Variant 8: CLANG O3

# Creating Graphs
- Run make to create results
> make PARAMS="-d -l 100000 50000000 1000000"
- Run ./create_graphs.sh to create performance comparison graphs.
- Make sure you have gnuplot installed.