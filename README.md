# FP-Growth for Association Rule Mining
## Introduction
This is a c++ implementation of FP-Growth algorithm, which is the course project of CMSC5724. The algorithm procedure can be found here. [asso-fpgrowth.pdf](http://www.cse.cuhk.edu.hk/~taoyf/course/cmsc5724/18-fall/lec/asso-fpgrowth.pdf)

## Prerequisite

The executable is compiled on macOS, if you use other platform. Please make use cmake is installed and compile the project with the following commands.

```
mkdir build
cd build
cmake ../
make
```

## Usage

```
./fp_growth data_file min_support[0-1] min_confidence[0-1]
```

For example, to run on the provided dataset, run the following command line

```
./fp_growth ./asso.csv 0.1 0.9
```


