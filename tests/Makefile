SRCDIR   = ../src
TESTDIR  = .

SRCOBJS  := $(addprefix $(SRCDIR)/, net-utils.o net-arp.o net-ethernet.o)
TESTOBJS := $(addprefix $(TESTDIR)/, test.o net-arp.o net-utils.o)
OBJS     := $(SRCOBJS) $(TESTOBJS)

INCLUDE  = -I../uspi/include/ -I..

CC := clang
CXX := clang++

CFLAGS	 += $(ARCH) $(INCLUDE) -MMD -MP -Wall -Wno-psabi -fsigned-char -fno-builtin -g -DNDEBUG -fprofile-instr-generate -fcoverage-mapping -fsanitize=address,undefined -fno-omit-frame-pointer
CXXFLAGS := $(CFLAGS) $(CXXFLAGS) -std=c++11
CFLAGS	 += -fno-delete-null-pointer-checks -fdata-sections -ffunction-sections -u _printf_float -std=gnu99

TARGET  ?= test

DEPENDS := $(patsubst %.o,%.d,$(OBJS))
-include $(DEPENDS)

.PHONY: all

all: $(TARGET)

coverage: $(TARGET)
	LLVM_PROFILE_FILE="$(TARGET).profraw" ./$(TARGET)
	llvm-profdata merge -sparse "$(TARGET).profraw" -o "$(TARGET).profdata"
	llvm-cov show ./$(TARGET) -instr-profile="$(TARGET).profdata" ../src

$(TARGET): $(OBJS)
	@echo "  LINK $@"
	$(Q)$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	$(Q)$(RM) $(SRCDIR)/*.o $(TESTDIR)/*.o $(SRCDIR)/*.d $(TESTDIR)/*.d $(TARGET) kernel
