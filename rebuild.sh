# rm -rf build/*
cd build
cmake .. && make -j

# cd src

# # build 
# make clean && make -j >/dev/null 2>&1
# rm libparser.a
# ar crf libparser.a lex.yy.o y.tab.o
# make clean && make -j