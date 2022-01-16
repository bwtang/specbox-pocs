# PoCs about security evaluation of SpecBox
This repository contains the PoCs of spectre attacks, such as Spectre-PHT, Spectre-BTB (also known as Spectre-BTI), Spectre-RSB, Spectre-STL. The goal of the PoCs is to demonstrate the effectiveness of "SpecBox" defense, which is proposed by the paper

"**SpecBox: A Label-Based Transparent Speculation Scheme Against Transient Execution Attacks**".

The code only works on GEM5 simulator with O3 CPU Model and Ruby Cache System. The contruction of these PoCs refers to the work https://github.com/IAIK/transientfail, so we believe that with a slight modification to the PoC, it can also be reproduced on real world hardwares. (Noticed that although the attack principles has no dependence with specific ISA, some PoCs code includs the X86 "inline assembly" code.)

# Building
The PoCs must be compiled with ’-O0‘ optimizing flag and '-static' linking flag. In addition, for the PoC of Spectre-RSB, it needs additional '-fno-stack-protector' flag to disable the stack canary. In our experiments, the compiler version is "gcc-5.4.0" and the standard library version is "glibc-2.23", which are normally equipped on Ubuntu 16.04 system.

# Running
In our experiments, we choose the version "*fe187de9bd1aa479ab6cd198522bfd118d0d50ec*" for GEM5 simulator (We believe the version does not affects the PoCs, just need to update some running options). After installing and building the simulator (can guided by GEM5 official website), and also afer building the executable file of the PoCs, we can run them and observe the output in the terminal. The running options of our simulator are:
```
$GEM5_DIR/build/X86_MESI_Two_Level/gem5.fast \
  $GEM5_DIR/configs/example/se.py \
  -c spectre-pht-icache \
  -o '5 this-is-a-secret' \
  --num-cpus=1 --mem-size=4GB \
  --l1d_assoc=8 --l2_assoc=16 --l1i_assoc=4 \
  --cpu-type=DerivO3CPU \
  --ruby --num-dirs=1 --network=simple --topology=Mesh_XY --mesh-rows=1
```
In the options, the '-o' option indicates the arguments for the PoC application. It has two parts, the first part (i.e. "5") means the repeat numbers of each attack iteration, the second part (i.e. "this-is-a-secret") means the secret string to transmit.

# Author
Bowen Tang (tangbowen@ict.ac.cn), a PhD candidate from Institute of Computing Technology, Chinese Academy of Sciences.
