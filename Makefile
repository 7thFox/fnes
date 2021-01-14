
clean:
	rm -rf bin

bin/:
	mkdir bin

build: bin/
	g++ -lncurses -o bin/6502emu *.cpp */*.cpp

run: build
	bin/6502emu

test: build
	bin/6502emu --test

testv: build
	bin/6502emu --test --verbose