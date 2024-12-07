import os
import time
import random
import argparse
import logging
from tqdm.auto import tqdm
from concurrent.futures import ThreadPoolExecutor, as_completed


def generate_test_command():
    # Usage: iomake [-v] [-t maxtracks] [-i num_ios] [-L lambda] [-f interarrival_factor]
    # maxtracks is the tracks the disks will have, default is 512
    # num_ios  is the number of ios to generate, default is 32
    # lambda is parameter to create a poisson distribution, default is 1.0   ( consider ranges from 0.01 .. 10.0 )
    # interarrival_factor  is  time  factor  how  rapidly  IOs  will  arrive,  default  is  1.0  (  consider  values  0.5 .. 1.5  ),  too  small  and  the
    # system will be overloaded and too large it will be underloaded and scheduling is mute as often only one i/o is outstanding.

    maxtracks = random.randint(512, 1024)
    num_ios = 10000
    _lambda = random.randint(1, 1000) / 100
    interarrival_factor = random.choice([0.5, 0.75, 1.0, 1.25, 1.5])
    command = f"{args.gen} -v -t {maxtracks} -i {num_ios} -L {_lambda} -f {interarrival_factor}"
    return command


def run_test_command(src, algorithm, infile, outfile):
    # ./iosched -sF input0
    start_time = time.time()
    os.system(f"{src} -s{algorithm} {infile} > {outfile}")
    runtime = time.time() - start_time
    return runtime


def run_test_for_algorithm(
    sourcefile, referencefile, test_id, algorithm, infile, time_limit
):
    test_outcome = True
    try:
        # Run the test with source implementation
        source_outfile = f"output_src_{test_id}_{algorithm}"
        source_run_time = run_test_command(
            sourcefile, algorithm, infile, source_outfile
        )

        # Run the test with reference implementation
        reference_outfile = f"output_ref_{test_id}_{algorithm}"
        reference_run_time = run_test_command(
            referencefile, algorithm, infile, reference_outfile
        )

        # Check time difference
        runtime_difference = source_run_time - reference_run_time
        if runtime_difference >= time_limit:
            logging.error(
                f"[Test {test_id}, Algo={algorithm}] Implementation takes {runtime_difference} seconds more to execute"
            )
            test_outcome = False

        # Check output difference using diff
        os.system(
            f"diff {source_outfile} {reference_outfile} > diff_{test_id}_{algorithm}"
        )
        if open(f"diff_{test_id}_{algorithm}").read().strip() != "":
            logging.error(f"[Test {test_id}, Algo={algorithm}] DIFF FAILED")
            test_outcome = False

        # Check output difference using cmp
        os.system(
            f"cmp {source_outfile} {reference_outfile} > cmp_{test_id}_{algorithm}"
        )
        if open(f"cmp_{test_id}_{algorithm}").read().strip() != "":
            logging.error(f"[Test {test_id}, Algo={algorithm}] CMP FAILED")
            test_outcome = False
    finally:
        # Clean up
        for file in [
            source_outfile,
            reference_outfile,
            f"diff_{test_id}_{algorithm}",
            f"cmp_{test_id}_{algorithm}",
        ]:
            if os.path.exists(file):
                os.remove(file)

    return algorithm, test_outcome, test_id


if __name__ == "__main__":
    parser = argparse.ArgumentParser("Run random tests for your IO Scheduler")
    parser.add_argument(
        "--src", "-s", type=str, help="Path to your source code", default="./iosched"
    )
    parser.add_argument(
        "--ref",
        "-r",
        type=str,
        help="Path to the reference implementation",
        default="./iosched-ref",
    )
    parser.add_argument(
        "--gen",
        "-g",
        type=str,
        help="Path to the test generator",
        default="./iomake",
    )
    parser.add_argument(
        "--num-tests",
        "-n",
        type=int,
        help="Number of tests to run per algorithm, e.g: a value of 10 will run 10x5 = 50 tests",
        default=1000,
    )
    parser.add_argument(
        "--threads",
        "-t",
        type=int,
        help="Number of threads to use for parallel testing. Defaults to 5 to run each algorithm in parallel",
        default=5,
    )
    parser.add_argument(
        "--time-limit",
        "-l",
        type=int,
        help="Permitted ime limit difference in seconds between source and reference implementation. Defaults to 2 seconds",
        default=2,
    )
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.INFO,
        filename="test.log",
        format="%(asctime)s - %(levelname)s - %(message)s",
        filemode="w",
    )

    algorithms = {"N": 0, "S": 0, "L": 0, "C": 0, "F": 0}
    total_tests = args.num_tests * len(algorithms)
    failed = 0
    progress_bar = tqdm(total=total_tests)

    with ThreadPoolExecutor(max_workers=args.threads) as executor:
        futures = []
        for test_id in range(args.num_tests):
            # Generate input
            test_command = generate_test_command()
            infile = f"infile{test_id}"
            os.system(f"{test_command} > {infile}")
            logging.info(f"[Test {test_id}] Running tests with: {test_command}")
            for algorithm in algorithms:
                futures.append(
                    executor.submit(
                        run_test_for_algorithm,
                        args.src,
                        args.ref,
                        test_id,
                        algorithm,
                        infile,
                        args.time_limit,
                    )
                )

        for future in as_completed(futures):
            algorithm, test_outcome, test_id = future.result()
            if not test_outcome:
                failed += 1
                logging.error(f"[Test {test_id}, Algo={algorithm}] FAILED")
            else:
                logging.info(f"[Test {test_id}, Algo={algorithm}] PASSED")
                algorithms[algorithm] += 1
            progress_bar.update(1)

    # Clean up
    for test_id in range(args.num_tests):
        os.remove(f"infile{test_id}")

    # Print the final stats
    logging.info(f"Tests completed successfully")
    logging.info(f"Tests failed: {failed}/{total_tests}")
    logging.info(f"Algorithm stats:")
    for algorithm, count in algorithms.items():
        logging.info(f"{algorithm}: {count}/{args.num_tests}")
