# =============================================================================
# Makefile — Parallel Monte Carlo Risk Engine
# =============================================================================
#
# Targets:
#   make          Build the risk_engine binary (default)
#   make run      Build and run with default example parameters
#   make clean    Remove all build artefacts
#
# Requirements:
#   gcc, POSIX-compliant OS (Linux / macOS), GNU make
#
# Compiler flags:
#   -Wall -Wextra          Full warning set (required for course submission)
#   -std=c11               C11 language standard
#   -D_DEFAULT_SOURCE      Enable glibc default feature set: includes POSIX.1,
#                            BSD extensions, and legacy functions such as rand_r()
#                            (rand_r was removed from POSIX.1-2008 but is present
#                            in glibc as a legacy API — _DEFAULT_SOURCE exposes it)
#   -O2                    Optimise: meaningful for large Monte Carlo runs
#   -MMD -MP               Auto-generate header dependency files (.d)
#                          so that editing a .h triggers correct recompilation
#   -lm                    Link the C math library (sqrt, log, exp, erfc, cos)
# =============================================================================

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -D_DEFAULT_SOURCE -O2 -MMD -MP
LDFLAGS := -lm

TARGET  := risk_engine

SRCS    := main.c         \
           controller.c   \
           worker.c       \
           mc.c           \
           option_pricing.c \
           stats.c        \
           util.c

OBJS    := $(SRCS:.c=.o)
DEPS    := $(SRCS:.c=.d)   # auto-generated dependency files

# Default example invocation (used by `make run`)
DEFAULT_ARGS := 4 50000 100 100 0.05 0.2 0.25

# -----------------------------------------------------------------------------

.PHONY: all run clean pdf

all: $(TARGET)

# Link step
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile step (pattern rule; dependencies injected via .d files)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Include auto-generated dependency files (silently ignore if absent)
-include $(DEPS)

# Build and run with default parameters
run: $(TARGET)
	./$(TARGET) $(DEFAULT_ARGS)

# Generate project.pdf (requires Python 3 + reportlab: pip install reportlab)
pdf: gen_pdf.py
	python3 gen_pdf.py || python gen_pdf.py

# Remove all generated files
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)
