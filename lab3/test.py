import os
import time
import random
import argparse
import logging
from tqdm.auto import tqdm


def generate_test_command():
    # Usage: ./mmu-generator [-P numprocs] [-V numvmas] [-i numinst]
    #         [-E numexits ] [ -m ] [ -w ] [-H numholes ]
    #         [-r readpercentage] [-L lambda] [-p numvirtpages] [-v]
    # -P numprocs:  <numprocs> processes are generated
    # -V numvmas:   Each process has between 1 and <numvmas> VMAs
    # -i numinst:   <numinst> instructions are generated
    # -E numexits:  <numexits> processes will exit, should be less than <numprocs>
    # -m:           randomly set upto one VMA per process to memory mapped, i.e. filemapped
    # -w:           randomly set upto one VMA per process to writeprotected
    # -H:           randomly create holes between VMAs so to not fully populated the address space
    # -r <perc>     percentage of instruction to be read (vs write) [ 0 .. 100]
    # -L <lambda>:   some selection is driven stochastically to create more locality.
    #               choose a value between [1.0 - 5.0 ]
    # -p <numpages>:simulation can be based on different address space sizes other than 64

    P = random.randint(1, 9)
    V = random.randint(1, 8)
    i = 1000000  # 1 million instructions
    E = random.randint(1, P)
    m = random.choice([True, False])
    w = random.choice([True, False])
    H = random.choice([True, False])
    r = random.randint(0, 100)
    L = random.uniform(1.0, 5.0)
    p = 64  # 64 pages

    command = f"{args.gen} -P {P} -V {V} -i {i} -E {E} -r {r} -L {L}"
    if m:
        command += " -m"
    if w:
        command += " -w"
    if H:
        command += " -H"

    return command


if __name__ == "__main__":
    parser = argparse.ArgumentParser("Run random tests for your Virtual Memory Manager")
    parser.add_argument(
        "--src", "-s", type=str, help="Path to your source code", default="./mmu"
    )
    parser.add_argument(
        "--ref",
        "-r",
        type=str,
        help="Path to the reference implementation",
        default="./mmu-ref",
    )
    parser.add_argument(
        "--gen",
        "-g",
        type=str,
        help="Path to the test generator",
        default="./mmu-generator",
    )
    parser.add_argument(
        "--num-tests", "-n", type=int, help="Number of tests to run", default=1000
    )
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[logging.FileHandler("test.log")],
    )

    frame_sizes = [16, 31, 32]
    alorithms = ["f", "c", "e", "a", "r", "w"]
    for i in tqdm(range(args.num_tests)):
        test_command = generate_test_command()
        os.system(f"{test_command} > generated_input")
        for frame_size in frame_sizes:
            for algorithm in alorithms:
                logging.info(f"Running test for {test_command} -f{frame_size} -{algorithm}")
                # ./mmu -f16 -aa -oOPFS in1 rfile > out
                start_time = time.time()
                os.system(
                    f"{args.src} -f{frame_size} -a{algorithm} -oOPFS generated_input rfile > output_src"
                )
                end_time = time.time()
                if end_time - start_time >= 30:
                    logging.error(
                        f"Test timed out for {test_command} -f{frame_size} -{algorithm}: {end_time - start_time}"
                    )
                os.system(
                    f"{args.ref} -f{frame_size} -a{algorithm} -oOPFS generated_input rfile > output_ref"
                )
                os.system(f"diff output_src output_ref > diff")
                if open("diff").read().strip() != "":
                    logging.error(
                        f"Test failed for {test_command} -f{frame_size} -{algorithm}"
                    )
                os.system("cmp output_src output_ref > cmp")
                if open("cmp").read().strip() != "":
                    logging.error(
                        f"Test failed for {test_command} -f{frame_size} -{algorithm}"
                    )
                os.remove("output_src")
                os.remove("output_ref")
                os.remove("diff")
                os.remove("cmp")
