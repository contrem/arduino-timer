TESTS := $(wildcard *Test)

UNIXHOSTDUINO := ../../../UnixHostDuino/UnixHostDuino.mk
UNIXHOSTDUINO_URL := https://github.com/bxparks/UnixHostDuino
UNIXHOSTDUINO_TAG := 1936444e0097a37ad268e5e3d6e6cca8d957a86b
AUNIT := ../../../AUnit/
AUNIT_URL := https://github.com/bxparks/AUnit
AUNIT_TAG := e0aa8dfb635101d170cdb9fd9669e1d8047e2db1

### ### ###

UHD_DIR := $(dir $(UNIXHOSTDUINO))
AU_DIR := $(dir $(AUNIT))
SUB_DIRS := $(addsuffix /, $(TESTS))
TEST_DIRS := $(join $(SUB_DIRS), $(TESTS))

INOS := $(addsuffix .ino, $(join $(SUB_DIRS), $(TESTS)))
BINS := $(addsuffix .out, $(TEST_DIRS))
RUNS := $(addsuffix .run, $(BINS))
CLEANS := $(addsuffix .clean, $(SUB_DIRS))

all: $(BINS)

$(UHD_DIR):
	git clone $(UNIXHOSTDUINO_URL) $@
	cd $@; git checkout $(UNIXHOSTDUINO_TAG)

$(AU_DIR):
	git clone $(AUNIT_URL) $@
	cd $@; git checkout $(AUNIT_TAG)

$(BINS): $(INOS) $(UHD_DIR) $(AU_DIR)
	# ==== Building: $(dir $@)
	$(MAKE) UNIXHOSTDUINO=../$(UNIXHOSTDUINO) -C $(dir $@)

$(RUNS): $(BINS)
	# ==== Running: $(@:.run=)
	$(@:.run=)

$(CLEANS):
	# ==== Cleaning: $(@:.clean=)
	$(MAKE) UNIXHOSTDUINO=../$(UNIXHOSTDUINO) -C $(@:.clean=) clean

tests: $(BINS)

runtests: $(RUNS)

clean: $(CLEANS)

test: runtests clean
