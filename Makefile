CC ?= gcc
CFLAGS ?= -Wall -Wextra -g
LDLIBS ?= -lgpiod
LDFLAGS = -Wl,--hash-style=gnu -Wl,--as-needed

# Application sources and executables
USER_APP_SRC = user_app.c
USER_APP_EXEC = user_app
TEST_APP_SRC = test_app.c
TEST_APP_EXEC = test_app

all: $(USER_APP_EXEC) $(TEST_APP_EXEC)

$(USER_APP_EXEC): $(USER_APP_SRC)
	$(CC) $(CFLAGS) $(USER_APP_SRC) -o $(USER_APP_EXEC) $(LDFLAGS) $(LDLIBS)

$(TEST_APP_EXEC): $(TEST_APP_SRC)
	$(CC) $(CFLAGS) $(TEST_APP_SRC) -o $(TEST_APP_EXEC) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(USER_APP_EXEC) $(TEST_APP_EXEC)
