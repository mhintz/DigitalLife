.PHONY: build run

all: build run

build:
	xcodebuild -configuration Debug -project xcode/DigitalLife.xcodeproj/

run: build
	./xcode/build/Debug/DigitalLife.app/Contents/MacOS/DigitalLife
