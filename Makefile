CXX ?= g++
PKG_CONFIG ?= pkg-config
CXXFLAGS ?= -O2 -Wall -Wextra -std=c++17
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk4)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk4)
SOURCES := main.cc components/keyboard/keyboard.cc components/ui/ui.cc
HEADERS := $(wildcard components/*/*.hh)

.PHONY: all check-deps preview clean
all: ecc
check-deps:
	@$(PKG_CONFIG) --atleast-version=4.12 gtk4 || { echo 'GTK 4.12+ development files are required.'; exit 1; }
ecc: $(SOURCES) $(HEADERS) | check-deps
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GTK_CFLAGS) $(SOURCES) -o $@ $(LDFLAGS) -pthread $(GTK_LIBS)
preview: ecc
	ECC_PREVIEW=1 ./ecc
clean:
	rm -f ecc

.PHONY: test
test:
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -I. tests/profile_test.cc -o build/profile-test
	./build/profile-test
	$(CXX) $(CXXFLAGS) -I. tests/keyboard_test.cc components/keyboard/keyboard.cc -pthread -o build/keyboard-test
	./build/keyboard-test
	$(CXX) $(CXXFLAGS) -I. tests/lighting_worker_test.cc -pthread -o build/lighting-worker-test
	./build/lighting-worker-test

.PHONY: install-user
install-user: ecc
	python3 packaging/install-user.py

.PHONY: diagnose
diagnose:
	python3 packaging/diagnose.py
