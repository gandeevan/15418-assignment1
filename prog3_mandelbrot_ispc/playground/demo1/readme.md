 # HOW TO EMIT ASSEMBLY
 ispc sum.ispc -o sum_ispc.s -O3 --opt=disable-handle-pseudo-memory-ops --emit-asm --target=sse4-x2
 g++ -S main.cpp -o main.s

 # BUILD INSTRUCTIONS
 ispc sum.ispc -o sum_ispc.o -O3 --pic
 g++ main.cpp sum_ispc.o -o main