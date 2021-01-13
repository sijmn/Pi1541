SRCDIR   = ../src
TESTDIR  = .

SRCOBJS  := net-utils.o net-arp.o net-ethernet.o
TESTOBJS := test.o net-arp.o net-utils.o
OBJS    := $(addprefix $(TESTDIR)/, $(TESTOBJS)) $(addprefix $(SRCDIR)/, $(SRCOBJS))

INCLUDE  = -I../uspi/include/ -I..

CC := clang
CXX := clang++

CFLAGS	 += $(ARCH) $(INCLUDE) -MMD -MP -Wall -Wno-psabi -fsigned-char -fno-builtin -g -DNDEBUG
CXXFLAGS := $(CFLAGS) $(CXXFLAGS) -std=c++11
CFLAGS	 += -fno-delete-null-pointer-checks -fdata-sections -ffunction-sections -u _printf_float -std=gnu99

TARGET  ?= test

DEPENDS := $(patsubst %.o,%.d,$(OBJS))
-include $(DEPENDS)

.PHONY: all

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "$(OBJS)"
	@echo "  LINK $@"
	$(Q)$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	$(Q)$(RM) $(SRCDIR)/*.o $(TESTDIR)/*.o $(SRCDIR)/*.d $(TESTDIR)/*.d $(TARGET) kernel
