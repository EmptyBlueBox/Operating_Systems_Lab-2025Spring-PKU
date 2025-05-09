# XV6-RISCV On K210

- Course: [Operating System](https://github.com/EmptyBlueBox/Operating_Systems-2025Spring-PKU)
- Lecturer: [陈向群(教授)](https://cs.pku.edu.cn/info/1062/1604.htm)
- Year: 2025 Spring
- Department: School of EECS, PKU
- Welcome to my [course review](https://www.lyt0112.com/blog/course_review-zh)!

## Dependencies

- `k210` development board or `qemu-system-riscv64`
- RISC-V GCC compiler: [riscv-gnu-toolchain](https://github.com/riscv/riscv-gnu-toolchain.git)

    ```bash
    sudo apt install gcc-riscv64-linux-gnu
    ```
- Docker

## Usage

### Environment preparation

Pull docker image:

```bash
docker pull docker.educg.net/cg/os-contest:2024p6
```

Clone and enter the `xv6-k210` directory:

```bash
git clone git@github.com:EmptyBlueBox/Operating_Systems_Lab-2025Spring-PKU.git
mv Operating_Systems_Lab-2025Spring-PKU xv6-k210
cd xv6-k210
```

Enter the `docker` container:

```bash
sudo docker run -ti --rm -v .:/xv6 --privileged=true docker.educg.net/cg/os-contest:2024p6 /bin/bash
```

### Run testcases

Enter the `xv6-k210` directory:

```bash
cd xv6
```

Clean the intermediate files:

```bash
make clean
```

Build the kernel and user programs:

```bash
make build
```

Generate the hexadecimal string of `xv6-user/init.c`:

```bash
./init2binary.sh
```

Make again and run:

```bash
make clean
make fs
make run
```

**In summary**, join the instructions inside the `docker` container together for copying:

```bash
cd xv6
make clean
make build
./init2binary.sh
make clean
make fs
make run
```

## Troubleshooting

- Tests not running
    ```bash
    hart 0 init done
    init: exec brk failed
    init: exec chdir failed
    init: exec clone failed
    init: exec close failed
    init: exec dup failed
    init: exec dup2 failed
    init: exec execve failed
    init: exec exit failed
    init: exec fork failed
    init: exec fstat failed
    init: exec getcwd failed
    init: exec getdents failed
    init: exec getpid failed
    init: exec getppid failed
    init: exec gettimeofday failed
    init: exec mkdir_ failed
    init: exec mmap failed
    init: exec mount failed
    init: exec munmap failed
    init: exec open failed
    init: exec openat failed
    init: exec pipe failed
    init: exec read failed
    Usage: sleep TIME
    init: exec test_echo failed
    init: exec times failed
    init: exec umount failed
    init: exec uname failed
    init: exec unlink failed
    init: exec wait failed
    init: exec waitpid failed
    init: exec write failed
    init: exec yield failed
    ```
    - Problem with `Makefile`, you should use `@cp -R tests/* $(dst)` instead of `@cp -R tests $(dst)`, otherwise it'll copy the folder and cause the local test not running.

- The following system calls are currently not working or failing their tests:
    - `mmap`
    - `munmap`
    - `openat`
    - `unlink`
    - `close`
