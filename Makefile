all:
	@echo nothing

main: main/main.touch

main/main.touch: main/main.ino
	arduino-cli compile -b arduino:avr:mega main
	touch main/main.touch

inst: main/main.touch 
	arduino-cli upload -p /dev/ttyUSB0 -b arduino:avr:mega main

clean: 
	rm main/main.touch 

