# XV6-RISCV On K210

## Dependencies
- `k210` development board or `qemu-system-riscv64`
- RISC-V GCC compiler: [riscv-gnu-toolchain](https://github.com/riscv/riscv-gnu-toolchain.git)

    ```bash
    sudo apt install gcc-riscv64-linux-gnu
    ```
- Docker

## Usage

Enter the `xv6-k210` directory:

```bash
cd xv6-k210
```

Enter the `docker` container:

```bash
sudo docker run -ti --rm -v .:/xv6 --privileged=true docker.educg.net/cg/os-contest:2024p6 /bin/bash
```

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

At last, join the instructions inside the `docker` container together for copying:

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

- `Makefile`
    - `	@cp -R tests/ $(dst)` instead of `	@cp -R tests $(dst)`, otherwise it'll copy the folder.
