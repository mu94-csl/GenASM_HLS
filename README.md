# GenASM Framework on FPGA using HLS 

ECE 5775 Final Project.

This project uses SDSoC v2018.2 or v2019.1, and targets the ZCU104 board.


## Instructions to build

Initialize your SDx/SDSoC environment.

CSim and HLS synthesis (SDSoC stuff pragma'd out) can be run inside `src` directory using the Makefile.

For deployment on the board, the hardware needs to be built inside `hw_build_*` using `make`.
This should result in a sd_card directory containing BOOT.BIN, a Linux image, and the GenASM shared library.
For the host software, run `make` inside `sw_build` to build an elf application.
Copy these files on to an SD card and insert into board to boot.




## Example Builds 

Already built files have been provided for ZCU104, inside the `example_builds` folder, using SDSoC v2018.2.

Copy the contents of the baseline/optimized subdirectory to an SD card, and insert into board to boot from it.

On the board, do

    export LD_LIBRARY_PATH=/mnt/
    cd /mnt
    ./host.elf <mode>

where `<mode>` should be replaced by 1/2 for processing the sample short/long reads dataset.



## Notes

The GenASM software has been modified to support only a maximum of 64 pattern length at a time. The divide-and-conquer approach must be used for longer strings.

## References

D. S. Cali et al., "GenASM: A High-Performance, Low-Power Approximate String Matching Acceleration Framework for Genome Sequence Analysis," 2020 53rd Annual IEEE/ACM International Symposium on Microarchitecture (MICRO), 2020, pp. 951-966, doi: 10.1109/MICRO50266.2020.00081.

