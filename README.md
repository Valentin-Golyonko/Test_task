# Test_task
Test task with multi processes and threads on linux.

The following program implements three processes: A, B, C.

- Process A is used to continuously enter data from a user (number).

- The number of process A entered by the user must pass to process B using the PIPE mechanism.

- Process B should square the entered number and use the shared memory mechanism to transfer the result of squaring into the process C.

- Process C must consist of two streams: the C-1 stream and the C-2 stream.

- Stream C1 must receive data from process B using the Shared memory mechanism, the C2 stream should output an arbitrary message to the terminal once a second (for example, "I am alive \ n"),

- At the moment when the stream C1 receives data from process B - it should output to the terminal the message "value = XXXX", where XXXX is the number transmitted through the shared memory mechanism from process B (The result of raising the power of the number entered in process A).

- If the result of squaring = "100" (that is, the user entered the number 10), then the program must send a signal SIGUSR1 to process B, which in the case of receiving such a signal must be terminated all three processes (A, B, C) correctly.
