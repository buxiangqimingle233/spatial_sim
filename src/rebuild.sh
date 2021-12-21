cat /dev/null > in_flight_flits.txt
make clean && make -j
./booksim examples/focusconfig
# ./booksim examples/testconfig