all: recv-loop recvmmsg-loop trunc-loop bpf-drop ebpf-drop

.PHONY: recv-loop
recv-loop:
	gcc -Wall -Wextra -O2 recv-loop.c -o recv-loop

.PHONY: recvmmsg-loop
recvmmsg-loop:
	gcc -Wall -Wextra -O2 recvmmsg-loop.c -o recvmmsg-loop

.PHONY: trunc-loop
trunc-loop:
	gcc -Wall -Wextra -O2 trunc-loop.c -o trunc-loop

.PHONY: bpf-drop
bpf-drop:
	gcc -Wall -Wextra -O2 bpf-drop.c -o bpf-drop


.PHONY: ebpf-drop
ebpf-drop:
	gcc -Wall -Wextra -O2 ebpf-drop.c -o ebpf-drop
