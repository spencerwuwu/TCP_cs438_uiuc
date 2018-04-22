SENDER=reliable_sender
RECEIVER=reliable_receiver

SENDER-DEBUG=reliable_sender-debug
RECEIVER-DEBUG=reliable_receiver-debug

# set up compiler
CC = gcc
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter
CFLAGS   = -std=gnu11 -pthread
DEBUG	 = -g

all: $(SENDER) $(RECEIVER)
sender: $(SENDER)
receiver: $(RECEIVER)
debug: $(SENDER-DEBUG) $(RECEIVER-DEBUG)

$(SENDER): tcp_sender.c sender_main.c
	$(CC) $^ $(CFLAGS) -o $(SENDER)

$(RECEIVER): tcp_receiver.c receiver_main.c
	$(CC) $^ $(CFLAGS) -o $(RECEIVER)

$(SENDER-DEBUG): tcp_sender.c sender_main.c
	$(CC) $^ $(CFLAGS) $(DEBUG) -o $(SENDER-DEBUG)

$(RECEIVER-DEBUG): tcp_receiver.c receiver_main.c
	$(CC) $^ $(CFLAGS) $(DEBUG) -o $(RECEIVER-DEBUG)

clean:
	$(RM) $(SENDER) $(RECEIVER) $(SENDER-DEBUG) $(RECEIVER-DEBUG)
