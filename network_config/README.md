# Network Configuration
## Overview
mNPUsim receives 'network configuration list file' (.txt) as a second input. \
'Network configuration list file' contains list of 'network architecture configuration file' (.csv) name, one by one in each line.

## Network Configuration File Specification
The network configuration file format mostly follows SCALE-Sim v2 format. \
Currently, this simulator supports Im2col_conv / Conv / Pool / Gemm.

## Address in Network Architecture (IFMAP Base Addr, Filter Base Addr, Ofmap Base Addr)
You can manually manage the start address of ifmap/filter/ofmap.
There are several options of connection.
* -1 (Sequential connection): the ifmap of this layer is the ofmap of previous layer.
* -2 (New address connection):  the ifmap of this layer is the new-space (new tensor in this network).
* *n (<= BOUNDARY_IFMAP_OFMAP_ARRAY)*: the ifmap/filter/ofmap is the output of *n*th layer.
* *m (> BOUNDARY_IFMAP_OFMAP_ARRAY)*: the ifmap/filter/ofmap is the input of *m-BOUNDARY_IFMAP_OFMAP_ARRAY*th layer.

The address is translated from base network file to intermediate network file.
Therefore, the ifmap/filter/ofmap base address values in intermediate network files are absolute value while relative values in original network files.
